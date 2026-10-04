#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELATABLEVIEWPRIVATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELATABLEVIEWPRIVATE_H_

#include <QObject>

#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"

class ElaTableView;
class ElaTableViewStyle;
class ElaTableViewPrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaTableView)
public:
    explicit ElaTableViewPrivate(QObject* parent = nullptr);
    ~ElaTableViewPrivate();

private:
    ElaTableViewStyle* _tableViewStyle{nullptr};
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELATABLEVIEWPRIVATE_H_

