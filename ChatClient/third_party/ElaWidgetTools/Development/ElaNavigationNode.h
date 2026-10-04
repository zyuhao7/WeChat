#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELANAVIGATIONNODE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELANAVIGATIONNODE_H_

#include <QModelIndex>
#include <QObject>

#include "ElaWidgetToolsDef.h"

class ElaNavigationNode : public QObject
{
    Q_OBJECT
    Q_PRIVATE_REF_CREATE(QList<ElaNavigationNode*>, ChildrenNodes)
    Q_PRIVATE_CREATE(ElaNavigationNode*, ParentNode)
    Q_PRIVATE_CREATE(ElaIconType::IconName, Awesome)
    Q_PRIVATE_REF_CREATE(QModelIndex, ModelIndex)
    Q_PRIVATE_CREATE(int, KeyPoints)
    Q_PRIVATE_CREATE(int, Depth)
    Q_PRIVATE_CREATE(bool, IsRootNode)
    Q_PRIVATE_CREATE(bool, IsFooterNode)
    Q_PRIVATE_CREATE(bool, IsHasFooterPage)
    Q_PRIVATE_CREATE(bool, IsExpanderNode)
    Q_PRIVATE_CREATE(bool, IsCategoryNode)
    Q_PRIVATE_CREATE(bool, IsVisible)
    Q_PRIVATE_REF_CREATE(QString, NodeTitle)
public:
    explicit ElaNavigationNode(const QString& nodeTitle, ElaNavigationNode* parent = nullptr);
    ~ElaNavigationNode() override;

    QString getNodeKey() const;

    void setIsExpanded(bool isExpanded);
    bool getIsExpanded() const;

    void setChildVisible(bool isVisible);
    bool getIsHasChild() const;
    bool getIsHasPageChild() const;

    void appendChildNode(ElaNavigationNode* childNode);
    void removeChildNode(ElaNavigationNode* childNode);

    bool getIsChildHasKeyPoints() const;

    ElaNavigationNode* getOriginalNode();
    bool getIsChildNode(ElaNavigationNode* node);

    int getRow() const;
    int getRowExceptCategoryNodes() const;

    QList<ElaNavigationNode*> getExceptCategoryNodes();

private:
    QString _nodeKey = "";
    bool _isExpanded{false};
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELANAVIGATIONNODE_H_

