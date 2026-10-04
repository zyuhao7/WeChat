#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELADOCKWIDGETTITLEBAR_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELADOCKWIDGETTITLEBAR_H_

#include <QDockWidget>
#include <QLabel>
#include <QWidget>

#include "ElaWidgetToolsDef.h"
class ElaText;
class ElaIconButton;
class ElaDockWidget;
class ElaDockWidgetTitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit ElaDockWidgetTitleBar(QWidget* parent = nullptr);
    ~ElaDockWidgetTitleBar();

    Q_SLOT void onFloatButtonClicked();
    Q_SLOT void onCloseButtonClicked();

protected:
    virtual void paintEvent(QPaintEvent* event) override;

private:
    friend class ElaDockWidgetPrivate;
    ElaThemeType::ThemeMode _themeMode;
    ElaDockWidget* _dockWidget{nullptr};
    QLabel* _iconLabel{nullptr};
    ElaText* _titleLabel{nullptr};
    ElaIconButton* _floatButton{nullptr};
    ElaIconButton* _closeButton{nullptr};
    void _setVisibleFromFeatures(QDockWidget::DockWidgetFeatures features);
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELADOCKWIDGETTITLEBAR_H_

