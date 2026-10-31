#include <QtTest>

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <memory>

#include "chatpage.h"
#include "clickedlabel.h"
#include "logindialog.h"
#include "messagetextedit.h"
#include "registdialog.h"
#include "resetdialog.h"
#include "tcpmgr.h"
#include "usermgr.h"

// These tests drive the real widgets, so a refactor that drops a connect() or renames a
// widget fails here even when every helper still compiles.
class UiWiringTest : public QObject
{
    Q_OBJECT

private slots:
    void loginDialogExposesItsControls()
    {
        LoginDialog dialog;

        auto* email = dialog.findChild<QLineEdit*>("email_edit");
        auto* pass = dialog.findChild<QLineEdit*>("pass_edit");
        auto* login = dialog.findChild<QPushButton*>("login_btn");
        auto* regist = dialog.findChild<QPushButton*>("reg_btn");
        auto* forget = dialog.findChild<ClickedLabel*>("forget_label");

        QVERIFY(email);
        QVERIFY(pass);
        QVERIFY(login);
        QVERIFY(regist);
        QVERIFY(forget);
        QCOMPARE(pass->echoMode(), QLineEdit::Password);
        QVERIFY(login->isEnabled());
        QVERIFY(regist->isEnabled());
    }

    void registerButtonSwitchesToRegisterPage()
    {
        LoginDialog dialog;
        dialog.show();
        QSignalSpy spy(&dialog, &LoginDialog::switchRegister);

        QTest::mouseClick(dialog.findChild<QPushButton*>("reg_btn"), Qt::LeftButton);

        QCOMPARE(spy.count(), 1);
    }

    void forgetPasswordLinkSwitchesToResetPage()
    {
        LoginDialog dialog;
        dialog.show();
        QSignalSpy spy(&dialog, &LoginDialog::switchReset);

        QTest::mouseClick(dialog.findChild<ClickedLabel*>("forget_label"), Qt::LeftButton);

        QCOMPARE(spy.count(), 1);
    }

    void emptyEmailIsRejectedBeforeAnyRequest()
    {
        LoginDialog dialog;
        dialog.show();
        auto* login = dialog.findChild<QPushButton*>("login_btn");
        auto* tip = dialog.findChild<QLabel*>("err_tip");

        QTest::mouseClick(login, Qt::LeftButton);

        QVERIFY(tip);
        QVERIFY(!tip->text().isEmpty());
        QVERIFY(login->isEnabled());
    }

    void shortPasswordIsRejected()
    {
        LoginDialog dialog;
        dialog.show();
        dialog.findChild<QLineEdit*>("email_edit")->setText(QStringLiteral("mock1@test.com"));
        dialog.findChild<QLineEdit*>("pass_edit")->setText(QStringLiteral("123"));
        auto* tip = dialog.findChild<QLabel*>("err_tip");

        QTest::mouseClick(dialog.findChild<QPushButton*>("login_btn"), Qt::LeftButton);

        QVERIFY(tip->text().contains(QStringLiteral("6")));
    }

    void enterInEitherFieldSubmitsTheLogin()
    {
        LoginDialog dialog;
        dialog.show();
        auto* email = dialog.findChild<QLineEdit*>("email_edit");
        auto* pass = dialog.findChild<QLineEdit*>("pass_edit");
        auto* tip = dialog.findChild<QLabel*>("err_tip");

        // empty form: the same validation as the button must run on Enter
        QTest::keyClick(pass, Qt::Key_Return);
        QVERIFY(!tip->text().isEmpty());

        tip->clear();
        QTest::keyClick(email, Qt::Key_Return);
        QVERIFY(!tip->text().isEmpty());
    }

    void escapeKeepsTheEmbeddedDialogVisible()
    {
        LoginDialog dialog;
        dialog.show();
        QSignalSpy rejected(&dialog, &QDialog::rejected);

        QTest::keyClick(&dialog, Qt::Key_Escape);

        QCOMPARE(rejected.count(), 0);
        QVERIFY(dialog.isVisible());
    }

    void registCancelReturnsToLogin()
    {
        RegistDialog dialog;
        dialog.show();
        QSignalSpy spy(&dialog, &RegistDialog::sigSwitchLogin);

        QTest::mouseClick(dialog.findChild<QPushButton*>("cancel_btn"), Qt::LeftButton);

        QCOMPARE(spy.count(), 1);
    }

    void resetReturnReturnsToLogin()
    {
        ResetDialog dialog;
        dialog.show();
        QSignalSpy spy(&dialog, &ResetDialog::switchLogin);

        QTest::mouseClick(dialog.findChild<QPushButton*>("return_btn"), Qt::LeftButton);

        QCOMPARE(spy.count(), 1);
    }

    void sendButtonEmitsThePendingText()
    {
        UserMgr::GetInstance()->SetUserInfo(std::make_shared<UserInfo>(6, "tester", ""));
        ChatPage page;
        page.SetUserInfo(std::make_shared<UserInfo>(9, "peer", ""));

        auto* edit = page.findChild<MessageTextEdit*>("chatEdit");
        auto* send = page.findChild<QPushButton*>("send_btn");
        QVERIFY(edit);
        QVERIFY(send);
        edit->setPlainText(QStringLiteral("hello"));

        int emitted = 0;
        std::shared_ptr<TextChatData> sent;
        connect(&page, &ChatPage::sig_append_send_chat_msg, &page,
                [&](std::shared_ptr<TextChatData> msg) {
                    ++emitted;
                    sent = std::move(msg);
                });

        QTest::mouseClick(send, Qt::LeftButton);

        QCOMPARE(emitted, 1);
        QVERIFY(sent);
        QCOMPARE(sent->_msg_content, QStringLiteral("hello"));
    }

    void sendButtonWithoutAnOpenChatDoesNothing()
    {
        UserMgr::GetInstance()->SetUserInfo(std::make_shared<UserInfo>(6, "tester", ""));
        ChatPage page;

        auto* edit = page.findChild<MessageTextEdit*>("chatEdit");
        auto* send = page.findChild<QPushButton*>("send_btn");
        QVERIFY(edit);
        QVERIFY(send);
        edit->setPlainText(QStringLiteral("hello"));

        int emitted = 0;
        connect(&page, &ChatPage::sig_append_send_chat_msg, &page,
                [&](std::shared_ptr<TextChatData>) { ++emitted; });

        QTest::mouseClick(send, Qt::LeftButton);

        QCOMPARE(emitted, 0);
    }

    void sendingIsHeldBackWhileReconnecting()
    {
        UserMgr::GetInstance()->SetUserInfo(std::make_shared<UserInfo>(6, "tester", ""));
        ChatPage page;
        page.SetUserInfo(std::make_shared<UserInfo>(9, "peer", ""));

        auto* edit = page.findChild<MessageTextEdit*>("chatEdit");
        auto* send = page.findChild<QPushButton*>("send_btn");
        QVERIFY(edit);
        QVERIFY(send);
        edit->setPlainText(QStringLiteral("hello"));

        int emitted = 0;
        connect(&page, &ChatPage::sig_append_send_chat_msg, &page,
                [&](std::shared_ptr<TextChatData>) { ++emitted; });

        emit TcpMgr::GetInstance() -> sig_reconnecting(1, 1000);
        QVERIFY(!send->isEnabled());
        QTest::mouseClick(send, Qt::LeftButton);
        QCOMPARE(emitted, 0);
        QCOMPARE(edit->toPlainText(), QStringLiteral("hello"));

        emit TcpMgr::GetInstance() -> sig_reconnected();
        QVERIFY(send->isEnabled());
        QTest::mouseClick(send, Qt::LeftButton);
        QCOMPARE(emitted, 1);
    }
};

QObject* createUiWiringTest()
{
    return new UiWiringTest;
}

#include "test_ui_wiring.moc"
