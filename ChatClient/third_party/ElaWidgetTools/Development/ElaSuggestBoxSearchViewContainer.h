#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELASUGGESTBOXSEARCHVIEWCONTAINER_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELASUGGESTBOXSEARCHVIEWCONTAINER_H_

#include <QWidget>

#include "ElaWidgetToolsDef.h"
class ElaSuggestBoxSearchViewContainer : public QWidget
{
    Q_OBJECT
public:
    explicit ElaSuggestBoxSearchViewContainer(QWidget* parent = nullptr);
    ~ElaSuggestBoxSearchViewContainer();

protected:
    virtual void paintEvent(QPaintEvent* event) override;

private:
    ElaThemeType::ThemeMode _themeMode;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELASUGGESTBOXSEARCHVIEWCONTAINER_H_

