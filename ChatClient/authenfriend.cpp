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

    // 隐藏对话框标题栏
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
    //连接输入标签回车事件
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

        QFontMetrics fontMetrics(lb->font()); // 获取 QLabel 部件的字体信息
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
    // 如果已经存在, 则清空输入框后返回.
    if(_friend_labels.find(name) != _friend_labels.end())
    {
        ui->lb_ed->clear();
        return;
    }
    //  创建新的好友标签
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
        // // 如果不需要换行，保持当前 _label_point 的 X 坐标不变
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

    //重排现有的 Label
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

    // 添加未添加到展示列表的
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

        QFontMetrics fontMetrics(lb->font()); // 获取QLabel控件的字体信息
        int textWidth = fontMetrics.horizontalAdvance(lb->text()); // 获取文本的宽度
        int textHeight = fontMetrics.height(); // 获取文本的高度

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
    auto find_iter = _add_labels.find(lbtext); // 为什么要查找 _add_labels.find()？ 为了确保点击的 Tip 标签合法且存在，防止空指针操作
    if(find_iter == _add_labels.end())
        return;
    if(state == ClickLbState::Selected) //  添加好友标签
    {
        addLabel(lbtext);
        return;
    }
    if(state == ClickLbState::Normal)  //  移除好友标签
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
        _tip_data.push_back(text); // 确保不重复添加
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
    // 适配新增tip标签后超出显示区域，防止 UI 视觉混乱或显示不全
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
