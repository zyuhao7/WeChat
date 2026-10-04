#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACOLORDISPLAYMODEL_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACOLORDISPLAYMODEL_H_

#include <QAbstractListModel>
#include <QColor>
class ElaColorDisplayModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit ElaColorDisplayModel(QObject* parent = nullptr);
    ~ElaColorDisplayModel();
    void appendDisplayColor(const QList<QColor>& colorList);
    void appendDisplayColor(const QColor& color);
    void removeDisplayColor(int index);
    void replaceDisplayColor(const QColor& color, int index);

    const QList<QColor>& getDisplayColorList() const;
    QColor getDisplayColor(int index) const;

protected:
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

private:
    QList<QColor> _displayColorList;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACOLORDISPLAYMODEL_H_

