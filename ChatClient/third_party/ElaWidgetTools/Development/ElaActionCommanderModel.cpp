#include "ElaActionCommanderModel.h"
#include <QDebug>

#include "ElaActionCommander.h"
#include "ElaActionCommanderNode.h"
#include <QTime>
ElaActionCommanderModel::ElaActionCommanderModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    _rootNode = new ElaActionCommanderNode(nullptr);
    _rootNode->setIsRootNode(true);
}

ElaActionCommanderModel::~ElaActionCommanderModel()
{
    delete _rootNode;
}

QModelIndex ElaActionCommanderModel::parent(const QModelIndex& child) const
{
    if (!child.isValid())
    {
        return {};
    }
    const auto childNode = static_cast<ElaActionCommanderNode*>(child.internalPointer());
    ElaActionCommanderNode* parentNode = childNode->getParentNode();
    if (parentNode == _rootNode)
    {
        return {};
    }
    if (parentNode == nullptr)
    {
        return {};
    }
    return createIndex(parentNode->getRow(), 0, parentNode);
}

QModelIndex ElaActionCommanderModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
    {
        return {};
    }
    ElaActionCommanderNode* parentNode;
    if (!parent.isValid())
    {
        parentNode = _rootNode;
    }
    else
    {
        parentNode = static_cast<ElaActionCommanderNode*>(parent.internalPointer());
    }
    ElaActionCommanderNode* childNode = nullptr;
    if (parentNode->getChildNodes().count() > row)
    {
        childNode = parentNode->getChildNodes()[row];
    }
    if (childNode)
    {
        childNode->setModelIndex(createIndex(row, column, childNode));
        return childNode->getModelIndex();
    }
    return {};
}

int ElaActionCommanderModel::rowCount(const QModelIndex& parent) const
{
    ElaActionCommanderNode* parentNode;
    if (parent.column() > 0)
    {
        return 0;
    }
    if (!parent.isValid())
    {
        parentNode = _rootNode;
    }
    else
    {
        parentNode = static_cast<ElaActionCommanderNode*>(parent.internalPointer());
    }
    if (!parentNode)
    {
        return 0;
    }
    return parentNode->getChildNodes().count();
}

int ElaActionCommanderModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant ElaActionCommanderModel::data(const QModelIndex& index, int role) const
{
    if (index.column() == 0)
    {
        switch (role)
        {
        case Qt::DisplayRole:
        {
            if (const auto node = static_cast<ElaActionCommanderNode*>(index.internalPointer()))
            {
                // 空出图标位置
                if (node->getIsCommandNode())
                {
                    return "   " + node->getNodeName();
                }
                return node->getNodeName();
            }
            return {};
        }
        case Qt::UserRole:
        {
            if (const auto node = static_cast<ElaActionCommanderNode*>(index.internalPointer()))
            {
                return node->getMessageMode();
            }
            return {};
        }
        default:
        {
            break;
        }
        }
    }
    return {};
}

void ElaActionCommanderModel::addCommand(ElaActionCommand* command)
{
    if (!command || _commanderNodeMap.contains(command))
    {
        return;
    }
    beginInsertRows(_rootNode->getModelIndex(), _commanderNodeMap.count(), _commanderNodeMap.count());
    auto commandNode = new ElaActionCommanderNode(_rootNode);

    commandNode->setMessageMode(command->getMessageMode());
    commandNode->setIsCommandNode(true);
    int nodeIndex = 1;
    if (!_rootNode->getChildNodes().isEmpty())
    {
        nodeIndex = _rootNode->getChildNodes().last()->getNodeIndex() + 1;
    }
    commandNode->setNodeIndex(nodeIndex);
    commandNode->setNodeIndexText(QString("%1").arg(nodeIndex, 3, 10, QChar('0')));
    commandNode->setNodeCreateTime(QTime::currentTime().toString("hh:mm:ss"));
    // 主要行
    commandNode->setNodeName(QString("%1 [%2] %3").arg(commandNode->getNodeIndexText()).arg(commandNode->getNodeCreateTime()).arg(command->getCommandDesc()));
    _rootNode->appendChildNode(commandNode);
    endInsertRows();
    // 次要行
    auto commandTypeNode = new ElaActionCommanderNode(commandNode);
    commandNode->appendChildNode(commandTypeNode);
    commandTypeNode->setNodeName(QString("命令类型: %1").arg(command->metaObject()->className()));
    auto commandStateNode = new ElaActionCommanderNode(commandNode);
    commandNode->appendChildNode(commandStateNode);
    commandStateNode->setNodeName(QString("命令状态: %1").arg("已执行"));
    auto commandResultNode = new ElaActionCommanderNode(commandNode);
    commandNode->appendChildNode(commandResultNode);
    switch (commandNode->getMessageMode())
    {
    case ElaMessageBarType::Information:
    case ElaMessageBarType::Success:
    {
        commandResultNode->setNodeName(QString("执行结果: %1").arg("成功"));
        break;
    }
    case ElaMessageBarType::Warning:
    {
        commandResultNode->setNodeName(QString("执行结果: %1").arg("警告"));
        break;
    }
    case ElaMessageBarType::Error:
    {
        commandResultNode->setNodeName(QString("执行结果: %1").arg("错误"));
        break;
    }
    }
    _commanderNodeMap.insert(command, commandNode);
    _updateNodeModelIndex();
}

void ElaActionCommanderModel::removeCommand(ElaActionCommand* command)
{
    if (!command || !_commanderNodeMap.contains(command))
    {
        return;
    }
    auto commandNode = _commanderNodeMap[command];
    _commanderNodeMap.remove(command);
    int removeRow = _rootNode->getChildNodes().indexOf(commandNode);
    beginRemoveRows(_rootNode->getModelIndex(), removeRow, removeRow);
    _rootNode->removeChildNode(commandNode);
    endRemoveRows();
    delete commandNode;
    _updateNodeModelIndex();
}

void ElaActionCommanderModel::updateCommand(ElaActionCommand* command, bool isRedo)
{
    if (!command || !_commanderNodeMap.contains(command))
    {
        return;
    }
    auto commandNode = _commanderNodeMap[command];
    commandNode->setMessageMode(command->getMessageMode());
    commandNode->setNodeName(QString("%1 [%2] %3").arg(commandNode->getNodeIndexText()).arg(commandNode->getNodeCreateTime()).arg(command->getCommandDesc()));
    // 次要行
    auto commandTypeNode = commandNode->getChildNodes()[0];
    commandTypeNode->setNodeName(QString("命令类型: %1").arg(command->metaObject()->className()));
    auto commandStateNode = commandNode->getChildNodes()[1];
    commandStateNode->setNodeName(QString("命令状态: %1").arg(isRedo ? "已执行" : "已撤销"));
    auto commandResultNode = commandNode->getChildNodes()[2];
    switch (commandNode->getMessageMode())
    {
    case ElaMessageBarType::Information:
    case ElaMessageBarType::Success:
    {
        commandResultNode->setNodeName(QString("执行结果: %1").arg("成功"));
        break;
    }
    case ElaMessageBarType::Warning:
    {
        commandResultNode->setNodeName(QString("执行结果: %1").arg("警告"));
        break;
    }
    case ElaMessageBarType::Error:
    {
        commandResultNode->setNodeName(QString("执行结果: %1").arg("错误"));
        break;
    }
    }
}

void ElaActionCommanderModel::clearCommand()
{
    beginResetModel();
    _rootNode->setChildNodes({});
    for (const auto node: _commanderNodeMap)
    {
        delete node;
    }
    _commanderNodeMap.clear();
    endResetModel();
    _updateNodeModelIndex();
}

const QModelIndex& ElaActionCommanderModel::getCommandModelIndex(ElaActionCommand* command)
{
    static QModelIndex invalidModelIndex;
    if (!_commanderNodeMap.contains(command))
    {
        return invalidModelIndex;
    }
    return _commanderNodeMap[command]->getModelIndex();
}

void ElaActionCommanderModel::_updateNodeModelIndex(const QModelIndex& parentIndex)
{
    int indexRowCount = rowCount(parentIndex);
    for (int row = 0; row < indexRowCount; ++row)
    {
        QModelIndex modelIndex = index(row, 0, parentIndex);
        const auto childNode = static_cast<ElaActionCommanderNode*>(modelIndex.internalPointer());
        childNode->setModelIndex(modelIndex);
        if (hasChildren(modelIndex))
        {
            _updateNodeModelIndex(modelIndex);
        }
    }
}