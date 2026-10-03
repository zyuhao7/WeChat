#ifndef HTTPMGR_H
#define HTTPMGR_H
#include "singleton.h"
#include<QString>
#include<QUrl>
#include<QObject>
#include<QNetworkAccessManager>
#include <QDebug>
#include<QJsonObject>
#include<QJsonDocument>
#include "global.h"

/******************************************************************************
 *
 * @file       httpmgr.h
 * @brief      XXXX Function
 *
 * @author     Moyuhao
 * @date       2025/02/09
 * @history
 *****************************************************************************/

// CRTP
class Httpmgr : public QObject, public Singleton<Httpmgr>, public std::enable_shared_from_this<Httpmgr>
{
    Q_OBJECT
public:
    ~Httpmgr();
    void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id,  Modules mod);
private:
    friend class Singleton<Httpmgr>;
    Httpmgr();
    QNetworkAccessManager _manager; // Qt native network manager


public slots:
    void slot_http_finish(ReqId id,QString res, ErrorCodes err, Modules mod);

signals:
    void sig_http_finish(ReqId id,QString res, ErrorCodes err, Modules mod);
    void  sig_reg_mod_finish(ReqId id,QString res, ErrorCodes err);
    void sig_reset_mod_finish(ReqId id,QString res, ErrorCodes err);
    void sig_login_mod_finish(ReqId id,QString res, ErrorCodes err);
};

#endif // HTTPMGR_H
