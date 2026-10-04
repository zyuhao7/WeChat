#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELALISTVIEW_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELALISTVIEW_H_

#include <QListView>

#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"

class ElaListViewPrivate;
class ELA_EXPORT ElaListView : public QListView
{
    Q_OBJECT
    Q_Q_CREATE(ElaListView)
    Q_PROPERTY_CREATE_Q_H(int, ItemHeight)
    Q_PROPERTY_CREATE_Q_H(bool, IsTransparent)
public:
    explicit ElaListView(QWidget* parent = nullptr);
    ~ElaListView();
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELALISTVIEW_H_
