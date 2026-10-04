#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELABREADCRUMBBARPRIVATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELABREADCRUMBBARPRIVATE_H_

#include <QObject>

#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"
class ElaBaseListView;
class ElaBreadcrumbBarModel;
class ElaBreadcrumbBarDelegate;

class ElaBreadcrumbBar;
class ElaBreadcrumbBarPrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaBreadcrumbBar)
    Q_PROPERTY_CREATE_D(int, TextPixelSize)
    Q_PROPERTY_CREATE_D(bool, IsAutoRemove)
public:
    explicit ElaBreadcrumbBarPrivate(QObject* parent = nullptr);
    ~ElaBreadcrumbBarPrivate();

private:
    ElaBreadcrumbBarModel* _listModel{nullptr};
    ElaBreadcrumbBarDelegate* _listDelegate{nullptr};
    ElaBaseListView* _listView{nullptr};
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELABREADCRUMBBARPRIVATE_H_

