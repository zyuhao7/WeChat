#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELASTATUSBAR_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELASTATUSBAR_H_

#include <QStatusBar>

#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"
class ELA_EXPORT ElaStatusBar : public QStatusBar
{
    Q_OBJECT
public:
    explicit ElaStatusBar(QWidget* parent = nullptr);
    ~ElaStatusBar() override;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELASTATUSBAR_H_
