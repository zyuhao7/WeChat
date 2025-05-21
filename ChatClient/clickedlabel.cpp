#include "clickedlabel.h"
#include <QMouseEvent>

ClickedLabel::ClickedLabel(QWidget *parent)
    :QLabel(parent),
      _curstate(ClickLbState::Normal)
{
    this->setCursor(Qt::PointingHandCursor);
}

// 鼠标按下事件处理函数
void ClickedLabel::mousePressEvent(QMouseEvent *event)
{
    // _curstate 是标签本身的逻辑状态，和鼠标是否按着没关
    if (event->button() == Qt::LeftButton)
    {
            // 如果当前状态是 Normal(未选中).
            if(_curstate == ClickLbState::Normal)
            {
                qDebug()<<"clicked , change to selected hover: "<< _selected_hover;
                _curstate = ClickLbState::Selected;
                setProperty("state",_selected_hover); // 选中状态下的 hover 样式
                repolish(this);
                update();
            }
            // 当前为 Selected(选中).
            else
            {
                qDebug()<<"clicked , change to normal hover: "<< _normal_hover;
                _curstate = ClickLbState::Normal;
                setProperty("state",_normal_hover);
                repolish(this);
                update();
            }
            //  处理完左键逻辑后直接返回，避免继续传递事件
            return;
      }

    // 调用基类的 mousePressEvent 以保证正常的事件处理
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

     // 调用基类的 mouseReleaseEvent 以保证正常的事件处理
    QLabel::mouseReleaseEvent(event);
}

void ClickedLabel::enterEvent(QEnterEvent *event)
{
        // 当鼠标移动到 ClickedLabel 上时
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
    // 当鼠标离开 ClickedLabel 时
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

