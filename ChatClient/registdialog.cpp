#include "registdialog.h"
#include "ui_registdialog.h"
#include "global.h"
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QRandomGenerator>
#include "httpmgr.h"


RegistDialog::RegistDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegistDialog),
    _countdown(5)
{
    ui->setupUi(this);
    ui->pass_edit->setEchoMode(QLineEdit::Password);
    ui->confirm_edit->setEchoMode(QLineEdit::Password);
    ui->err_tip->setProperty("state","normal");
    repolish(ui->err_tip);
    connect(Httpmgr::GetInstance().get(),&Httpmgr::sig_reg_mod_finish,
            this, &RegistDialog::slot_reg_mod_finish);

    initHttpHandlers();
    ui->err_tip->clear();

    connect(ui->user_edit,&QLineEdit::editingFinished,this,[this](){
        checkUserValid();
    });

    connect(ui->email_edit, &QLineEdit::editingFinished, this, [this](){
        checkEmailValid();
    });

    connect(ui->pass_edit, &QLineEdit::editingFinished, this, [this](){
        checkPassValid();
    });

    connect(ui->confirm_edit, &QLineEdit::editingFinished, this, [this](){
        checkConfirmValid();
    });

    connect(ui->verify_edit, &QLineEdit::editingFinished, this, [this](){
            checkVerifyValid();
    });

    ui->pass_visible->setCursor(Qt::PointingHandCursor);
    ui->confirm_visible->setCursor(Qt::PointingHandCursor);

    ui->pass_visible->SetState("unvisible","unvisible_hover","","visible",
                                "visible_hover","");

    ui->confirm_visible->SetState("unvisible","unvisible_hover","","visible",
                                    "visible_hover","");

    ui->pass_visible->SetIcons(ElaIconType::EyeSlash, ElaIconType::Eye);
    ui->confirm_visible->SetIcons(ElaIconType::EyeSlash, ElaIconType::Eye);

    //connect the click event

    connect(ui->pass_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->pass_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->pass_edit->setEchoMode(QLineEdit::Password);
        }else{
                ui->pass_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";
    });

    connect(ui->confirm_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->confirm_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->confirm_edit->setEchoMode(QLineEdit::Password);
        }else{
                ui->confirm_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";
    });

    // create the timer
    _countdown_timer = new QTimer(this);

    // connect signal and slot
    connect(_countdown_timer, &QTimer::timeout, [this](){
        if(_countdown==0){
            _countdown_timer->stop();
            emit sigSwitchLogin();
            return;
        }
        _countdown--;
        auto str = QString("注册成功，%1 s后返回登录").arg(_countdown);
        ui->tip_lb->setText(str);
    });
}

RegistDialog::~RegistDialog()
{
    qDebug()<<"destruct RegistDialog";
    delete ui;
}

void RegistDialog::on_get_code_clicked()
{
    auto email = ui->email_edit->text();
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch();

    if(match)
    {
        // send the HTTP verify code
        QJsonObject json_obj;
        json_obj["email"] = email;
        Httpmgr::GetInstance()->PostHttpReq(QUrl( gate_url_prefix +"/get_verify_code"),
                                            json_obj,ReqId::ID_GET_VERIFY_CODE,Modules::REGISTERMOD);
    }
    else
    {
        showTip(tr("邮箱地址不正确"),false);
    }
}

void RegistDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS)
    {
        showTip(tr("网络请求出错"), false);
        return;
    }

    // parse the JSON string; convert res to a QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if(jsonDoc.isNull())
    {
        showTip(tr("json 解析失败"), false);
        return;
    }

    // JSON parse error
    if(!jsonDoc.isObject())
    {
        showTip(tr("json 解析失败"), false);
        return;
    }

    _handlers[id](jsonDoc.object());
    return;
}
void RegistDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void RegistDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
      ui->err_tip->clear();
      return;
    }

    showTip(_tip_errs.first(), false);
}

void RegistDialog::ChangeTipPage()
{
    _countdown_timer->stop();
     ui->stackedWidget->setCurrentWidget(ui->page_2);

   // start the timer with a 1000 ms (1s) interval
   _countdown_timer->start(1000);
}

bool RegistDialog::checkUserValid()
{
    if(ui->user_edit->text() == ""){
          AddTipErr(TipErr::TIP_USER_ERR, tr("用户名不能为空"));
          return false;
      }

      DelTipErr(TipErr::TIP_USER_ERR);
      return true;
}

bool RegistDialog::checkEmailValid()
{
    //regex for validating the email address
    auto email = ui->email_edit->text();
    // regex for the email address
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch(); // run the regex match
    if(!match){
        //warn that the email is incorrect
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱地址不正确"));
        return false;
    }

    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool RegistDialog::checkPassValid()
{
    auto pass = ui->pass_edit->text();

    if(pass.length() < 6 || pass.length()>15){
        //warn that the length is invalid
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }

    // create a regex object for the password rules above
    // explanation of this regex:
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ password at least 6 chars, letters, digits and some special chars
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(pass).hasMatch();
    if(!match){
        //warn that the characters are invalid
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符"));
        return false;
    }

    DelTipErr(TipErr::TIP_PWD_ERR);

    return true;
}

bool RegistDialog::checkVerifyValid()
{
    auto pass = ui->verify_edit->text();
    if(pass.isEmpty()){
        AddTipErr(TipErr::TIP_VERIFY_ERR, tr("验证码不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_VERIFY_ERR);
    return true;
}

bool RegistDialog::checkConfirmValid()
{
    auto pass = ui->pass_edit->text();
       auto confirm = ui->confirm_edit->text();

       if(confirm.length() < 6 || confirm.length() > 15 ){
           //warn that the length is invalid
           AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("密码长度应为6~15"));
           return false;
       }

       // create a regex object for the password rules above
       // explanation of this regex:
       // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ password at least 6 chars, letters, digits and some special chars
       QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");
       bool match = regExp.match(confirm).hasMatch();
       if(!match){
           //warn that the characters are invalid
           AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("不能包含非法字符"));
           return false;
       }

       DelTipErr(TipErr::TIP_CONFIRM_ERR);

       if(pass != confirm){
           //warn that the passwords do not match
           AddTipErr(TipErr::TIP_PWD_CONFIRM, tr("确认密码和密码不匹配"));
           return false;
       }else{
          DelTipErr(TipErr::TIP_PWD_CONFIRM);
       }
       return true;
}



void RegistDialog::initHttpHandlers()
{
    // register: get-verify-code reply logic
    _handlers.insert(ReqId::ID_GET_VERIFY_CODE,[this](const QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS)
        {
            showTip(tr("参数错误"), false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已经发送到邮箱, 注意查收!"), true);
        qDebug()<<"email is "<<email;
    });

    //register: register-user reply logic
    _handlers.insert(ReqId::ID_REG_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("用户注册成功"), true);
        qDebug()<< "email is " << email ;
        qDebug()<< "user uid is " <<  jsonObj["uid"].toString();
        ChangeTipPage();
    });
}



void RegistDialog::showTip(QString str, bool b_ok)
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

void RegistDialog::on_confirm_btn_clicked()
{
    if(ui->user_edit->text() == ""){
            showTip(tr("用户名不能为空"), false);
            return;
        }

        if(ui->email_edit->text() == ""){
            showTip(tr("邮箱不能为空"), false);
            return;
        }

        if(ui->pass_edit->text() == ""){
            showTip(tr("密码不能为空"), false);
            return;
        }

        if(ui->confirm_edit->text() == ""){
            showTip(tr("确认密码不能为空"), false);
            return;
        }

        if(ui->confirm_edit->text() != ui->pass_edit->text()){
            showTip(tr("密码和确认密码不匹配"), false);
            return;
        }

        if(ui->verify_edit->text() == ""){
            showTip(tr("验证码不能为空"), false);
            return;
        }

        QJsonObject json_obj;
        json_obj["user"] = ui->user_edit->text();
        json_obj["email"] = ui->email_edit->text();
        json_obj["passwd"] = xorString(ui->pass_edit->text());
        json_obj["sex"] = 0;

        int randomValue = QRandomGenerator::global()->bounded(100); // generate a random integer between 0 and 99
        int head_i = randomValue % heads.size();

        json_obj["icon"] = heads[head_i];
        json_obj["nick"] = ui->user_edit->text();
        json_obj["confirm"] = xorString(ui->confirm_edit->text());
        json_obj["verifycode"] = ui->verify_edit->text();

        Httpmgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_register"),
                     json_obj, ReqId::ID_REG_USER,Modules::REGISTERMOD);
}

void RegistDialog::on_return_btn_clicked()
{
    _countdown_timer->stop();
    emit sigSwitchLogin();
}

void RegistDialog::on_cancel_btn_clicked()
{
    _countdown_timer->stop();
    emit sigSwitchLogin();
}
