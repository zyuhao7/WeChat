#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAWINDOWSTYLE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAWINDOWSTYLE_H_

#include <QProxyStyle>

#include "ElaWidgetToolsDef.h"
class ElaWindowStyle : public QProxyStyle
{
    Q_OBJECT
public:
    explicit ElaWindowStyle(QStyle* style = nullptr);
    ~ElaWindowStyle();
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;

private:
    ElaThemeType::ThemeMode _themeMode;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAWINDOWSTYLE_H_

