#include "adduseritem.h"
#include "ui_adduseritem.h"
#include "ElaIcon.h"

AddUserItem::AddUserItem(QWidget *parent) :
    ListItemBase(parent),
    ui(new Ui::AddUserItem)
{
    ui->setupUi(this);
    SetItemType(ListItemType::ADD_USER_TIP_ITEM);
    // real icons replacing the removed QSS border-image
    ui->add_tip->setPixmap(ElaIcon::getInstance()->getElaIcon(ElaIconType::MagnifyingGlass, 22).pixmap(22, 22));
    ui->add_tip->setAlignment(Qt::AlignCenter);
    ui->right_tip->setPixmap(ElaIcon::getInstance()->getElaIcon(ElaIconType::ChevronRight, 18).pixmap(18, 18));
    ui->right_tip->setAlignment(Qt::AlignCenter);
}

AddUserItem::~AddUserItem()
{
    delete ui;
}
