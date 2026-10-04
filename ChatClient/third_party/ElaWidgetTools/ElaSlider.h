#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELASLIDER_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELASLIDER_H_

#include <QSlider>

#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"
class ELA_EXPORT ElaSlider : public QSlider
{
    Q_OBJECT
public:
    explicit ElaSlider(QWidget* parent = nullptr);
    explicit ElaSlider(Qt::Orientation orientation, QWidget* parent = nullptr);
    ~ElaSlider();
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELASLIDER_H_
