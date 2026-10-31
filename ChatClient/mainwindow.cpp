#include "mainwindow.h"
#include "resetdialog.h"
#include "chatdialog.h"
#include "tcpmgr.h"

#include <QVBoxLayout>
#include <QLayout>
#include <QStackedWidget>
#include "ElaMessageBar.h"

// Title-bar height; the page container lives in the central area below it.
static const int kAppBarHeight = 48;

MainWindow::MainWindow(QWidget* parent)
    : ElaWindow(parent), _backdrop(nullptr), _page_container(nullptr), _page_layout(nullptr), _login_dlg(nullptr), _reg_dlg(nullptr), _reset_dlg(nullptr), _chat_dlg(nullptr)
{
    _ui_status = LOGIN_UI;

    setWindowTitle("Chat");
    setAppBarHeight(kAppBarHeight);
    // The chat page already ships its own left rail, so an extra empty Ela
    // navigation bar would only add noise; keep the Fluent title bar instead.
    setIsNavigationBarEnable(false);
    setWindowButtonFlags(ElaAppBarType::MinimizeButtonHint | ElaAppBarType::CloseButtonHint);

    // container whose single child is swapped between the pages
    _page_container = new QWidget(this);
    _page_container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _page_layout = new QVBoxLayout(_page_container);
    _page_layout->setContentsMargins(0, 0, 0, 0);
    _page_layout->setSpacing(0);
    setCentralCustomWidget(_page_container);
    // ElaCentralStackedWidget::setCustomWidget() drops our container into a
    // vertical layout next to its own (unused here) page stack; both would
    // split the height. Hide that empty sibling so the page fills the area.
    if (QWidget* host = _page_container->parentWidget())
    {
        const auto stacks = host->findChildren<QStackedWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QStackedWidget* stack : stacks)
        {
            stack->hide();
        }
    }

    // Spans the whole window, title bar included, beneath every other child.
    _backdrop = new GlassBackdrop(this);
    _backdrop->setGeometry(rect());
    _backdrop->lower();
    setIsCentralStackedWidgetTransparent(true);

    _login_dlg = new LoginDialog(this);
    setPage(_login_dlg);
    applyLoginSize();

    // connect the login page's register signal
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    // connect the login page's forgot-password signal
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    // connect the create-chat-page signal
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_swich_chatdlg, this, &MainWindow::SlotSwitchChat);
    // connect the server kick message
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_notify_offline, this, &MainWindow::SlotOffline);
    // automatic reconnect after an unexpected drop
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_reconnecting, this, &MainWindow::SlotReconnecting);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_reconnected, this, &MainWindow::SlotReconnected);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_reconnect_failed, this, &MainWindow::SlotReconnectFailed);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setPage(QWidget* page)
{
    if (!page)
    {
        return;
    }
    // drop the previous page without destroying it (callers may reuse it)
    while (_page_layout->count() > 0)
    {
        QLayoutItem* item = _page_layout->takeAt(0);
        if (item->widget())
        {
            item->widget()->hide();
            item->widget()->setParent(nullptr);
        }
        delete item;
    }
    // embedded as a plain child, not as a top-level dialog window
    page->setWindowFlags(Qt::Widget);
    _page_layout->addWidget(page);
    page->show();

    _backdrop->setAnimated(qobject_cast<ChatDialog*>(page) == nullptr);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    ElaWindow::resizeEvent(event);
    _backdrop->setGeometry(rect());
}

void MainWindow::applyLoginSize()
{
    const int w = 300;
    const int h = 500 + kAppBarHeight;
    setIsFixedSize(true);
    setWindowButtonFlags(ElaAppBarType::MinimizeButtonHint | ElaAppBarType::CloseButtonHint);
    setMinimumSize(w, h);
    setMaximumSize(w, h);
    resize(w, h);
}

void MainWindow::applyChatSize()
{
    setIsFixedSize(false);
    setWindowButtonFlags(ElaAppBarType::MinimizeButtonHint | ElaAppBarType::MaximizeButtonHint | ElaAppBarType::CloseButtonHint);
    setMinimumSize(1050, 900 + kAppBarHeight);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    resize(1050, 900 + kAppBarHeight);
}

void MainWindow::SlotSwitchReg()
{
    _reg_dlg = new RegistDialog(this);
    setPage(_reg_dlg);
    applyLoginSize();
    connect(_reg_dlg, &RegistDialog::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);
    _ui_status = REGISTER_UI;
}

void MainWindow::SlotSwitchLogin()
{
    _login_dlg = new LoginDialog(this);
    setPage(_login_dlg);
    applyLoginSize();

    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);

    _ui_status = LOGIN_UI;
}

void MainWindow::SlotSwitchReset()
{
    _ui_status = RESET_UI;

    _reset_dlg = new ResetDialog(this);
    setPage(_reset_dlg);
    applyLoginSize();

    connect(_reset_dlg, &ResetDialog::switchLogin, this, &MainWindow::SlotSwitchLogin2);
}

void MainWindow::SlotSwitchLogin2()
{
    _login_dlg = new LoginDialog(this);
    setPage(_login_dlg);
    applyLoginSize();

    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    _ui_status = LOGIN_UI;
}

void MainWindow::SlotSwitchChat()
{
    _chat_dlg = new ChatDialog();
    setPage(_chat_dlg);
    applyChatSize();
    _ui_status = CHAT_UI;
}

void MainWindow::SlotOffline()
{
    TcpMgr::GetInstance()->CloseConnection();
    offlineLogin();
    // in-window toast: a modal QMessageBox can surface as a stray top-level window
    ElaMessageBar::warning(ElaMessageBarType::TopLeft, QStringLiteral("下线提醒"),
                           QStringLiteral("已在别处登录"), 3000, this);
}

void MainWindow::SlotReconnecting(int attempt, int delayMs)
{
    setWindowTitle(QString("Chat - 连接中断，第 %1 次重连（%2 秒后）")
                       .arg(attempt)
                       .arg(delayMs / 1000));
}

void MainWindow::SlotReconnected()
{
    setWindowTitle("Chat");
}

void MainWindow::SlotReconnectFailed()
{
    setWindowTitle("Chat");
    TcpMgr::GetInstance()->CloseConnection();
    offlineLogin();
    ElaMessageBar::warning(ElaMessageBarType::TopLeft, QStringLiteral("网络异常"),
                           QStringLiteral("请重新登录"), 3000, this);
}

void MainWindow::offlineLogin()
{
    if (_ui_status == LOGIN_UI)
    {
        return; // if the UI has returned to the login screen, do nothing
    }
    _login_dlg = new LoginDialog(this);
    setPage(_login_dlg);
    applyLoginSize();

    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    _ui_status = LOGIN_UI;
}
