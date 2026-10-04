#ifndef CLICKEDLABEL_H
#define CLICKEDLABEL_H
#include <QLabel>
#include <QEvent>
#include <QEnterEvent>
#include <QPixmap>
#include "ElaWidgetToolsDef.h"
#include "global.h"


class ClickedLabel : public QLabel
{
    Q_OBJECT;
public:
    ClickedLabel(QWidget* parent = nullptr);
    virtual void mousePressEvent(QMouseEvent* ev) override;
    virtual void mouseReleaseEvent(QMouseEvent* ev) override;
    virtual void enterEvent(QEnterEvent* event) override;
    virtual void leaveEvent(QEvent* event) override;
    void SetState(QString normal="", QString hover="", QString press="",
                     QString select="", QString select_hover="", QString select_press="");

     // Draw a real icon (Ela icon font) for the normal / selected logical states,
     // replacing the removed QSS border-image. Hover/press reuse the same glyphs.
     void SetIcons(ElaIconType::IconName normal,
                   ElaIconType::IconName selected,
                   int pixelSize = 0);

     ClickLbState GetCurState();
     bool SetCurState(ClickLbState state);
    void ResetNormalState();

private:
     void ApplyIcon();

     QString    _normal;
     QString _normal_hover;
     QString _normal_press;

     QString _selected;
     QString _selected_hover;
     QString _selected_press;
    ClickLbState _curstate;

    bool    _has_icons;
    QPixmap _pix_normal;
    QPixmap _pix_selected;

signals:
    void clicked(QString, ClickLbState);
};

#endif // CLICKEDLABEL_H
