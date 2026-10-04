#ifndef CLICKEDBTN_H
#define CLICKEDBTN_H

#include <QPushButton>
#include <QEnterEvent>
#include <QIcon>
#include "ElaWidgetToolsDef.h"

class ClickedBtn : public QPushButton
{
    Q_OBJECT
public:
    ClickedBtn(QWidget* parent = nullptr);
    ~ClickedBtn();
    void SetState(QString normal, QString hover, QString press);
    // Draw a real icon (Ela icon font) instead of the removed QSS border-image.
    // normal/hover/press are swapped on the matching mouse events.
    void SetIcons(ElaIconType::IconName normal,
                  ElaIconType::IconName hover,
                  ElaIconType::IconName press,
                  int pixelSize = 0);
protected:
    virtual void enterEvent(QEnterEvent* event) override ; // mouse enter
    virtual void leaveEvent(QEvent* event) override;  //mouse leave
    virtual void mousePressEvent(QMouseEvent* event) override; // mouse press
    virtual void mouseReleaseEvent(QMouseEvent* event) override; //mouse release
private:
    QString _normal;
    QString _hover;
    QString _press;

    bool  _has_icon;
    QIcon _icon_normal;
    QIcon _icon_hover;
    QIcon _icon_press;
};

#endif // CLICKEDBTN_H
