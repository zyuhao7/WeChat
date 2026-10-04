#include "ElaRibbonBarModel.h"
#include <QMenu>
#include <QWidget>
ElaRibbonBarModel::ElaRibbonBarModel(QObject* parent)
    : QAbstractListModel(parent)
{
    // 根组
    ElaRibbonGroupData data;
    _ribbonDataList.append(data);
}

ElaRibbonBarModel::~ElaRibbonBarModel()
{
}

QAction* ElaRibbonBarModel::addRibbonAction(const QString& actionName)
{
    auto action = new QAction(actionName, this);
    beginResetModel();
    _ribbonDataList[0].ribbonDataList.append(action);
    endResetModel();
    return action;
}

QAction* ElaRibbonBarModel::addRibbonAction(const QString& actionName, ElaIconType::IconName icon)
{
    auto action = new QAction(actionName, this);
    beginResetModel();
    action->setProperty("ElaRibbonIcon", QChar(icon));
    _ribbonDataList[0].ribbonDataList.append(action);
    endResetModel();
    return action;
}

QAction* ElaRibbonBarModel::addRibbonAction(const QString& actionName, const QIcon& icon)
{
    auto action = new QAction(icon, actionName, this);
    beginResetModel();
    _ribbonDataList[0].ribbonDataList.append(action);
    endResetModel();
    return action;
}

QAction* ElaRibbonBarModel::addRibbonAction(const QString& groupName, const QString& actionName)
{
    auto action = new QAction(actionName, this);
    action->setProperty("ElaRibbonGroupName", groupName);
    // 查找指定Group
    beginResetModel();
    bool isFindGroup = false;
    for (auto& groupListData: _ribbonDataList)
    {
        if (groupListData.groupName == groupName)
        {
            isFindGroup = true;
            groupListData.ribbonDataList.append(action);
            break;
        }
    }
    if (!isFindGroup)
    {
        ElaRibbonGroupData groupData;
        groupData.groupName = groupName;
        groupData.ribbonDataList.append(action);
        _ribbonDataList.append(groupData);
    }
    endResetModel();
    return action;
}

QAction* ElaRibbonBarModel::addRibbonAction(const QString& groupName, const QString& actionName, ElaIconType::IconName icon)
{
    auto action = new QAction(actionName, this);
    action->setProperty("ElaRibbonGroupName", groupName);
    action->setProperty("ElaRibbonIcon", QChar(icon));
    // 查找指定Group
    beginResetModel();
    bool isFindGroup = false;
    for (auto& groupListData: _ribbonDataList)
    {
        if (groupListData.groupName == groupName)
        {
            isFindGroup = true;
            groupListData.ribbonDataList.append(action);
            break;
        }
    }
    if (!isFindGroup)
    {
        ElaRibbonGroupData groupData;
        groupData.groupName = groupName;
        groupData.ribbonDataList.append(action);
        _ribbonDataList.append(groupData);
    }
    endResetModel();
    return action;
}

QAction* ElaRibbonBarModel::addRibbonAction(const QString& groupName, const QString& actionName, const QIcon& icon)
{
    auto action = new QAction(icon, actionName, this);
    action->setProperty("ElaRibbonGroupName", groupName);
    // 查找指定Group
    beginResetModel();
    bool isFindGroup = false;
    for (auto& groupListData: _ribbonDataList)
    {
        if (groupListData.groupName == groupName)
        {
            isFindGroup = true;
            groupListData.ribbonDataList.append(action);
            break;
        }
    }
    if (!isFindGroup)
    {
        ElaRibbonGroupData groupData;
        groupData.groupName = groupName;
        groupData.ribbonDataList.append(action);
        _ribbonDataList.append(groupData);
    }
    endResetModel();
    return action;
}

void ElaRibbonBarModel::removeRibbonAction(QAction* action)
{
    if (!action)
    {
        return;
    }
    auto ribbonGroupName = action->property("ElaRibbonGroupName").toString();
    beginResetModel();
    for (auto& groupData: _ribbonDataList)
    {
        if (groupData.groupName == ribbonGroupName)
        {
            groupData.ribbonDataList.removeAll(action);
            if (auto indexWidget = action->property("ElaRibbonWidget").value<QWidget*>())
            {
                indexWidget->deleteLater();
            }
            action->deleteLater();
            break;
        }
    }
    endResetModel();
}

void ElaRibbonBarModel::removeRibbonAction(const QString& groupName, const QString& actionName)
{
    beginResetModel();
    for (auto& groupData: _ribbonDataList)
    {
        if (groupData.groupName == groupName)
        {
            for (auto action: groupData.ribbonDataList)
            {
                if (action->text() == actionName)
                {
                    groupData.ribbonDataList.removeOne(action);
                    action->deleteLater();
                    break;
                }
            }
        }
    }
    endResetModel();
}

const QList<ElaRibbonGroupData>& ElaRibbonBarModel::getRibbonGroupData() const
{
    return _ribbonDataList;
}

QAction* ElaRibbonBarModel::getRibbonAction(int index) const
{
    if (index < 0)
    {
        return nullptr;
    }
    for (const auto& groupData: _ribbonDataList)
    {
        if (groupData.ribbonDataList.count() > index)
        {
            return groupData.ribbonDataList[index];
        }
        index -= groupData.ribbonDataList.count();
    }
    return nullptr;
}

QModelIndex ElaRibbonBarModel::getRibbonActionModelIndex(QAction* ribbonAction) const
{
    if (!ribbonAction)
    {
        return {};
    }
    int actionIndex = 0;
    for (const auto& groupData: _ribbonDataList)
    {
        for (const auto action: groupData.ribbonDataList)
        {
            if (action == ribbonAction)
            {
                return this->index(actionIndex);
            }
            actionIndex++;
        }
    }
    return {};
}

QList<QAction*> ElaRibbonBarModel::getRibbonActions()
{
    QList<QAction*> ribbonActionList;
    for (const auto& ribbonGroup: _ribbonDataList)
    {
        ribbonActionList.append(ribbonGroup.ribbonDataList);
    }
    return ribbonActionList;
}

QList<QAction*> ElaRibbonBarModel::getRibbonActions(const QString& groupName)
{
    QList<QAction*> ribbonActionList;
    for (const auto& ribbonGroup: _ribbonDataList)
    {
        if (ribbonGroup.groupName == groupName)
        {
            ribbonActionList.append(ribbonGroup.ribbonDataList);
        }
    }
    return ribbonActionList;
}

QList<QAction*> ElaRibbonBarModel::getRibbonActions(const QString& groupName, const QString& actionName)
{
    QList<QAction*> ribbonActionList;
    for (const auto& groupData: _ribbonDataList)
    {
        if (groupData.groupName == groupName)
        {
            for (auto action: groupData.ribbonDataList)
            {
                if (action->text() == actionName)
                {
                    ribbonActionList.append(action);
                }
            }
        }
    }
    return ribbonActionList;
}

int ElaRibbonBarModel::rowCount(const QModelIndex& parent) const
{
    int rowCount = 0;
    for (auto& ribbonGroup: _ribbonDataList)
    {
        rowCount += ribbonGroup.ribbonDataList.count();
    }
    return rowCount;
}

QVariant ElaRibbonBarModel::data(const QModelIndex& index, int role) const
{
    switch (role)
    {
    case Qt::DisplayRole:
    {
        auto action = getRibbonAction(index.row());
        if (!action)
        {
            return {};
        }
        const auto actionText = action->text();
        // 拉长间距
        return action->menu() ? actionText + "         " : actionText + "      ";
    }
    case Qt::UserRole:
    {
        return QVariant::fromValue(getRibbonAction(index.row()));
    }
    default:
    {
        break;
    }
    }
    return {};
}
