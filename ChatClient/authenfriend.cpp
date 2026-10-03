#include "authenfriend.h"
#include "ui_authenfriend.h"
#include "clickedlabel.h"
#include "friendlabel.h"
#include <QScrollBar>
#include <QJsonDocument>
#include "usermgr.h"
#include "tcpmgr.h"

AuthenFriend::AuthenFriend(QWidget *parent) :
    QDialog(parent),
    _label_point(2, 6),
    ui(new Ui::AuthenFriend)
{
    ui->setupUi(this);

    // hide the dialog title bar
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    this->setObjectName("AuthenFriend");
    this->setModal(true);
    ui->name_ed->setPlaceholderText(tr("沫羽皓"));
    ui->lb_ed->setPlaceholderText("搜索、添加标签");
    ui->back_ed->setPlaceholderText("friend");


    ui->lb_ed->setMaxLength(21);
    ui->lb_ed->move(2, 2);
    ui->lb_ed->setFixedHeight(20);
    ui->lb_ed->setMaxLength(10);
    ui->input_tip_wid->hide();

    _tip_cur_point = QPoint(5,5);
    _tip_data = {
        "同学", "朋友", "家人", "亲戚", "老师", "师傅", "陪玩", "主包", "Leader", "学妹"
    };
    connect(ui->more_lb, &ClickedOnceLabel::clicked, this, &AuthenFriend::ShowMoreLabel);
    InitTipLbs();
    //connect the input label Enter event
    connect(ui->lb_ed, &CustomizeEdit::returnPressed, this, &AuthenFriend::SlotLabelEnter);
    connect(ui->lb_ed, &CustomizeEdit::textChanged, this, &AuthenFriend::SlotLabelTextChange);
    connect(ui->lb_ed, &CustomizeEdit::editingFinished, this, &AuthenFriend::SlotLabelEditFinished);
    connect(ui->tip_lb,&ClickedOnceLabel::clicked, this, &AuthenFriend::SlotAddFirendLabelByClickTip);

    ui->scrollArea->horizontalScrollBar()->setHidden(true);
    ui->scrollArea->verticalScrollBar()->setHidden(true);
    ui->scrollArea->installEventFilter(this);
    ui->sure_btn->SetState("normal", "hover", "press");
    ui->cancel_btn->SetState("normal", "hover", "press");

    connect(ui->cancel_btn,&QPushButton::clicked, this, &AuthenFriend::SlotAuthenCancel);
    connect(ui->sure_btn, &QPushButton::clicked, this, &AuthenFriend::SlotAuthenSure);
}

AuthenFriend::~AuthenFriend()
{
    qDebug() << "AuthenFriend destrcut!";
    delete ui;
}

void AuthenFriend::InitTipLbs()
{
    int lines = 1;
    for(size_t i = 0; i < _tip_data.size(); ++i)
    {
        auto* lb = new ClickedLabel(ui->lb_list);
        lb->SetState("normal", "hover", "pressed", "selected_normal",
                     "selected_hover", "selected_pressed");
        lb->setObjectName("tipslb");
        lb->setText(_tip_data[i]);
        connect(lb, &ClickedLabel::clicked, this, &AuthenFriend::SlotChangeFriendLabelByTip);

        QFontMetrics fontMetrics(lb->font()); // get the QLabel font info
        int textWidth = fontMetrics.horizontalAdvance(lb->text());
        int textHeight = fontMetrics.height();

        if(_tip_cur_point.x() + textWidth + tip_offset > ui->lb_list->width())
        {
            lines++;
            if(lines > 2)
            {
                delete lb;
                return;
            }
            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);
        }

        auto next_point = _tip_cur_point;
        AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
        _tip_cur_point = next_point;
    }
}

void AuthenFriend::AddTipLbs(ClickedLabel *lb, QPoint cur_point, QPoint &next_point, int text_width, int text_height)
{
    lb->move(cur_point);
    lb->show();
    _add_labels.insert(lb->text(), lb);
    _add_label_keys.push_back(lb->text());
    next_point.setX(lb->pos().x() + text_width + 15);
    next_point.setY(lb->pos().y());
}

bool AuthenFriend::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == ui->scrollArea &&event->type() == QEvent::Enter)
    {
        ui->scrollArea->verticalScrollBar()->setHidden(false);
    }
    else if(obj == ui->scrollArea && event->type() == QEvent::Leave)
    {
        ui->scrollArea->verticalScrollBar()->setHidden(true);
    }
    return QObject::eventFilter(obj, event);
}

void AuthenFriend::SetApplyInfo(std::shared_ptr<ApplyInfo> apply_info)
{
    _apply_info = apply_info;
    ui->back_ed->setPlaceholderText(apply_info->_name);
}

void AuthenFriend::resetLabels()
{
    auto max_width = ui->gridWidget->width();
    auto label_height = 0;
    for(auto iter = _friend_labels.begin(); iter != _friend_labels.end(); ++iter)
    {
        if( _label_point.x() + iter.value()->width() > max_width) {
           _label_point.setY(_label_point.y()+iter.value()->height()+6);
           _label_point.setX(2);
        }

        iter.value()->move(_label_point);
        iter.value()->show();

        _label_point.setX(_label_point.x()+iter.value()->width()+2);
        _label_point.setY(_label_point.y());
        label_height = iter.value()->height();
    }
    if(_friend_labels.isEmpty()){
        ui->lb_ed->move(_label_point);
        return;
    }

    if(_label_point.x() + MIN_APPLY_LABEL_ED_LEN > ui->gridWidget->width()){
        ui->lb_ed->move(2,_label_point.y()+label_height+6);
    }
    else
    {
        ui->lb_ed->move(_label_point);
    }
}

void AuthenFriend::addLabel(QString name)
{
    // if it already exists, clear the input box and return.
    if(_friend_labels.find(name) != _friend_labels.end())
    {
        ui->lb_ed->clear();
        return;
    }
    //  create a new friend label
    auto tmplabel = new FriendLabel(ui->gridWidget);
    tmplabel->SetText(name);
    tmplabel->setObjectName("FriendLabel");
    auto max_width = ui->gridWidget->width();
    if(_label_point.x() + tmplabel->width() > max_width)
    {
        _label_point.setY(_label_point.y() + tmplabel->height() + 6);
        _label_point.setX(2);
    }
    else{
        // // if no line break is needed, keep the current X of _label_point
    }

    tmplabel->move(_label_point);
    tmplabel->show();
    _friend_labels[tmplabel->Text()] = tmplabel;
    _friend_label_keys.push_back(tmplabel->Text());

    connect(tmplabel, &FriendLabel::sig_close, this, &AuthenFriend::SlotRemoveFriendLabel);
    _label_point.setX(_label_point.x() + tmplabel->width() + 2);

    if (_label_point.x() + MIN_APPLY_LABEL_ED_LEN > ui->gridWidget->width()) {
        ui->lb_ed->move(2, _label_point.y() + tmplabel->height() + 2);
    }
    else {
        ui->lb_ed->move(_label_point);
    }

    ui->lb_ed->clear();

    if (ui->gridWidget->height() < _label_point.y() + tmplabel->height() + 2) {
        ui->gridWidget->setFixedHeight(_label_point.y() + tmplabel->height() * 2 + 2);
    }
}

void AuthenFriend::ShowMoreLabel()
{
    qDebug() << "receive more label clicked";
    ui->more_lb_wid->hide();
    ui->lb_list->setFixedWidth(325);
    _tip_cur_point = QPoint(5, 5);

    auto next_point = _tip_cur_point;
    int textWidth;
    int textHeight;

    //re-layout the existing labels
    for(auto& added_key : _add_label_keys)
    {
        auto added_lb = _add_labels[added_key];

        QFontMetrics fontMetrics(added_lb->font());
        textWidth = fontMetrics.horizontalAdvance(added_lb->text());
        textHeight = fontMetrics.height();

        if(_tip_cur_point.x() + textWidth + tip_offset > ui->lb_list->width())
        {
            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);
        }
        added_lb->move(_tip_cur_point);

        next_point.setX(added_lb->pos().x() + textWidth + 15);
        next_point.setY(_tip_cur_point.y());

        _tip_cur_point = next_point;
    }

    // add those not yet in the display list
    for(size_t i = 0; i < _tip_data.size(); ++i)
    {
        auto iter = _add_labels.find(_tip_data[i]);
        if(iter != _add_labels.end())
            continue;
        auto* lb = new ClickedLabel(ui->lb_list);
        lb->SetState("normal", "hover", "pressed", "selected_normal",
                    "selected_hover", "selected_pressed");
        lb->setObjectName("tipslb");
        lb->setText(_tip_data[i]);
        connect(lb, &ClickedLabel::clicked, this, &AuthenFriend::SlotChangeFriendLabelByTip);

        QFontMetrics fontMetrics(lb->font()); // get the QLabel font info
        int textWidth = fontMetrics.horizontalAdvance(lb->text()); // get the text width
        int textHeight = fontMetrics.height(); // get the text height

        if (_tip_cur_point.x() + textWidth + tip_offset > ui->lb_list->width()) {

            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);

        }

         next_point = _tip_cur_point;

        AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);

        _tip_cur_point = next_point;
    }

    int diff_height = next_point.y() + textHeight + tip_offset - ui->lb_list->height();
    ui->lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);

    ui->scrollcontent->setFixedHeight(ui->scrollcontent->height() + diff_height);
}

void AuthenFriend::SlotLabelEnter()
{
    if(ui->lb_ed->text().isEmpty())
        return;

    addLabel(ui->lb_ed->text());
    ui->input_tip_wid->hide();
}

void AuthenFriend::SlotRemoveFriendLabel(QString name)
{
    qDebug() <<"receive close signal";
    _label_point.setX(2);
    _label_point.setY(6);
    auto find_iter = _friend_labels.find(name);
    if(find_iter == _friend_labels.end()) return;

    auto find_key = _friend_label_keys.end();
    for(auto it = _friend_label_keys.begin(); it != _friend_label_keys.end(); ++it)
    {
        if(*it  == name)
        {
            find_key = it;
            break;
        }
    }

    if(find_key != _friend_label_keys.end())
    {
        _friend_label_keys.erase(find_key);
    }

    delete find_iter.value();
    _friend_labels.erase(find_iter);
    resetLabels();

    auto find_add = _add_labels.find(name);
    if(find_add  == _add_labels.end()) return;
    find_add.value()->ResetNormalState();
}

void AuthenFriend::SlotChangeFriendLabelByTip(QString lbtext, ClickLbState state)
{
    auto find_iter = _add_labels.find(lbtext); // Why check _add_labels.find()? To ensure the clicked Tip label is valid and exists, preventing null-pointer access
    if(find_iter == _add_labels.end())
        return;
    if(state == ClickLbState::Selected) //  add the friend label
    {
        addLabel(lbtext);
        return;
    }
    if(state == ClickLbState::Normal)  //  remove the friend label
    {
        SlotRemoveFriendLabel(lbtext);
        return;
    }
}

void AuthenFriend::SlotLabelTextChange(const QString &text)
{
    if(text.isEmpty())
    {
        ui->tip_lb->setText("");
        ui->input_tip_wid->hide();
        return;
    }
    auto it = std::find(_tip_data.begin(), _tip_data.end(), text);
    if(it == _tip_data.end())
    {
        auto new_text = add_prefix + text;
        ui->tip_lb->setText(new_text);
        ui->input_tip_wid->show();
        return;
    }
    ui->tip_lb->setText(text);
    ui->input_tip_wid->show();
}

void AuthenFriend::SlotLabelEditFinished()
{
     ui->input_tip_wid->hide();
}

void AuthenFriend::SlotAddFirendLabelByClickTip(QString text)
{
    int index = text.indexOf(add_prefix);
    if(index != -1)
    {
        text = text.mid(index + add_prefix.length());
    }
    addLabel(text);

    if (std::find(_tip_data.begin(), _tip_data.end(), text) == _tip_data.end())
    {
        _tip_data.push_back(text); // ensure no duplicate insertion
    }

    auto find_add = _add_labels.find(text);
    if(find_add != _add_labels.end())
    {
        find_add.value()->SetCurState(ClickLbState::Selected);
        return;
    }

    auto *lb = new ClickedLabel(ui->lb_list);
    lb->SetState("normal", "hover", "pressed", "selected_normal",
        "selected_hover", "selected_pressed");
    lb->setObjectName("tipslb");
    lb->setText(text);
    connect(lb, &ClickedLabel::clicked, this, &AuthenFriend::SlotChangeFriendLabelByTip);
    qDebug() <<"ui->lb_list->width() is "<<ui->lb_list->width();
    qDebug() <<"_tip_cur_point.x() is "<<_tip_cur_point.x();

    QFontMetrics fontMetrics(lb->font());
    int textWidth = fontMetrics.horizontalAdvance(lb->text());
    int textHeight = fontMetrics.height();
    qDebug() <<"textWidth is " << textWidth;

    if(_tip_cur_point.x() + textWidth + tip_offset + 3 > ui->lb_list->width())
    {
        _tip_cur_point.setX(5);
        _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);
    }
    auto next_point = _tip_cur_point;

    AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
    _tip_cur_point = next_point;

    int diff_height = next_point.y() + textHeight + tip_offset - ui->lb_list->height();
    ui->lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);

    lb->SetCurState(ClickLbState::Selected);
    // handle overflow after adding tip labels, to prevent visual clutter or clipping
    ui->scrollcontent->setFixedHeight(ui->scrollcontent->height() + diff_height);
}

void AuthenFriend::SlotAuthenSure()
{
    qDebug()<<"Slot Authen Sure Called";
    QJsonObject jsonObj;
    auto uid = UserMgr::GetInstance()->GetUid();
    jsonObj["fromuid"] = uid;
    jsonObj["touid"] = _apply_info->_uid;

    QString bakname ="";
    if(ui->back_ed->text().isEmpty())
        bakname = ui->back_ed->placeholderText();
    else
        bakname = ui->back_ed->text();
    jsonObj["back"] = bakname;

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_AUTH_FRIEND_REQ, jsonData);
    this->hide();
    deleteLater();
}

void AuthenFriend::SlotAuthenCancel()
{
    qDebug() <<" slot Authen Cancel Called";
    this->hide();
    deleteLater();
}
