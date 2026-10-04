#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include "ElaWindow.h"
#include "logindialog.h"
#include "registdialog.h"
#include "resetdialog.h"
#include "chatdialog.h"

/******************************************************************************
 *
 * @file       mainwindow.h
 * @brief      main function
 *
 * @author     Moyuhao
 * @date       2024/12/02
 * @history
 *****************************************************************************/

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

enum UIStatus{
    LOGIN_UI,
    REGISTER_UI,
    RESET_UI,
    CHAT_UI
};

// Fluent (ElaWidgetTools) frameless window that hosts the swappable pages.
// Login / register / reset / chat all live as pages in the central custom
// widget so the whole app keeps its original single-window flow.
class MainWindow : public ElaWindow
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
    void setPage(QWidget* page);
    void applyLoginSize();   // fixed 300x500 content
    void applyChatSize();    // resizable 1050x900 content

    QWidget* _page_container;
    QVBoxLayout* _page_layout;

    LoginDialog* _login_dlg;
    RegistDialog* _reg_dlg;
    ResetDialog* _reset_dlg;
    ChatDialog* _chat_dlg;
    UIStatus _ui_status;
};
#endif // MAINWINDOW_H
