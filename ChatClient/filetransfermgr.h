#ifndef FILETRANSFERMGR_H
#define FILETRANSFERMGR_H

#include <QByteArray>
#include <QFile>
#include <QHash>
#include <QJsonObject>
#include <QSet>
#include <QString>
#include <QTimer>
#include <memory>

#include "global.h"
#include "singleton.h"

// The server relays every chunk and stores nothing, so a whole transfer lives in these two
// maps. The sender slices its file and keeps a bounded window of chunks in flight; the
// receiver writes each chunk at its own offset and only renames the file once every index
// has arrived.
struct OutgoingFile
{
    QString file_id;
    int to_uid = 0;
    QString name;
    qint64 size = 0;
    int total = 0;
    int next_index = 0; // the next chunk still to hand to the socket
    int acked = 0;
    QFile file;
    QSet<int> in_flight;
    qint64 last_ack = 0;
};

struct IncomingFile
{
    QString file_id;
    int from_uid = 0;
    int to_uid = 0;
    QString name;
    QString path; // final location, chosen on the first chunk
    qint64 size = 0;
    int total = 0;
    int received = 0;
    QFile file;
    QSet<int> arrived;
    qint64 last_chunk = 0;
};

class FileTransferMgr : public QObject, public Singleton<FileTransferMgr>
{
    Q_OBJECT
public:
    friend class Singleton<FileTransferMgr>;
    ~FileTransferMgr();

    // Returns the id the transfer is tracked under, or an empty string when the file was
    // rejected before a byte went out.
    QString SendFile(int to_uid, const QString& path);

    static QString SanitizeFileName(const QString& name);
    static int ChunkCount(qint64 size);
    static qint64 ChunkLength(qint64 size, int index);
    static QString DefaultSaveDir();

public slots:
    void slot_file_rsp(int error, const QString& file_id, int index);
    void slot_file_chunk(const QJsonObject& obj);
    void slot_check_timeouts();
    void slot_connection_closed();

signals:
    void sig_send_progress(QString file_id, int percent);
    void sig_send_finished(QString file_id);
    void sig_send_failed(QString file_id, QString reason);
    void sig_recv_started(QString file_id, int from_uid, QString name, qint64 size);
    void sig_recv_progress(QString file_id, int percent);
    void sig_recv_finished(QString file_id, int from_uid, QString name, QString path);
    void sig_recv_failed(QString file_id, QString reason);

private:
    FileTransferMgr();
    void Pump(OutgoingFile& out);
    void FailSend(const QString& file_id, const QString& reason);
    void FailRecv(const QString& file_id, const QString& reason);
    std::shared_ptr<IncomingFile> OpenIncoming(const QString& file_id, int from_uid, int to_uid,
                                               const QString& name, qint64 size, int total);
    void FinishRecv(const QString& file_id);
    static QString UniquePath(const QString& dir, const QString& name);

    QHash<QString, std::shared_ptr<OutgoingFile>> _sending;
    QHash<QString, std::shared_ptr<IncomingFile>> _receiving;
    QTimer _watchdog;
};

#endif // FILETRANSFERMGR_H
