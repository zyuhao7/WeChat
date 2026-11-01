#include "filetransfermgr.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QStringList>
#include <QUuid>

#include "tcpmgr.h"

namespace
{
// Chunks handed to the socket but not yet acked. The server relays each frame immediately, so a
// deeper window buys nothing but memory on the receiving side's queue.
const int SEND_WINDOW = 16;
// A transfer that sees no ack (send) or no chunk (recv) for this long is dead: the peer went
// offline, the relay dropped a frame, or the app was suspended.
const qint64 STALL_MS = 30 * 1000;
const int WATCHDOG_MS = 5 * 1000;
// A receiver that is reconnecting looks offline to the relay. Keep the chunks queued and
// retry often enough to catch its return, but not forever: after this window the peer is
// treated as gone and the transfer fails.
const qint64 PEER_RETRY_MS = 5 * 1000;
const qint64 PEER_GRACE_MS = 60 * 1000;

qint64 Now()
{
    return QDateTime::currentMSecsSinceEpoch();
}

// The server rejects these too; a name that reaches the receiver with one is a bug or an
// attack, so it never gets joined onto a path.
bool IsBadNameChar(ushort c)
{
    return c < 0x20 || c == 0x7f || c == '/' || c == '\\' || c == ':';
}
} // namespace

FileTransferMgr::FileTransferMgr()
{
    auto tcp = TcpMgr::GetInstance();
    connect(tcp.get(), &TcpMgr::sig_file_rsp, this, &FileTransferMgr::slot_file_rsp);
    connect(tcp.get(), &TcpMgr::sig_file_msg, this, &FileTransferMgr::slot_file_chunk);
    connect(tcp.get(), &TcpMgr::sig_connection_closed, this, &FileTransferMgr::slot_connection_closed);
    connect(tcp.get(), &TcpMgr::sig_reconnected, this, &FileTransferMgr::slot_reconnected);
    connect(tcp.get(), &TcpMgr::sig_reconnect_failed, this, &FileTransferMgr::slot_reconnect_failed);

    _watchdog.setInterval(WATCHDOG_MS);
    connect(&_watchdog, &QTimer::timeout, this, &FileTransferMgr::slot_check_timeouts);
    _watchdog.start();
}

FileTransferMgr::~FileTransferMgr()
{
    _sending.clear();
    for (auto& inc : _receiving)
    {
        // a half-written .part would shadow the next transfer under the same name
        inc->file.close();
        QFile::remove(inc->file.fileName());
    }
    _receiving.clear();
}

QString FileTransferMgr::SendFile(int to_uid, const QString& path, QString* error)
{
    // a rejection before the first byte has no file id for the caller to match a bubble to,
    // so it also hands the reason back directly
    const auto reject = [this, error](const QString& reason) {
        if (error != nullptr)
            *error = reason;
        emit sig_send_failed(QString(), reason);
        return QString();
    };

    if (to_uid <= 0)
        return reject(QStringLiteral("对方不在线"));

    QFileInfo info(path);
    if (!info.exists() || !info.isFile())
    {
        return reject(QStringLiteral("文件不存在"));
    }
    if (info.size() <= 0)
    {
        return reject(QStringLiteral("不能发送空文件"));
    }
    if (info.size() > MAX_FILE_BYTES)
    {
        return reject(QStringLiteral("文件超过 100 MB"));
    }

    const QString name = SanitizeFileName(info.fileName());
    if (name.isEmpty())
    {
        return reject(QStringLiteral("文件名无效"));
    }

    auto out = std::make_shared<OutgoingFile>();
    out->file_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    out->to_uid = to_uid;
    out->name = name;
    out->size = info.size();
    out->total = ChunkCount(out->size);
    out->acked_flags.assign(out->total, 0);
    out->last_ack = Now();
    out->file.setFileName(path);
    if (!out->file.open(QIODevice::ReadOnly))
    {
        return reject(QStringLiteral("无法读取文件"));
    }

    const QString file_id = out->file_id;
    _sending.insert(file_id, out);
    Pump(*out);
    // Pump fails the transfer itself when the first read or write goes wrong
    return _sending.contains(file_id) ? file_id : QString();
}

void FileTransferMgr::Pump(OutgoingFile& out)
{
    while (out.acked < out.total && out.in_flight.size() < SEND_WINDOW && out.next_index < out.total)
    {
        const int index = out.next_index;
        const qint64 len = ChunkLength(out.size, index);
        QByteArray chunk(static_cast<int>(len), Qt::Uninitialized);
        if (!out.file.seek(index * static_cast<qint64>(FILE_CHUNK_BYTES)) ||
            out.file.read(chunk.data(), len) != len)
        {
            FailSend(out.file_id, QStringLiteral("读取文件失败"));
            return;
        }

        QJsonObject obj;
        obj["touid"] = out.to_uid;
        obj["fileid"] = out.file_id;
        obj["filename"] = out.name;
        // the server validates this field as an int; MAX_FILE_BYTES fits, and a fractional
        // double would be rejected outright
        obj["filesize"] = static_cast<int>(out.size);
        obj["index"] = index;
        obj["total"] = out.total;
        obj["data"] = QString::fromLatin1(chunk.toBase64());

        out.in_flight.insert(index);
        ++out.next_index;
        emit TcpMgr::GetInstance() -> sig_send_data(ReqId::ID_FILE_MSG_REQ,
                                                    QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }
}

void FileTransferMgr::slot_file_rsp(int error, const QString& file_id, int index)
{
    auto it = _sending.find(file_id);
    if (it == _sending.end())
        return; // a reply for a transfer that already finished or failed

    auto out = it.value();
    if (error != ErrorCodes::SUCCESS)
    {
        if (IsPeerUnavailableError(error))
        {
            // the receiver is offline, usually just reconnecting: drop the in-flight
            // window and retry from the first unacked chunk after a short delay
            const bool was_in_flight = out->in_flight.remove(index);
            if (!was_in_flight && out->peer_offline_since != 0)
                return; // stale reply from a window that is already being retried
            if (out->peer_offline_since == 0)
                out->peer_offline_since = Now();
            out->peer_retry_at = Now() + PEER_RETRY_MS;
            out->in_flight.clear();
            out->next_index = out->resume_from;
            out->last_ack = Now();
            return;
        }
        // any other rejection is final
        FailSend(file_id, serverErrorText(error));
        return;
    }
    if (!out->in_flight.remove(index))
        return; // duplicate reply; the index has already been counted

    out->MarkAcked(index);
    out->last_ack = Now();
    out->peer_offline_since = 0;
    out->peer_retry_at = 0;
    if (out->acked >= out->total)
    {
        _sending.erase(it);
        out->file.close();
        emit sig_send_finished(file_id);
        return;
    }

    emit sig_send_progress(file_id, out->acked * 100 / out->total);
    Pump(*out);
}

void FileTransferMgr::slot_file_chunk(const QJsonObject& obj)
{
    const QString file_id = obj["fileid"].toString();
    const QString name = SanitizeFileName(obj["filename"].toString());
    const int from_uid = obj["fromuid"].toInt();
    const int to_uid = obj["touid"].toInt();
    const qint64 size = static_cast<qint64>(obj["filesize"].toDouble());
    const int index = obj["index"].toInt(-1);
    const int total = obj["total"].toInt(-1);

    if (file_id.isEmpty() || name.isEmpty() || from_uid <= 0 || size <= 0 || size > MAX_FILE_BYTES ||
        total != ChunkCount(size) || index < 0 || index >= total)
    {
        qDebug() << "drop malformed file chunk" << file_id << index;
        return;
    }

    auto it = _receiving.find(file_id);
    if (it == _receiving.end())
    {
        auto opened = OpenIncoming(file_id, from_uid, to_uid, name, size, total);
        if (!opened)
        {
            emit sig_recv_failed(file_id, QStringLiteral("无法保存文件"));
            return;
        }
        it = _receiving.insert(file_id, opened);
        emit sig_recv_started(file_id, from_uid, name, size);
    }

    auto inc = it.value();
    if (inc->size != size || inc->total != total)
    {
        FailRecv(file_id, QStringLiteral("文件信息不一致"));
        return;
    }
    if (inc->arrived.contains(index))
    {
        // still a sign of life: a peer that only resends duplicates is not stalled
        inc->last_chunk = Now();
        return;
    }

    QByteArray raw = QByteArray::fromBase64(obj["data"].toString().toLatin1());
    if (raw.size() != ChunkLength(size, index))
    {
        FailRecv(file_id, QStringLiteral("文件数据损坏"));
        return;
    }
    if (!inc->file.seek(index * static_cast<qint64>(FILE_CHUNK_BYTES)) ||
        inc->file.write(raw) != raw.size())
    {
        FailRecv(file_id, QStringLiteral("写入文件失败"));
        return;
    }

    inc->arrived.insert(index);
    ++inc->received;
    inc->last_chunk = Now();
    if (inc->received >= inc->total)
    {
        FinishRecv(file_id);
        return;
    }
    emit sig_recv_progress(file_id, inc->received * 100 / inc->total);
}

void FileTransferMgr::slot_check_timeouts()
{
    if (_suspended)
        return;

    const qint64 now = Now();

    QStringList dead_send;
    QStringList retry_send;
    for (auto it = _sending.begin(); it != _sending.end(); ++it)
    {
        auto out = it.value();
        if (out->peer_offline_since != 0)
        {
            if (now - out->peer_offline_since > PEER_GRACE_MS)
                dead_send.append(it.key());
            else if (now >= out->peer_retry_at)
            {
                out->peer_retry_at = now + PEER_RETRY_MS;
                retry_send.append(it.key());
            }
            // the peer being away is not a stalled socket: only the grace window applies
            continue;
        }
        if (now - out->last_ack > STALL_MS)
            dead_send.append(it.key());
    }
    for (const auto& id : dead_send)
    {
        auto it = _sending.find(id);
        const bool peer_away = it != _sending.end() && it.value()->peer_offline_since != 0;
        FailSend(id, peer_away ? QStringLiteral("对方不在线，文件未发送")
                               : QStringLiteral("传输超时"));
    }
    for (const auto& id : retry_send)
    {
        auto it = _sending.find(id);
        if (it == _sending.end())
            continue;
        auto out = it.value();
        out->in_flight.clear();
        out->next_index = out->resume_from;
        Pump(*out);
    }

    QStringList dead_recv;
    for (auto it = _receiving.begin(); it != _receiving.end(); ++it)
    {
        // give a reconnecting sender the same window the send side gives a peer
        if (now - it.value()->last_chunk > PEER_GRACE_MS)
            dead_recv.append(it.key());
    }
    for (const auto& id : dead_recv)
        FailRecv(id, QStringLiteral("传输超时"));
}

void FileTransferMgr::slot_connection_closed()
{
    // an automatic reconnect keeps the session and both partial files; only a final
    // close (logout, kick, retries exhausted) tears the transfers down
    if (TcpMgr::GetInstance()->IsReconnecting())
    {
        PauseAll();
        return;
    }
    FailAll(QStringLiteral("连接已断开"));
}

void FileTransferMgr::slot_reconnected()
{
    ResumeAll();
}

void FileTransferMgr::slot_reconnect_failed()
{
    FailAll(QStringLiteral("连接已断开"));
}

void FileTransferMgr::PauseAll()
{
    _suspended = true;
    const qint64 now = Now();
    for (auto& out : _sending)
    {
        // every in-flight frame died with the socket: resend from the first unacked chunk
        out->in_flight.clear();
        out->next_index = out->resume_from;
        out->peer_offline_since = 0;
        out->peer_retry_at = 0;
        out->last_ack = now;
    }
    for (auto& inc : _receiving)
        inc->last_chunk = now;
}

void FileTransferMgr::ResumeAll()
{
    _suspended = false;
    const qint64 now = Now();
    for (auto& inc : _receiving)
        inc->last_chunk = now;
    // Pump may fail and erase an entry, so take the ids first
    const QStringList ids = _sending.keys();
    for (const auto& id : ids)
    {
        auto it = _sending.find(id);
        if (it == _sending.end())
            continue;
        auto out = it.value();
        out->last_ack = now;
        out->peer_offline_since = 0;
        out->peer_retry_at = 0;
        Pump(*out);
    }
}

void FileTransferMgr::FailAll(const QString& reason)
{
    _suspended = false;
    const QStringList send_ids = _sending.keys();
    for (const auto& id : send_ids)
        FailSend(id, reason);
    const QStringList recv_ids = _receiving.keys();
    for (const auto& id : recv_ids)
        FailRecv(id, reason);
}

void FileTransferMgr::FailSend(const QString& file_id, const QString& reason)
{
    auto it = _sending.find(file_id);
    if (it == _sending.end())
        return;
    auto out = it.value();
    _sending.erase(it);
    out->file.close();
    emit sig_send_failed(file_id, reason);
}

void FileTransferMgr::FailRecv(const QString& file_id, const QString& reason)
{
    auto it = _receiving.find(file_id);
    if (it == _receiving.end())
        return;
    auto inc = it.value();
    _receiving.erase(it);
    inc->file.close();
    // a partial file is of no use to anyone and would shadow the next transfer's name
    QFile::remove(inc->file.fileName());
    emit sig_recv_failed(file_id, reason);
}

std::shared_ptr<IncomingFile> FileTransferMgr::OpenIncoming(const QString& file_id, int from_uid,
                                                            int to_uid, const QString& name, qint64 size,
                                                            int total)
{
    const QString dir = DefaultSaveDir();
    if (!QDir().mkpath(dir))
        return nullptr;

    auto inc = std::make_shared<IncomingFile>();
    inc->file_id = file_id;
    inc->from_uid = from_uid;
    inc->to_uid = to_uid;
    inc->name = name;
    inc->size = size;
    inc->total = total;
    inc->path = UniquePath(dir, name);
    inc->last_chunk = Now();
    inc->file.setFileName(inc->path + ".part");
    // ReadWrite at a fresh path: sizes are only known once the last chunk lands
    if (!inc->file.open(QIODevice::ReadWrite | QIODevice::Truncate))
        return nullptr;
    return inc;
}

void FileTransferMgr::FinishRecv(const QString& file_id)
{
    auto it = _receiving.find(file_id);
    if (it == _receiving.end())
        return;
    auto inc = it.value();
    _receiving.erase(it);
    inc->file.close();

    const QString part = inc->file.fileName();
    if (!QFile::rename(part, inc->path))
    {
        QFile::remove(part);
        emit sig_recv_failed(file_id, QStringLiteral("无法保存文件"));
        return;
    }
    emit sig_recv_finished(file_id, inc->from_uid, inc->name, inc->path);
}

bool FileTransferMgr::IsPeerUnavailableError(int error)
{
    // UidInvalid: the node has no live session for the receiver (offline, or its node is
    // restarting and has not accepted the login yet). RPCFailed: the node could not reach its
    // peer to relay the chunk, which current servers fold into UidInvalid but older ones
    // report raw. Both are transient and the chunk is still held.
    return error == 1011 || error == 1002;
}

QString FileTransferMgr::SanitizeFileName(const QString& name)
{
    // a name may carry a directory (or a Windows drive) on the way in; keep only the leaf
    QString leaf = name;
    leaf.replace('\\', '/');
    leaf = leaf.section('/', -1);
    if (leaf == "." || leaf == "..")
        return QString();

    // characters the server rejects (and that would be illegal on Windows) become '_' so a
    // local file can still be sent
    QString cleaned;
    cleaned.reserve(leaf.size());
    for (QChar c : leaf)
    {
        cleaned.append(IsBadNameChar(c.unicode()) ? QChar('_') : c);
    }
    cleaned = cleaned.trimmed();
    if (cleaned.isEmpty())
        return QString();

    if (cleaned.toUtf8().size() > MAX_FILE_NAME_BYTES)
    {
        // keep the extension; a file without one still opens either way
        const int dot = cleaned.lastIndexOf('.');
        const QString ext = (dot > 0 && cleaned.size() - dot <= 16) ? cleaned.mid(dot) : QString();
        QString stem = ext.isEmpty() ? cleaned : cleaned.left(cleaned.size() - ext.size());
        const int ext_bytes = ext.toUtf8().size();
        while (!stem.isEmpty() && stem.toUtf8().size() + ext_bytes > MAX_FILE_NAME_BYTES)
            stem.chop(1);
        cleaned = stem + ext;
    }
    return cleaned;
}

int FileTransferMgr::ChunkCount(qint64 size)
{
    if (size <= 0)
        return 0;
    return static_cast<int>((size + FILE_CHUNK_BYTES - 1) / FILE_CHUNK_BYTES);
}

qint64 FileTransferMgr::ChunkLength(qint64 size, int index)
{
    if (size <= 0 || index < 0 || index >= ChunkCount(size))
        return 0;
    const qint64 offset = static_cast<qint64>(index) * FILE_CHUNK_BYTES;
    return qMin<qint64>(FILE_CHUNK_BYTES, size - offset);
}

QString FileTransferMgr::DefaultSaveDir()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (base.isEmpty())
        base = QDir::homePath();
    return base + "/WeChat/files";
}

QString FileTransferMgr::UniquePath(const QString& dir, const QString& name)
{
    const QDir d(dir);
    const auto free_path = [](const QString& candidate) {
        // the .part counts as taken: a second transfer must not clobber one in progress
        return !QFileInfo::exists(candidate) && !QFileInfo::exists(candidate + ".part");
    };

    const QString first = d.filePath(name);
    if (free_path(first))
        return first;

    const int dot = name.lastIndexOf('.');
    const QString stem = (dot > 0 && dot < name.size() - 1) ? name.left(dot) : name;
    const QString ext = (dot > 0 && dot < name.size() - 1) ? name.mid(dot) : QString();
    for (int n = 1; n < 1000; ++n)
    {
        const QString candidate = d.filePath(QStringLiteral("%1 (%2)%3").arg(stem).arg(n).arg(ext));
        if (free_path(candidate))
            return candidate;
    }
    return d.filePath(QUuid::createUuid().toString(QUuid::WithoutBraces) + ext);
}
