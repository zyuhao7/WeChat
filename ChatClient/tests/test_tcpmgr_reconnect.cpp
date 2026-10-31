#include <QDataStream>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

#include "tcpmgr.h"

// Minimal chat-server stand-in: speaks the same frame format and answers the
// login request, so the reconnect state machine can be exercised without the
// full backend stack.
class FakeChatServer : public QObject
{
    Q_OBJECT
public:
    explicit FakeChatServer(QObject* parent = nullptr)
        : QObject(parent)
    {
        QObject::connect(&_server, &QTcpServer::newConnection, this, [this]() {
            while (QTcpSocket* socket = _server.nextPendingConnection())
            {
                _client = socket;
                _buffer.clear();
                QObject::connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                    readFrames(socket);
                });
                QObject::connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
                    if (_client == socket)
                        _client = nullptr;
                    socket->deleteLater();
                });
            }
        });
    }

    bool listen() { return _server.listen(QHostAddress::LocalHost, 0); }

    quint16 port() const { return _server.serverPort(); }

    // Keep listening but cut the current session, the way a crashed server drops sockets.
    void dropClient()
    {
        if (!_client)
            return;
        _client->disconnectFromHost();
    }

    void stopListening()
    {
        _server.close();
        dropClient();
    }

    void setLoginError(int error) { _login_error = error; }

private:
    void readFrames(QTcpSocket* socket)
    {
        _buffer.append(socket->readAll());
        while (_buffer.size() >= 4)
        {
            QDataStream header(_buffer.left(4));
            header.setByteOrder(QDataStream::BigEndian);
            quint16 id = 0;
            quint16 len = 0;
            header >> id >> len;
            if (_buffer.size() < 4 + len)
                return;
            QByteArray body = _buffer.mid(4, len);
            _buffer.remove(0, 4 + len);
            if (id == ID_CHAT_LOGIN)
                sendLoginReply(socket, body);
        }
    }

    void sendLoginReply(QTcpSocket* socket, const QByteArray& body)
    {
        QJsonObject request = QJsonDocument::fromJson(body).object();
        QJsonObject response;
        response["error"] = _login_error;
        response["uid"] = request["uid"].toInt();
        response["token"] = request["token"].toString();
        response["name"] = "reconnect-test";
        response["nick"] = "";
        response["icon"] = "";
        response["sex"] = 0;
        response["desc"] = "";
        response["friend_list"] = QJsonArray();

        QByteArray payload = QJsonDocument(response).toJson(QJsonDocument::Compact);
        QByteArray frame;
        QDataStream out(&frame, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::BigEndian);
        out << quint16(ID_CHAT_LOGIN_RSP) << quint16(payload.size());
        frame.append(payload);
        socket->write(frame);
    }

    QTcpServer _server;
    QTcpSocket* _client = nullptr;
    QByteArray _buffer;
    int _login_error = 0;
};

class TcpMgrReconnectTest : public QObject
{
    Q_OBJECT

private:
    static ServerInfo serverInfo(const FakeChatServer& server)
    {
        ServerInfo info;
        info.Host = "127.0.0.1";
        info.Port = QString::number(server.port());
        info.Token = "test-token";
        info.Uid = 42;
        return info;
    }

    void signIn(FakeChatServer& server)
    {
        TcpMgr* tcp = TcpMgr::GetInstance().get();
        QSignalSpy conResult(tcp, &TcpMgr::sig_con_success);
        QSignalSpy switched(tcp, &TcpMgr::sig_swich_chatdlg);
        tcp->slot_tcp_connect(serverInfo(server));
        QVERIFY(conResult.wait(5000));
        QCOMPARE(conResult.at(0).at(0).toBool(), true);

        // LoginDialog normally sends this after the socket is up.
        QJsonObject request;
        request["uid"] = 42;
        request["token"] = "test-token";
        tcp->slot_send_data(ID_CHAT_LOGIN, QJsonDocument(request).toJson(QJsonDocument::Compact));
        QVERIFY(switched.wait(5000));
    }

private slots:
    void backoffDoublesUntilTheCap()
    {
        QCOMPARE(TcpReconnect::delayMs(1), 1000);
        QCOMPARE(TcpReconnect::delayMs(2), 2000);
        QCOMPARE(TcpReconnect::delayMs(3), 4000);
        QCOMPARE(TcpReconnect::delayMs(4), 8000);
        QCOMPARE(TcpReconnect::delayMs(5), 16000);
        QCOMPARE(TcpReconnect::delayMs(6), 30000);
    }

    void backoffNeverExceedsTheCap()
    {
        for (int attempt = 7; attempt < 100; ++attempt)
            QCOMPARE(TcpReconnect::delayMs(attempt), TcpReconnect::kMaxDelayMs);
    }

    void nonPositiveAttemptsHaveNoDelay()
    {
        QCOMPARE(TcpReconnect::delayMs(0), 0);
        QCOMPARE(TcpReconnect::delayMs(-3), 0);
    }

    void unexpectedDropReconnectsAndResumesTheSession()
    {
        FakeChatServer server;
        QVERIFY(server.listen());
        signIn(server);

        TcpMgr* tcp = TcpMgr::GetInstance().get();
        QSignalSpy reconnecting(tcp, &TcpMgr::sig_reconnecting);
        QSignalSpy reconnected(tcp, &TcpMgr::sig_reconnected);

        server.dropClient();

        QVERIFY(reconnecting.wait(3000));
        QCOMPARE(reconnecting.at(0).at(0).toInt(), 1);
        QCOMPARE(reconnecting.at(0).at(1).toInt(), 1000);
        QVERIFY(reconnected.wait(5000));
    }

    void rejectedReloginFallsBackToTheLoginPage()
    {
        FakeChatServer server;
        QVERIFY(server.listen());
        signIn(server);

        server.setLoginError(1);
        QSignalSpy failed(TcpMgr::GetInstance().get(), &TcpMgr::sig_reconnect_failed);
        server.dropClient();

        QVERIFY(failed.wait(5000));
    }

    void givesUpAfterTheLastAttempt()
    {
        FakeChatServer server;
        QVERIFY(server.listen());
        signIn(server);

        TcpMgr* tcp = TcpMgr::GetInstance().get();
        QSignalSpy reconnecting(tcp, &TcpMgr::sig_reconnecting);
        QSignalSpy failed(tcp, &TcpMgr::sig_reconnect_failed);

        server.stopListening();

        // 1+2+4+8+16+30+30+30 seconds, then the ninth failure ends the loop.
        QVERIFY(failed.wait(150000));
        QCOMPARE(reconnecting.count(), TcpReconnect::kMaxAttempts);
        QCOMPARE(failed.count(), 1);
    }
};

QObject* createTcpMgrReconnectTest()
{
    return new TcpMgrReconnectTest;
}

#include "test_tcpmgr_reconnect.moc"
