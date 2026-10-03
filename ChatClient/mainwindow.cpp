#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "resetdialog.h"
#include "chatdialog.h"
#include "tcpmgr.h"
#include <QLayout>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    _ui_status = LOGIN_UI;
    ui->setupUi(this);

    // create a central widget
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    _login_dlg->show();
    setCentralWidget(_login_dlg);

    //connect the login page's register signal
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //connect the login page's forgot-password signal
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    //connect the create-chat-page signal
    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_swich_chatdlg, this, &MainWindow::SlotSwitchChat);
    //connect the server kick message
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_notify_offline,this, &MainWindow::SlotOffline);
    //connect the server-disconnect / heartbeat-timeout / exception connection info
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_connection_closed, this, &MainWindow::SlotExcepConOffline);

    //for testing
    //emit TcpMgr::GetInstance()->sig_swich_chatdlg();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SlotSwitchReg()
{
    _reg_dlg= new RegistDialog(this);
    _reg_dlg->hide(); // prevent UI flicker

    _reg_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);

    //connect the register page's return-to-login signal
    connect(_reg_dlg, &RegistDialog::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);
    setCentralWidget(_reg_dlg);

    _login_dlg->hide();
    _reg_dlg->show();
    _ui_status = REGISTER_UI;
}

void MainWindow::SlotSwitchLogin()
{
    //create a central widget and set it as MainWindow's central widget
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

    _reg_dlg->hide();
    _login_dlg->show();

    //connect the login page's register signal
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);

    //connect the login page's forgot-password signal
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);

    _ui_status = LOGIN_UI;
}

void MainWindow::SlotSwitchReset()
{
    _ui_status = RESET_UI;

    //create a central widget and set it as MainWindow's central widget
    _reset_dlg = new ResetDialog(this);
    _reset_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_reset_dlg);

    _login_dlg->hide();
    _reset_dlg->show();
    //register the login-return signal and slot
    connect(_reset_dlg, &ResetDialog::switchLogin, this, &MainWindow::SlotSwitchLogin2);
}

void MainWindow::SlotSwitchLogin2()
{
    //create a central widget and set it as MainWindow's central widget
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

    _reset_dlg->hide();
    _login_dlg->show();
    //connect the login page's forgot-password signal
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    //connect the login page's register signal
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    _ui_status = LOGIN_UI;
}

void MainWindow::SlotSwitchChat()
{
    _chat_dlg = new ChatDialog();
    _chat_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_chat_dlg);
    _chat_dlg->show();
    _login_dlg->hide();
    this->setMinimumSize(QSize(1050,900));
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    _ui_status = CHAT_UI;
}

void MainWindow::SlotOffline()
{
    // pop up the message box via a static method
    QMessageBox::information(this, "下线提醒", "同账号异地登录, 该客户端下线!");
    // TcpMgr::GetInstance()->CloseConnection();
    offlineLogin();
}

void MainWindow::SlotExcepConOffline()
{
    // pop up the message box via a static method
    QMessageBox::information(this, "下线提醒", "心跳超时/临界异常, 该客户端下线!");
    TcpMgr::GetInstance()->CloseConnection();
    offlineLogin();
}

void MainWindow::offlineLogin()
{
    if(_ui_status == LOGIN_UI)
    {
        return; // if the UI has returned to the login screen, do nothing
    }
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

    _chat_dlg->hide();
    this->setMaximumSize(300, 500);
    this->setMinimumSize(300, 500);
    this->resize(300, 500);
    _login_dlg->show();

    // connect the login page's register and forgot-password signals
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    _ui_status = LOGIN_UI;
}

