#include "findsuccessdlg.h"
#include "ui_findsuccessdlg.h"
#include <QDir>
#include <QDebug>
#include "applyfriend.h"
#include <memory>
FindSuccessDlg::FindSuccessDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FindSuccessDlg),
    _parent(parent)
{
    ui->setupUi(this);
    // set the dialog title
    setWindowTitle("添加");
    // hide the dialog title bar
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    this->setObjectName("FindSuccessDlg");

    // get the current application path
    QString app_path = QCoreApplication::applicationDirPath();
    QString pix_path = QDir::toNativeSeparators(app_path +
                                                QDir::separator() + "static" + QDir::separator() + "head_1.jpg");
    QPixmap head_pix(pix_path);
    head_pix = head_pix.scaled(ui->head_lb->size(),
                               Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->head_lb->setPixmap(head_pix);
    ui->add_friend_btn->SetState("normal", "hover", "press");
    this->setModal(true);

}

FindSuccessDlg::~FindSuccessDlg()
{
    qDebug() <<"FindSuccessDlg success";
    delete ui;
}

void FindSuccessDlg::SetSearchInfo(std::shared_ptr<SearchInfo> si)
{
    ui->name_lb->setText(si->_name);
    _si = si;
}

void FindSuccessDlg::on_add_friend_btn_clicked()
{
    this->hide();
    // pop up the add-friend page
    auto applyFriend = new ApplyFriend(_parent);
    applyFriend->SetSearchInfo(_si);
    applyFriend->setModal(true);
    applyFriend->show();
}
