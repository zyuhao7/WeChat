#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELATOOLTIP_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELATOOLTIP_H_

#include <QWidget>

#include "ElaPropertyMacro.h"
#include "ElaWidgetToolsExport.h"
class ElaToolTipPrivate;
class ELA_EXPORT ElaToolTip : public QWidget
{
    Q_OBJECT
    Q_Q_CREATE(ElaToolTip)
    Q_PROPERTY_CREATE_Q_H(int, BorderRadius)
    Q_PROPERTY_CREATE_Q_H(int, DisplayMsec)
    Q_PROPERTY_CREATE_Q_H(int, ShowDelayMsec)
    Q_PROPERTY_CREATE_Q_H(int, HideDelayMsec)
    Q_PROPERTY_CREATE_Q_H(QString, ToolTip)
    Q_PROPERTY_CREATE_Q_H(QWidget*, CustomWidget)
public:
    explicit ElaToolTip(QWidget* parent = nullptr);
    ~ElaToolTip() override;

    void updatePos();

protected:
    virtual void paintEvent(QPaintEvent* event);
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELATOOLTIP_H_
