#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "logindialog.h"
#include "registdialog.h"
#include "resetdialog.h"
#include "chatdialog.h"

/******************************************************************************
 *
 * @file       mainwindow.h
 * @brief      主函数
 *
 * @author     沫羽皓
 * @date       2024/12/02
 * @history
 *****************************************************************************/

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

enum UIStatus{
    LOGIN_UI,
    REGISTER_UI,
    RESET_UI,
    CHAT_UI
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void SlotSwitchReg();
    void SlotSwitchLogin();
    void SlotSwitchReset();
    void SlotSwitchLogin2();
    void SlotSwitchChat();
    void SlotOffline();
    void SlotExcepConOffline();
private:
    void offlineLogin();
    Ui::MainWindow *ui;
    LoginDialog* _login_dlg;
    RegistDialog* _reg_dlg;
    ResetDialog* _reset_dlg;
    ChatDialog* _chat_dlg;
    UIStatus _ui_status;
};
#endif // MAINWINDOW_H
