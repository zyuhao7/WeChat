#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACALENDARPICKERCONTAINER_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACALENDARPICKERCONTAINER_H_

#include <QWidget>

#include "ElaWidgetToolsDef.h"
class ElaCalendarPickerContainer : public QWidget
{
    Q_OBJECT
public:
    explicit ElaCalendarPickerContainer(QWidget* parent = nullptr);
    ~ElaCalendarPickerContainer() override;

protected:
    virtual void paintEvent(QPaintEvent* event) override;

private:
    ElaThemeType::ThemeMode _themeMode;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACALENDARPICKERCONTAINER_H_

