#ifndef REGISTDIALOG_H
#define REGISTDIALOG_H

#include <QDialog>
#include "global.h"
#include <functional>
#include <QMap>
#include <QJsonObject>
#include <QSet>
#include <QString>
#include <QTimer>



namespace Ui {
class RegistDialog;
}

class RegistDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegistDialog(QWidget *parent = nullptr);
    ~RegistDialog();

private slots:
    void on_get_code_clicked();
    void on_confirm_btn_clicked();
    void on_return_btn_clicked();
    void on_cancel_btn_clicked();

public slots:
    void slot_reg_mod_finish(ReqId id,QString res, ErrorCodes err);

private:
    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);
    void ChangeTipPage();
    bool checkUserValid();
    bool checkEmailValid();
    bool checkPassValid();
    bool checkVerifyValid();
    bool checkConfirmValid();

    void initHttpHandlers();
    Ui::RegistDialog *ui;
    void showTip(QString str, bool b_ok);
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
    QMap<TipErr, QString> _tip_errs;
    QTimer *_countdown_timer;
    int _countdown;

signals:
    void sigSwitchLogin();
};



#endif // REGISTDIALOG_H
