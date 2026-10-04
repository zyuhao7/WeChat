#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELALCDNUMBER_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELALCDNUMBER_H_

#include <QLCDNumber>

#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"

class ElaLCDNumberPrivate;
class ELA_EXPORT ElaLCDNumber : public QLCDNumber
{
    Q_OBJECT
    Q_Q_CREATE(ElaLCDNumber)
    Q_PROPERTY_CREATE_Q_H(bool, IsUseAutoClock)
    Q_PROPERTY_REF_CREATE_Q_H(QString, AutoClockFormat)
    Q_PROPERTY_CREATE_Q_H(bool, IsTransparent)

public:
    explicit ElaLCDNumber(QWidget* parent = nullptr);
    explicit ElaLCDNumber(uint numDigits, QWidget* parent = nullptr);
    ~ElaLCDNumber() override;

protected:
    virtual void paintEvent(QPaintEvent* event) override;
};

#endif //ELAWORKSPACE_ELAWIDGETTOOLS_ELALCDNUMBER_H_
