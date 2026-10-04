#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACUSTOMWIDGET_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACUSTOMWIDGET_H_

#include <QDialog>

#include "ElaAppBar.h"
#include "ElaWidgetToolsDef.h"

class QVBoxLayout;
class ElaCustomWidget : public QDialog
{
    Q_OBJECT
    Q_TAKEOVER_NATIVEEVENT_H
public:
    explicit ElaCustomWidget(QWidget* parent = nullptr);
    ~ElaCustomWidget() override;

    void setCentralWidget(QWidget* widget);

protected:
    virtual void paintEvent(QPaintEvent* event) override;
    QVBoxLayout* _mainLayout{nullptr};
    ElaAppBar* _appBar{nullptr};
    QWidget* _centralWidget{nullptr};

private:
    ElaThemeType::ThemeMode _themeMode;
    ElaApplicationType::WindowDisplayMode _windowDisplayMode;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACUSTOMWIDGET_H_

