#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAACTIONCOMMANDERNODE_H
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAACTIONCOMMANDERNODE_H

#include "ElaWidgetToolsDef.h"
#include "ElaPropertyMacro.h"
#include <QModelIndex>
#include <QObject>

class ElaActionCommanderNode : public QObject
{
    Q_OBJECT
    Q_PRIVATE_REF_CREATE(QString, NodeName)
    Q_PRIVATE_CREATE(int, NodeIndex)
    Q_PRIVATE_REF_CREATE(QString, NodeIndexText)
    Q_PRIVATE_REF_CREATE(QString, NodeCreateTime)
    Q_PRIVATE_REF_CREATE(QList<ElaActionCommanderNode*>, ChildNodes)
    Q_PRIVATE_CREATE(ElaActionCommanderNode*, ParentNode)
    Q_PRIVATE_REF_CREATE(QModelIndex, ModelIndex)
    Q_PRIVATE_CREATE(bool, IsRootNode)
    Q_PRIVATE_CREATE(bool, IsCommandNode)
    Q_PRIVATE_CREATE(bool, IsCurrentNode)
    Q_PRIVATE_CREATE(ElaMessageBarType::MessageMode, MessageMode)
public:
    explicit ElaActionCommanderNode(ElaActionCommanderNode* parent = nullptr);
    ~ElaActionCommanderNode() override;

    void appendChildNode(ElaActionCommanderNode* childNode);
    void removeChildNode(ElaActionCommanderNode* childNode);

    int getRow() const;
};

#endif //ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAACTIONCOMMANDERNODE_H
