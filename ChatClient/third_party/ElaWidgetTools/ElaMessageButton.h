#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELAMESSAGEBUTTON_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELAMESSAGEBUTTON_H_

#include <QPushButton>
#include <QWidget>

#include "ElaWidgetToolsDef.h"
#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"
class ElaMessageButtonPrivate;
class ELA_EXPORT ElaMessageButton : public QPushButton
{
    Q_OBJECT
    Q_Q_CREATE(ElaMessageButton)
    Q_PROPERTY_CREATE_Q_H(int, BorderRadius)
    Q_PROPERTY_REF_CREATE_Q_H(QString, BarTitle);
    Q_PROPERTY_REF_CREATE_Q_H(QString, BarText);
    Q_PROPERTY_CREATE_Q_H(int, DisplayMsec);
    Q_PROPERTY_CREATE_Q_H(QWidget*, MessageTargetWidget)
    Q_PROPERTY_CREATE_Q_H(ElaMessageBarType::MessageMode, MessageMode);
    Q_PROPERTY_CREATE_Q_H(ElaMessageBarType::PositionPolicy, PositionPolicy);

public:
    explicit ElaMessageButton(QWidget* parent = nullptr);
    explicit ElaMessageButton(const QString& text, QWidget* parent = nullptr);
    ~ElaMessageButton();

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELAMESSAGEBUTTON_H_
