#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAPLAINTEXTEDITSTYLE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAPLAINTEXTEDITSTYLE_H_

#include <QProxyStyle>

#include "ElaWidgetToolsDef.h"
class ElaPlainTextEditStyle : public QProxyStyle
{
    Q_OBJECT
    Q_PROPERTY_CREATE(qreal, ExpandMarkWidth)
public:
    explicit ElaPlainTextEditStyle(QStyle* style = nullptr);
    ~ElaPlainTextEditStyle();
    void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;

private:
    ElaThemeType::ThemeMode _themeMode;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAPLAINTEXTEDITSTYLE_H_

