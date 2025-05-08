#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "global.h"
namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

private:
    void initHttpHandlers();
    void initHead();
    bool checkUserValid();
    bool checkPwdValid();
    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);
    void showTip(QString str, bool b_ok);
    bool enableBtn(bool enabled);
    QMap<TipErr, QString> _tip_errs;
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
    Ui::LoginDialog *ui;
    int _uid;
    QString _token;

public slots:
    void slot_forget_pwd();
    void slot_login_mod_finish(ReqId id,QString res, ErrorCodes err);

signals:
   void switchRegister();
   void switchReset();
   void sig_connect_tcp(ServerInfo);

private slots:
   void on_login_btn_clicked();
   void slot_tcp_con_finish(bool bsuccess);
   void slot_login_failed(int err);
};

#endif // LOGINDIALOG_H
