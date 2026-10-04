#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELACALENDARPICKER_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELACALENDARPICKER_H_

#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"
#include <QDate>
#include <QPushButton>
class ElaCalendarPickerPrivate;
class ELA_EXPORT ElaCalendarPicker : public QPushButton
{
    Q_OBJECT
    Q_Q_CREATE(ElaCalendarPicker)
    Q_PROPERTY_CREATE_Q_H(int, BorderRadius)
    Q_PRIVATE_CREATE_Q_H(QDate, SelectedDate)
public:
    explicit ElaCalendarPicker(QWidget* parent = nullptr);
    ~ElaCalendarPicker() override;

Q_SIGNALS:
    Q_SIGNAL void selectedDateChanged(QDate date);

protected:
    virtual void paintEvent(QPaintEvent* event) override;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELACALENDARPICKER_H_
