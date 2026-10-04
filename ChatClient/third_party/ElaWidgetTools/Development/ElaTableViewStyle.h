#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELATABLEVIEWSTYLE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELATABLEVIEWSTYLE_H_
#include <QModelIndex>
#include <QProxyStyle>

#include "ElaWidgetToolsDef.h"
class QStyleOptionViewItem;
class ElaTableViewStyle : public QProxyStyle
{
    Q_OBJECT
    Q_PROPERTY_CREATE(int, HeaderMargin)
    Q_PROPERTY_CREATE(int, CurrentHoverRow)
public:
    explicit ElaTableViewStyle(QStyle* style = nullptr);
    ~ElaTableViewStyle();
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override;

private:
    ElaThemeType::ThemeMode _themeMode;
    int _horizontalPadding{11};
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELATABLEVIEWSTYLE_H_

