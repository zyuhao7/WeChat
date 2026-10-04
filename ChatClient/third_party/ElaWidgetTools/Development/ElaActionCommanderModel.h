#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAACTIONCOMMANDERMODEL_H
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAACTIONCOMMANDERMODEL_H

#include "ElaActionCommander.h"

#include <QAbstractItemModel>
#include <QMap>
#include <QObject>

class ElaActionCommanderNode;
class ElaActionCommanderModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ElaActionCommanderModel(QObject* parent = nullptr);
    ~ElaActionCommanderModel() override;

    QModelIndex parent(const QModelIndex& child) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    void addCommand(ElaActionCommand* command);
    void removeCommand(ElaActionCommand* command);
    void updateCommand(ElaActionCommand* command, bool isRedo);
    void clearCommand();

    const QModelIndex& getCommandModelIndex(ElaActionCommand* command);

private:
    ElaActionCommanderNode* _rootNode{nullptr};
    QMap<ElaActionCommand*, ElaActionCommanderNode*> _commanderNodeMap;
    void _updateNodeModelIndex(const QModelIndex& parentIndex = {});
};

#endif //ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAACTIONCOMMANDERMODEL_H
