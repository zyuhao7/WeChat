#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELABREADCRUMBBAR_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELABREADCRUMBBAR_H_
#include <QWidget>

#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"

class ElaBreadcrumbBarPrivate;
class ELA_EXPORT ElaBreadcrumbBar : public QWidget
{
    Q_OBJECT
    Q_Q_CREATE(ElaBreadcrumbBar)
    Q_PROPERTY_CREATE_Q_H(int, TextPixelSize)
    Q_PROPERTY_CREATE_Q_H(bool, IsAutoRemove)
public:
    explicit ElaBreadcrumbBar(QWidget* parent = nullptr);
    ~ElaBreadcrumbBar() override;
    void setBreadcrumbList(const QStringList& breadcrumbList);
    QStringList appendBreadcrumb(QString breadcrumb);
    QStringList removeBreadcrumb(QString breadcrumb);

    int getBreadcrumbListCount() const;
    const QStringList& getBreadcrumbList() const;

Q_SIGNALS:
    Q_SIGNAL void breadcrumbClicked(QString breadcrumb, QStringList lastBreadcrumbList);
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELABREADCRUMBBAR_H_
