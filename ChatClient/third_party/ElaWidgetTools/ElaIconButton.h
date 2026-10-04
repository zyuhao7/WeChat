#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELAICONBUTTON_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELAICONBUTTON_H_

#include <QPushButton>

#include "ElaWidgetToolsDef.h"
#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"
class ElaIconButtonPrivate;
class ELA_EXPORT ElaIconButton : public QPushButton
{
    Q_OBJECT
    Q_Q_CREATE(ElaIconButton)
    Q_PROPERTY_CREATE_Q_H(int, BorderRadius)
    Q_PROPERTY_CREATE_Q_H(qreal, Opacity);
    Q_PROPERTY_REF_CREATE_Q_H(QColor, LightHoverColor);
    Q_PROPERTY_REF_CREATE_Q_H(QColor, DarkHoverColor);
    Q_PROPERTY_REF_CREATE_Q_H(QColor, LightIconColor);
    Q_PROPERTY_REF_CREATE_Q_H(QColor, DarkIconColor);
    Q_PROPERTY_REF_CREATE_Q_H(QColor, LightHoverIconColor);
    Q_PROPERTY_REF_CREATE_Q_H(QColor, DarkHoverIconColor);
    Q_PROPERTY_CREATE_Q_H(bool, IsSelected);

public:
    ElaIconButton(const QPixmap& pix, QWidget* parent = nullptr);
    ElaIconButton(ElaIconType::IconName awesome, QWidget* parent = nullptr);
    ElaIconButton(ElaIconType::IconName awesome, int pixelSize, QWidget* parent = nullptr);
    ElaIconButton(ElaIconType::IconName awesome, int pixelSize, int fixedWidth, int fixedHeight, QWidget* parent = nullptr);
    ~ElaIconButton();
    void setAwesome(ElaIconType::IconName awesome);
    ElaIconType::IconName getAwesome() const;

    void setPixmap(const QPixmap& pix);

protected:
    virtual bool event(QEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELAICONBUTTON_H_
