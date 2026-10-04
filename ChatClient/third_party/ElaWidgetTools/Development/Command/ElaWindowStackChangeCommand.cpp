#include "ElaWindowStackChangeCommand.h"

#include "ElaCentralStackedWidget.h"
#include "ElaWindow.h"
ElaWindowStackChangeCommand::ElaWindowStackChangeCommand(QObject* parent)
    : ElaActionCommand(parent)
{
}

ElaWindowStackChangeCommand::~ElaWindowStackChangeCommand()
{
}

void ElaWindowStackChangeCommand::undo()
{
    _pWindowPrivate->_centerStackedWidget->doWindowStackSwitch(_pWindowPrivate->_pStackSwitchMode, _pUndoStackIndex, false);
    Q_EMIT _pWindowPrivate->q_ptr->pCurrentStackIndexChanged();
    setCommandDesc("撤销堆栈页面切换操作");
}

void ElaWindowStackChangeCommand::redo()
{
    _pWindowPrivate->_centerStackedWidget->doWindowStackSwitch(_pWindowPrivate->_pStackSwitchMode, _pRedoStackIndex, false);
    Q_EMIT _pWindowPrivate->q_ptr->pCurrentStackIndexChanged();
    setCommandDesc("执行堆栈页面切换操作");
}