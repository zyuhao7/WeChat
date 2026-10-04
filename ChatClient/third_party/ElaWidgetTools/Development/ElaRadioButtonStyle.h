#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELARADIOBUTTONSTYLE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELARADIOBUTTONSTYLE_H_

#include <QProxyStyle>

#include "ElaWidgetToolsDef.h"
class ElaRadioButtonStyle : public QProxyStyle
{
    Q_OBJECT
public:
    explicit ElaRadioButtonStyle(QStyle* style = nullptr);
    ~ElaRadioButtonStyle();
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override;

private:
    ElaThemeType::ThemeMode _themeMode;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELARADIOBUTTONSTYLE_H_

