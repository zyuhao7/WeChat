#include "logindialog.h"
#include "ui_logindialog.h"
#include "httpmgr.h"
#include  "mainwindow.h"
#include "tcpmgr.h"
#include <QDebug>
#include<QPainter>
#include <QPainterPath>
#include <QRegularExpression>


LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    ui->pass_edit->setEchoMode(QLineEdit::Password);
    connect(ui->reg_btn, &QPushButton::clicked, this, &LoginDialog::switchRegister);

    ui->forget_label->SetState("normal","hover","","selected","selected_hover","");
    ui->forget_label->setCursor(Qt::PointingHandCursor);
    connect(ui->forget_label, &ClickedLabel::clicked, this, &LoginDialog::slot_forget_pwd);

    initHttpHandlers();

    //connect the login reply signal
       connect(Httpmgr::GetInstance().get(), &Httpmgr::sig_login_mod_finish, this,
               &LoginDialog::slot_login_mod_finish);

       //connect the tcp connect-request signal and slot
     connect(this, &LoginDialog::sig_connect_tcp, TcpMgr::GetInstance().get(), &TcpMgr::slot_tcp_connect);
     //connect the connect-success signal from the tcp manager
     connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_con_success, this, &LoginDialog::slot_tcp_con_finish);
     //connect the login-failure signal from the tcp manager
     connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_login_failed, this, &LoginDialog::slot_login_failed);
     initHead();
}

LoginDialog::~LoginDialog()
{
    qDebug()<<"destruct LoginDlg";
    delete ui;
}

void LoginDialog::initHttpHandlers()
{
    //register the login-reply logic
      _handlers.insert(ReqId::ID_LOGIN_USER, [this](QJsonObject jsonObj){
          int error = jsonObj["error"].toInt();
          if(error != ErrorCodes::SUCCESS){
              showTip(tr("参数错误"),false);
              enableBtn(true);
              return;
          }
          auto email = jsonObj["email"].toString();

          //emit a signal to tell tcpMgr to open the long connection
          ServerInfo si;
          si.Uid = jsonObj["uid"].toInt();
          si.Host = jsonObj["host"].toString();
          si.Port = jsonObj["port"].toString();
          si.Token = jsonObj["token"].toString();

          _uid = si.Uid;
          _token = si.Token;
          qDebug()<< "email is " << email << " uid is " << si.Uid <<" host is "
                  << si.Host << " Port is " << si.Port << " Token is " << si.Token;
          emit sig_connect_tcp(si);
      });
}

void LoginDialog::initHead()
{
    // load the image
        QPixmap originalPixmap(":/res/ice.png");
          // set the image to auto-scale
        qDebug()<< originalPixmap.size() << ui->head_label->size();
        originalPixmap = originalPixmap.scaled(ui->head_label->size(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // create a QPixmap the same size as the original, used to draw the rounded image
        QPixmap roundedPixmap(originalPixmap.size());
        roundedPixmap.fill(Qt::transparent); // fill with a transparent color

        QPainter painter(&roundedPixmap);
        painter.setRenderHint(QPainter::Antialiasing); // enable antialiasing for smoother rounded corners
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        // use QPainterPath to set rounded corners
        QPainterPath path;
        path.addRoundedRect(0, 0, originalPixmap.width(), originalPixmap.height(), 10, 10); // the last two params are the x and y corner radii
        painter.setClipPath(path);

        // draw the original image onto roundedPixmap
        painter.drawPixmap(0, 0, originalPixmap);

        // set the drawn rounded image on the QLabel
        ui->head_label->setPixmap(roundedPixmap);

}

bool LoginDialog::checkUserValid()
{
    auto email = ui->email_edit->text();
      if(email.isEmpty()){
          qDebug() << "email empty " ;
          AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱不能为空"));
          return false;
      }
      DelTipErr(TipErr::TIP_EMAIL_ERR);
      return true;
}

bool LoginDialog::checkPwdValid()
{
    auto pwd = ui->pass_edit->text();
       if(pwd.length() < 6 || pwd.length() > 15){
           qDebug() << "Pass length invalid";
           //warn that the length is invalid
           AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
           return false;
       }

       // create a regex object for the password rules above
       // explanation of this regex:
       // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ password at least 6 chars, letters, digits and some special chars
       QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");
       bool match = regExp.match(pwd).hasMatch();
       if(!match){
           //warn that the characters are invalid
           AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符且长度为(6~15)"));
           return false;;
       }

       DelTipErr(TipErr::TIP_PWD_ERR);

       return true;
}

void LoginDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void LoginDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
        ui->err_tip->clear();
        return;
    }
}

void LoginDialog::showTip(QString str, bool b_ok)
{
    if(ui->err_tip->setProperty("state","err") == b_ok)
    {
       ui->err_tip->setProperty("state","normal");
    }
    else
    {
        ui->err_tip->setProperty("state","err");
    }
    ui->err_tip->setText(str);

    repolish(ui->err_tip);
}

bool LoginDialog::enableBtn(bool enabled)
{
    ui->login_btn->setEnabled(enabled);
    ui->reg_btn->setEnabled(enabled);
        return true;
}




void LoginDialog::slot_forget_pwd()
{
    qDebug()<<"slot forget pwd";
    emit switchReset();
}

void LoginDialog::slot_login_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS){
          showTip(tr("网络请求错误"),false);
          return;
      }

      // parse the JSON string; res must be converted to a QByteArray
      QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
      //JSON parse error
      if(jsonDoc.isNull()){
          showTip(tr("json解析错误"),false);
          return;
      }

      //JSON parse error
      if(!jsonDoc.isObject()){
          showTip(tr("json解析错误"),false);
          return;
      }


      //dispatch to the matching logic by id.
      _handlers[id](jsonDoc.object());

      return;
}

void LoginDialog::on_login_btn_clicked()
{
    qDebug()<<"login btn clicked";
       if(checkUserValid() == false){
           return;
       }

       if(checkPwdValid() == false){
           return ;
       }
//       enableBtn(false);

       auto email = ui->email_edit->text();
       auto pwd = ui->pass_edit->text();
       //send the HTTP login request
       QJsonObject json_obj;
       json_obj["email"] = email;
       json_obj["passwd"] = xorString(pwd);
       Httpmgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_login"),
                                json_obj, ReqId::ID_LOGIN_USER,Modules::LOGINMOD);
}

void LoginDialog::slot_tcp_con_finish(bool bsuccess)
{

   if(bsuccess){
      showTip(tr("聊天服务连接成功，正在登录..."),true);
      QJsonObject jsonObj;
      jsonObj["uid"] = _uid;
      jsonObj["token"] = _token;

      QJsonDocument doc(jsonObj);
      QByteArray jsonData = doc.toJson(QJsonDocument::Indented);

      //send the TCP request to the chat server
      emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_CHAT_LOGIN, jsonData);

   }else{
      showTip(tr("网络异常"),false);
      enableBtn(true);
   }
}

void LoginDialog::slot_login_failed(int err)
{
    QString result = QString("登录失败, err is %1").arg(err);
    showTip(result,false);
    enableBtn(true);
}
