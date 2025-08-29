#include "statelabel.h"

StateLabel::StateLabel(QWidget *parent)
    : QLabel{parent}, _curstate(ClickLbState::Normal)
{
    setCursor(Qt::PointingHandCursor);
}

void StateLabel::mousePressEvent(QMouseEvent *ev)
{
    if(ev->button() == Qt::LeftButton)
    {
        if(_curstate == ClickLbState::Selected)
        {
            qDebug() << "PressEvent, Already to select press: "<< _selected_press;
            QLabel::mousePressEvent(ev);
            return;
        }
        if(_curstate == ClickLbState::Normal)
        {
            qDebug() << "PressEvent, change to selected press: " << _selected_press;
            _curstate = ClickLbState::Selected;
            setProperty("state", _selected_press);
            repolish(this);
            update();
        }
        return;
    }
    QLabel::mouseMoveEvent(ev);
}

void StateLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    if(ev->button() == Qt::LeftButton)
    {
        if(_curstate == ClickLbState::Normal)
        {
            qDebug() << "ReleaseEvent, change to normal hover: " << _normal_hover;
            setProperty("state", _normal_hover);
            repolish(this);
            update();
        }
        else
        {
            qDebug() << "ReleaseEvent, change to select hover: " << _selected_hover;
            setProperty("state", _selected_hover);
            repolish(this);
            update();
        }
        emit clicked();
        return;
    }
    QLabel::mouseReleaseEvent(ev);
}

void StateLabel::enterEvent(QEnterEvent *event)
{
    if(_curstate == ClickLbState::Normal)
    {
        qDebug() << "enterEvent, change to normal hover: " << _normal_hover;
        setProperty("state", _normal_hover);
        repolish(this);
        update();
    }
    else
    {
        qDebug() <<"EnterEvent, change to selected hover: " << _selected_hover;
        setProperty("state", _selected_hover);
        repolish(this);
        update();
    }
    QLabel::enterEvent(event);
}

void StateLabel::leaveEvent(QEvent *event)
{
    if(_curstate == ClickLbState::Normal)
    {
        qDebug() << "LeaveEvent, change to normal: "<< _normal;
        setProperty("state", _normal);
        repolish(this);
        update();
    }
    else
    {
        qDebug() << "LeaveEvent, change to normal: "<< _selected;
        setProperty("state", _selected);
        repolish(this);
        update();
    }
    QLabel::leaveEvent(event);
}

void StateLabel::SetState(QString normal, QString hover, QString press, QString select, QString select_hover, QString select_press)
{
    _normal = normal;
    _normal_hover = hover;
    _normal_press = press;

    _selected = select;
    _selected_hover = select_hover;
    _selected_press = select_press;

    setProperty("state", normal);
    repolish(this);
}

ClickLbState StateLabel::GetCurState()
{
    return _curstate;
}

void StateLabel::ClearState()
{
    _curstate = ClickLbState::Normal;
    setProperty("state", _normal);
    repolish(this);
    update();
}

void StateLabel::SetSelected(bool bselected)
{
    if(bselected)
    {
        _curstate = ClickLbState::Selected;
        setProperty("state", _selected);
        repolish(this);
        update();
        return;
    }
    _curstate = ClickLbState::Normal;
    setProperty("state", _normal);
    repolish(this);
    update();
    return;
}


