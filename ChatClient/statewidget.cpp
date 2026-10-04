#include "statewidget.h"
#include "ElaIcon.h"
#include <QPaintEvent>
#include <QStyleOption>
#include <QPainter>
#include <QLabel>
#include <QVBoxLayout>


StateWidget::StateWidget(QWidget *parent)
    :QWidget(parent),
      _curstate(ClickLbState::Normal),
      _has_icons(false)
{
    setCursor(Qt::PointingHandCursor);
    // add the red dot
    AddRedPoint();
}

void StateWidget::SetIcons(ElaIconType::IconName normal,
                           ElaIconType::IconName selected,
                           int pixelSize)
{
    int box = qMin(minimumWidth() > 1 ? minimumWidth() : 24,
                   minimumHeight() > 1 ? minimumHeight() : 24);
    int size = pixelSize > 0 ? pixelSize : qBound(16, box, 24);

    _pix_normal   = ElaIcon::getInstance()->getElaIcon(normal, size).pixmap(size, size);
    _pix_selected = ElaIcon::getInstance()->getElaIcon(selected, size).pixmap(size, size);
    _has_icons = true;
    update();
}

void StateWidget::SetState(QString normal, QString hover, QString press, QString select, QString select_hover, QString select_press)
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

ClickLbState StateWidget::GetCurState()
{
    return _curstate;
}

void StateWidget::ClearState()
{
    _curstate = ClickLbState::Normal;
    setProperty("state", _normal);
    repolish(this);
    update();
}

void StateWidget::SetSelected(bool bselected)
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

void StateWidget::AddRedPoint()
{
    // add the red-dot indicator
    _red_point = new QLabel();
    _red_point->setObjectName("red_point");
    _red_point->setPixmap(QPixmap(":/res/red_point.png"));
    _red_point->setScaledContents(true);
    QVBoxLayout* layout2 = new QVBoxLayout;
    _red_point->setAlignment(Qt::AlignCenter);
    layout2->addWidget(_red_point);
    layout2->setContentsMargins(0, 0 ,0 ,0);
    this->setLayout(layout2);
    _red_point->setVisible(false);
}

void StateWidget::ShowRedPoint(bool show)
{
    Q_UNUSED(show);
    _red_point->setVisible(true);
}

void StateWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    if(_has_icons){
        const QPixmap& pm = (_curstate == ClickLbState::Selected) ? _pix_selected : _pix_normal;
        QRect target(QPoint((width() - pm.width()) / 2, (height() - pm.height()) / 2), pm.size());
        p.drawPixmap(target, pm);
    }
    return;
}

void StateWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
            if(_curstate == ClickLbState::Selected){
                qDebug()<<"PressEvent , already to selected press: "<< _selected_press;
                //emit clicked();
                // call the base-class mousePressEvent for normal event handling
                QWidget::mousePressEvent(event);
                return;
            }

            if(_curstate == ClickLbState::Normal){
                qDebug()<<"PressEvent , change to selected press: "<< _selected_press;
                _curstate = ClickLbState::Selected;
                setProperty("state",_selected_press);
                repolish(this);
                update();
            }

            return;
    }
       // call the base-class mousePressEvent for normal event handling
       QWidget::mousePressEvent(event);
}

void StateWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
            if(_curstate == ClickLbState::Normal){
                //qDebug()<<"ReleaseEvent , change to normal hover: "<< _normal_hover;
                setProperty("state",_normal_hover);
                repolish(this);
                update();

            }else{
                //qDebug()<<"ReleaseEvent , change to select hover: "<< _selected_hover;
                setProperty("state",_selected_hover);
                repolish(this);
                update();
            }
            emit clicked();
            return;
        }
        // call the base-class mousePressEvent for normal event handling
    QWidget::mousePressEvent(event);
}

void StateWidget::enterEvent(QEnterEvent *event)
{
        // handle mouse hover-enter logic here
       if(_curstate == ClickLbState::Normal)
       {
            //qDebug()<<"enter , change to normal hover: "<< _normal_hover;
           setProperty("state",_normal_hover);
           repolish(this);
           update();

       }
       else
       {
            //qDebug()<<"enter , change to selected hover: "<< _selected_hover;
           setProperty("state",_selected_hover);
           repolish(this);
           update();
       }

       QWidget::enterEvent(event);
}

void StateWidget::leaveEvent(QEvent *event)
{
    // handle mouse hover-leave logic here
       if(_curstate == ClickLbState::Normal){
           // qDebug()<<"leave , change to normal : "<< _normal;
           setProperty("state",_normal);
           repolish(this);
           update();

       }else{
           // qDebug()<<"leave , change to select normal : "<< _selected;
           setProperty("state",_selected);
           repolish(this);
           update();
       }
       QWidget::leaveEvent(event);
}


