#include "ElaActionCommanderNode.h"

#include <QDebug>
#include <QUuid>
ElaActionCommanderNode::ElaActionCommanderNode(ElaActionCommanderNode* parent)
    : QObject(nullptr)
{
    _pIsRootNode = false;
    _pIsCommandNode = false;
    _pParentNode = parent;
    _pIsCurrentNode = false;
    _pNodeIndex = 1;
    _pMessageMode = ElaMessageBarType::Success;
}

ElaActionCommanderNode::~ElaActionCommanderNode()
{
    // 不使用QT的继承链删除 手动释放 一般情况下此时_pChildNodes已经为空
    for (const auto childNode: _pChildNodes)
    {
        delete childNode;
    }
}

void ElaActionCommanderNode::appendChildNode(ElaActionCommanderNode* childNode)
{
    _pChildNodes.append(childNode);
}

void ElaActionCommanderNode::removeChildNode(ElaActionCommanderNode* childNode)
{
    _pChildNodes.removeOne(childNode);
}

int ElaActionCommanderNode::getRow() const
{
    if (_pParentNode)
    {
        return _pParentNode->getChildNodes().indexOf(const_cast<ElaActionCommanderNode*>(this));
    }
    return 0;
}
