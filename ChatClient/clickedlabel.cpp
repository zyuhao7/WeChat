#include "clickedlabel.h"
#include <QMouseEvent>

ClickedLabel::ClickedLabel(QWidget *parent)
    :QLabel(parent),
      _curstate(ClickLbState::Normal)
{
    this->setCursor(Qt::PointingHandCursor);
}

// mouse press event handler
void ClickedLabel::mousePressEvent(QMouseEvent *event)
{
    // _curstate is the label's own logical state, unrelated to whether the mouse is pressed
    if (event->button() == Qt::LeftButton)
    {
            // if the current state is Normal (not selected).
            if(_curstate == ClickLbState::Normal)
            {
                qDebug()<<"clicked , change to selected hover: "<< _selected_hover;
                _curstate = ClickLbState::Selected;
                setProperty("state",_selected_hover); // hover style in the selected state
                repolish(this);
                update();
            }
            // currently Selected.
            else
            {
                qDebug()<<"clicked , change to normal hover: "<< _normal_hover;
                _curstate = ClickLbState::Normal;
                setProperty("state",_normal_hover);
                repolish(this);
                update();
            }
            //  return after handling the left-click logic to avoid further event propagation
            return;
      }

    // call the base-class mousePressEvent for normal event handling
    QLabel::mousePressEvent(event);
}

void ClickedLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
            if(_curstate == ClickLbState::Normal){
                  qDebug()<<"ReleaseEvent, change to normal hover : "<< _normal_hover;
                setProperty("state",_normal_press);
                repolish(this);
                update();

            }
            else
            {
                  qDebug()<<"Release , change to select hover: "<< _selected_hover;
                setProperty("state",_selected_press);
                repolish(this);
                update();
            }
            emit clicked(this->text(), _curstate);
            return;
     }

     // call the base-class mouseReleaseEvent for normal event handling
    QLabel::mouseReleaseEvent(event);
}

void ClickedLabel::enterEvent(QEnterEvent *event)
{
        // when the mouse moves onto ClickedLabel
        if(_curstate == ClickLbState::Normal){
             qDebug()<<"enter , change to normal hover: "<< _normal_hover;
            setProperty("state",_normal_hover);
            repolish(this);
            update();

        }else{
             qDebug()<<"enter , change to selected hover: "<< _selected_hover;
            setProperty("state",_selected_hover);
            repolish(this);
            update();
        }
        QLabel::enterEvent(event);
}

void ClickedLabel::leaveEvent(QEvent *event)
{
    // when the mouse leaves ClickedLabel
        if(_curstate == ClickLbState::Normal){
             qDebug()<<"leave , change to normal : "<< _normal;
            setProperty("state",_normal);
            repolish(this);
            update();

        }else{
             qDebug()<<"leave , change to normal hover: "<< _selected;
            setProperty("state",_selected);
            repolish(this);
            update();
        }
        QLabel::leaveEvent(event);
}

void ClickedLabel::SetState(QString normal, QString hover, QString press, QString select, QString select_hover, QString select_press)
{
       _normal = normal;
       _normal_hover = hover;
       _normal_press = press;

       _selected = select;
       _selected_hover = select_hover;
       _selected_press = select_press;

       setProperty("state",normal);
       repolish(this);
}

ClickLbState ClickedLabel::GetCurState()
{
    return _curstate;
}

bool ClickedLabel::SetCurState(ClickLbState state)
{
    _curstate = state;
    if(_curstate == ClickLbState::Normal)
    {
        setProperty("state", _normal);
        repolish(this);
    }
    else if(_curstate == ClickLbState::Selected)
    {
        setProperty("state", _selected);
        repolish(this);
    }
    return true;
}


void ClickedLabel::ResetNormalState()
{
    _curstate = ClickLbState::Normal;
    setProperty("state", _normal);
    repolish(this);
}

