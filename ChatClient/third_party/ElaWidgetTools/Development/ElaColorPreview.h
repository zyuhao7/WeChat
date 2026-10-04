#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACOLORPREVIEW_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACOLORPREVIEW_H_

#include <QWidget>

#include "ElaWidgetToolsDef.h"
class ElaColorPreview : public QWidget
{
    Q_OBJECT
    Q_PRIVATE_REF_CREATE(QColor, BaseColor)
    Q_PRIVATE_CREATE(ElaThemeType::ThemeMode, ThemeMode)
public:
    explicit ElaColorPreview(QWidget* parent = nullptr);
    ~ElaColorPreview();

protected:
    virtual void paintEvent(QPaintEvent* event) override;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACOLORPREVIEW_H_

