#include "clickedbtn.h"
#include "global.h"
#include "ElaIcon.h"
#include <QVariant>



ClickedBtn::ClickedBtn(QWidget *parent)
    : QPushButton(parent),
      _has_icon(false)
{
    setCursor(Qt::PointingHandCursor); //set the cursor to a pointing hand
    setFocusPolicy(Qt::NoFocus);
}

ClickedBtn::~ClickedBtn(){}

void ClickedBtn::SetState(QString normal, QString hover, QString press)
{
    _normal = normal;
    _hover = hover;
    _press = press;
    setProperty("state", normal);
    repolish(this);
    update();
}

void ClickedBtn::SetIcons(ElaIconType::IconName normal,
                          ElaIconType::IconName hover,
                          ElaIconType::IconName press,
                          int pixelSize)
{
    // pick a glyph size that fits the button's fixed box when one is declared
    int box = qMin(minimumWidth() > 1 ? minimumWidth() : 24,
                   minimumHeight() > 1 ? minimumHeight() : 24);
    int size = pixelSize > 0 ? pixelSize : qBound(16, qRound(box * 0.7), 24);

    _icon_normal = ElaIcon::getInstance()->getElaIcon(normal, size);
    _icon_hover  = ElaIcon::getInstance()->getElaIcon(hover, size);
    _icon_press  = ElaIcon::getInstance()->getElaIcon(press, size);
    _has_icon = true;

    setText(QString());
    setFlat(true);
    setIconSize(QSize(size, size));
    setIcon(_icon_normal);
}


void ClickedBtn::enterEvent(QEnterEvent *event)
{
    setProperty("state", _hover);
    repolish(this);
    update();
    if(_has_icon) setIcon(_icon_hover);
    QPushButton::enterEvent(event);
}

void ClickedBtn::leaveEvent(QEvent *event)
{
    setProperty("state",_normal);
    repolish(this);
    update();
    if(_has_icon) setIcon(_icon_normal);
    QPushButton::leaveEvent(event);
}

void ClickedBtn::mousePressEvent(QMouseEvent *event)
{
    setProperty("state",_press);
    repolish(this);
    update();
    if(_has_icon) setIcon(_icon_press);
    QPushButton::mousePressEvent(event);
}

void ClickedBtn::mouseReleaseEvent(QMouseEvent *event)
{
    setProperty("state",_hover);
    repolish(this);
    update();
    if(_has_icon) setIcon(_icon_hover);
    QPushButton::mouseReleaseEvent(event);
}
