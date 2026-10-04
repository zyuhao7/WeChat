#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELABREADCRUMBBARMODEL_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELABREADCRUMBBARMODEL_H_

#include <QAbstractListModel>

class ElaBreadcrumbBarModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit ElaBreadcrumbBarModel(QObject* parent = nullptr);
    ~ElaBreadcrumbBarModel();

    void appendBreadcrumb(const QString& breadcrumb);
    void removeBreadcrumb(const QString& breadcrumb);
    void removeBreadcrumb(int index);

    void setBreadcrumbList(const QStringList& breadcrumbList);

    int getBreadcrumbListCount() const;
    const QStringList& getBreadcrumbList() const;

protected:
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

private:
    QStringList _breadcrumbList;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELABREADCRUMBBARMODEL_H_

