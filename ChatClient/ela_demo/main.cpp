// Throwaway proof-of-concept: renders a Fluent (ElaWidgetTools) login screen.
// Links the ElaWidgetTools shared lib built separately with CMake.
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QFont>

#include "ElaApplication.h"
#include "ElaWidget.h"
#include "ElaText.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    // Enable Ela framework (theme + frameless window plumbing)
    eApp->init();
    eApp->setFontPixelSize(15);

    ElaWidget w;
    w.setWindowTitle("Chat");
    w.resize(380, 560);
    w.setIsFixedSize(false);
    w.setAppBarHeight(48);

    auto* central = new QWidget(&w);
    auto* lay = new QVBoxLayout(central);
    lay->setContentsMargins(48, 40, 48, 40);
    lay->setSpacing(16);

    auto* title = new ElaText("Chat", 34);
    title->setAlignment(Qt::AlignCenter);
    auto* sub = new ElaText(QStringLiteral("登录你的账号"), 15);
    sub->setAlignment(Qt::AlignCenter);

    auto* email = new ElaLineEdit();
    email->setPlaceholderText(QStringLiteral("邮箱 / 账号"));
    email->setFixedHeight(42);
    email->setIsClearButtonEnable(true);

    auto* pwd = new ElaLineEdit();
    pwd->setPlaceholderText(QStringLiteral("密码"));
    pwd->setEchoMode(QLineEdit::Password);
    pwd->setFixedHeight(42);

    auto* login = new ElaPushButton(QStringLiteral("登  录"));
    login->setFixedHeight(42);
    login->setBorderRadius(6);

    auto* reg = new ElaPushButton(QStringLiteral("注  册"));
    reg->setFixedHeight(42);
    reg->setBorderRadius(6);

    lay->addSpacing(24);
    lay->addWidget(title);
    lay->addWidget(sub);
    lay->addSpacing(28);
    lay->addWidget(email);
    lay->addWidget(pwd);
    lay->addSpacing(12);
    lay->addWidget(login);
    lay->addWidget(reg);
    lay->addStretch();

    auto* root = new QVBoxLayout(&w);
    root->setContentsMargins(0, 48, 0, 0); // leave room for the Ela app bar
    root->addWidget(central);

    w.moveToCenter();
    w.show();
    return a.exec();
}
