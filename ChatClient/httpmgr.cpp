#include "httpmgr.h"

Httpmgr::~Httpmgr()
{

}

Httpmgr::Httpmgr()
{
    connect(this,&Httpmgr::sig_http_finish,this, &Httpmgr::slot_http_finish);
}

void Httpmgr::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod)
{
    if(mod == Modules::REGISTERMOD){
           //emit a signal to notify the given module that the HTTP response finished
           emit sig_reg_mod_finish(id, res, err);
       }
    if(mod == Modules::RESETMOD)
        // emit a signal to notify the given module that the HTTP response finished
        emit sig_reset_mod_finish(id, res, err);

    if(mod == Modules::LOGINMOD)
        emit sig_login_mod_finish(id, res, err);
}

void Httpmgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod)
{
        //create an HTTP POST request with headers and body
       QByteArray data = QJsonDocument(json).toJson();

       //build the request from the url
       QNetworkRequest request(url);
       request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
       request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));

       //send the request and handle the response; get its own shared_ptr to form a pseudo-closure and raise the refcount
       auto self = shared_from_this();
       QNetworkReply * reply = _manager.post(request, data);

       //connect signal and slot to wait for the send to finish
       QObject::connect(reply, &QNetworkReply::finished, [reply, self, req_id, mod](){
           //handle the error case
           if(reply->error() != QNetworkReply::NoError){
               qDebug() << reply->errorString();
               //emit a signal to notify completion
               emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
               reply->deleteLater();
               return;
           }

           //if no error, read the reply
           QString res = reply->readAll();

           //emit a signal to notify completion
           emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS, mod);
           reply->deleteLater();
           return;
       });
}
