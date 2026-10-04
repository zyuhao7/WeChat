#include "ElaActionCommanderPrivate.h"

#include "ElaActionCommander.h"
#include "ElaActionCommanderModel.h"
#include "ElaActionCommanderView.h"
#include "ElaMessageBar.h"

ElaActionCommanderPrivate::ElaActionCommanderPrivate(QObject* parent)
    : QObject{parent}
{
}

ElaActionCommanderPrivate::~ElaActionCommanderPrivate()
{
}

void ElaActionCommanderPrivate::_showCommandDesc(const ElaActionCommand* command, bool isRedo, bool isExecute) const
{
    if (!command->getIsShowMessage() || !_pIsMessageDisplayEnable)
    {
        return;
    }
    QString commandTitle = isExecute ? "执行" : isRedo ? "重做"
                                                       : "撤回";
    switch (command->getMessageMode())
    {
    case ElaMessageBarType::Success:
    {
        ElaMessageBar::success(ElaMessageBarType::TopRight, commandTitle, command->getCommandDesc(), _pMessageDisplayMesc, command->getMessageWidget());
        break;
    }
    case ElaMessageBarType::Warning:
    {
        ElaMessageBar::warning(ElaMessageBarType::TopRight, commandTitle, command->getCommandDesc(), _pMessageDisplayMesc, command->getMessageWidget());
        break;
    }
    case ElaMessageBarType::Information:
    {
        ElaMessageBar::information(ElaMessageBarType::TopRight, commandTitle, command->getCommandDesc(), _pMessageDisplayMesc, command->getMessageWidget());
        break;
    }
    case ElaMessageBarType::Error:
    {
        ElaMessageBar::error(ElaMessageBarType::TopRight, commandTitle, command->getCommandDesc(), _pMessageDisplayMesc, command->getMessageWidget());
        break;
    }
    }
}

bool ElaActionCommanderPrivate::_executeCommand(const QString& domainName, ElaActionCommand* command)
{
    Q_Q(ElaActionCommander);
    auto& commandData = _commandDomainMap[domainName];
    if (commandData.isBusyMode)
    {
        ElaMessageBar::warning(ElaMessageBarType::TopRight, "警告", "任务管理器繁忙中!", _pMessageDisplayMesc, command->getMessageWidget());
        return false;
    }
    if (!command)
    {
        ElaMessageBar::warning(ElaMessageBarType::TopRight, "警告", "命令对象不可用!", _pMessageDisplayMesc, command->getMessageWidget());
        return false;
    }
    q->setCurrentDomain(domainName);
    command->setParent(q);
    q->clearCommand(domainName);
    command->redo();
    _showCommandDesc(command, true, true);
    return true;
}

bool ElaActionCommanderPrivate::_recordCommand(const QString& domainName, ElaActionCommand* command, bool isRedo)
{
    Q_Q(ElaActionCommander);
    auto& commandData = _commandDomainMap[domainName];
    if (commandData.isBusyMode)
    {
        ElaMessageBar::warning(ElaMessageBarType::TopRight, "警告", "任务管理器繁忙中!", _pMessageDisplayMesc, command->getMessageWidget());
        return false;
    }
    if (!command)
    {
        ElaMessageBar::warning(ElaMessageBarType::TopRight, "警告", "命令对象不可用!", _pMessageDisplayMesc, command->getMessageWidget());
        return false;
    }
    if (!_isCommanderValid)
    {
        ElaMessageBar::warning(ElaMessageBarType::TopRight, "警告", "当前不处于可编辑模式，仅支持运行命令!", _pMessageDisplayMesc, command->getMessageWidget());
        return false;
    }
    q->setCurrentDomain(domainName);
    command->setParent(q);
    if (isRedo)
    {
        // 加入时Redo一次
        command->redo();
        _showCommandDesc(command, true, true);
        // 记录时执行失败则丢弃命令 后续撤回重做时失败不进行处理
        if (command->getMessageMode() == ElaMessageBarType::Error)
        {
            delete command;
            return false;
        }
    }
    auto& commandList = commandData.commandList;
    auto commandModel = commandData.commanderModel;
    if (commandData.currentIndex <= 0 || commandList.count() == 0)
    {
        commandData.undoState = ElaActionCommanderType::UndoValid;
        commandData.redoState = ElaActionCommanderType::RedoInvalid;
        Q_EMIT q->commanderStateChanged(domainName, ElaActionCommanderType::UndoValid);
        Q_EMIT q->commanderStateChanged(domainName, ElaActionCommanderType::RedoInvalid);
    }
    else
    {
        // 超过最大命令数 则移除第一条命令
        if (commandList.count() >= _pMaxCommandCount)
        {
            auto deleteCommand = commandList[0];
            commandModel->removeCommand(deleteCommand);
            delete deleteCommand;
            commandList.removeFirst();
            commandData.currentIndex -= 1;
        }
    }
    // 当前索引不位于末尾 则清除索引后的数据
    if (commandData.currentIndex != commandList.count() - 1)
    {
        int deleteStartIndex = commandData.currentIndex + 1;
        int deleteCount = commandList.count() - commandData.currentIndex - 1;
        int deleteEndIndex = deleteStartIndex + deleteCount;
        for (int i = deleteStartIndex; i < deleteEndIndex; i++)
        {
            auto deleteCommand = commandList[i];
            commandModel->removeCommand(deleteCommand);
            delete deleteCommand;
        }
        commandList.remove(deleteStartIndex, deleteCount);
        if (commandData.currentIndex > 0)
        {
            commandData.redoState = ElaActionCommanderType::RedoInvalid;
            Q_EMIT q->commanderStateChanged(domainName, ElaActionCommanderType::RedoInvalid);
        }
    }
    commandList.append(command);
    commandData.currentIndex = commandList.count() - 1;
    commandModel->addCommand(command);
    _commanderView->clearSelection();
    _commanderView->selectionModel()->select(commandModel->getCommandModelIndex(command), QItemSelectionModel::Select | QItemSelectionModel::Rows);
    return true;
}

void ElaActionCommanderPrivate::_undoCommand(const QString& domainName, bool isShowMessage)
{
    Q_Q(ElaActionCommander);
    auto& commandData = _commandDomainMap[domainName];
    if (commandData.undoState == ElaActionCommanderType::UndoInvalid)
    {
        return;
    }
    auto& commandList = commandData.commandList;
    if (commandList.isEmpty())
    {
        return;
    }
    if (commandData.currentIndex == 0)
    {
        commandData.undoState = ElaActionCommanderType::UndoInvalid;
        Q_EMIT q->commanderStateChanged(domainName, ElaActionCommanderType::UndoInvalid);
    }
    if (commandData.currentIndex == commandList.size() - 1)
    {
        commandData.redoState = ElaActionCommanderType::RedoValid;
        Q_EMIT q->commanderStateChanged(domainName, ElaActionCommanderType::RedoValid);
    }
    auto command = commandList[commandData.currentIndex];
    command->undo();
    commandData.currentIndex -= 1;
    auto commandModel = commandData.commanderModel;
    commandModel->updateCommand(command, false);
    _commanderView->clearSelection();
    _commanderView->selectionModel()->select(commandModel->getCommandModelIndex(command), QItemSelectionModel::Select | QItemSelectionModel::Rows);
    _commanderView->viewport()->update();
    if (isShowMessage)
    {
        _showCommandDesc(command, false);
    }
}

void ElaActionCommanderPrivate::_redoCommand(const QString& domainName, bool isShowMessage)
{
    Q_Q(ElaActionCommander);
    auto& commandData = _commandDomainMap[domainName];
    auto& commandList = commandData.commandList;
    if (commandData.redoState == ElaActionCommanderType::RedoInvalid)
    {
        return;
    }
    if (commandList.isEmpty())
    {
        return;
    }
    if (commandData.currentIndex <= 0)
    {
        commandData.undoState = ElaActionCommanderType::UndoValid;
        Q_EMIT q->commanderStateChanged(domainName, ElaActionCommanderType::UndoValid);
    }
    if (commandData.currentIndex == commandList.size() - 2)
    {
        commandData.redoState = ElaActionCommanderType::RedoInvalid;
        Q_EMIT q->commanderStateChanged(domainName, ElaActionCommanderType::RedoInvalid);
    }
    if (commandData.currentIndex < commandList.size() - 1)
    {
        commandData.currentIndex += 1;
    }
    auto command = commandList[commandData.currentIndex];
    command->redo();
    auto commandModel = commandData.commanderModel;
    commandModel->updateCommand(command, true);
    _commanderView->viewport()->update();
    _commanderView->clearSelection();
    _commanderView->selectionModel()->select(commandModel->getCommandModelIndex(command), QItemSelectionModel::Select | QItemSelectionModel::Rows);
    if (isShowMessage)
    {
        _showCommandDesc(command, true);
    }
}

void ElaActionCommanderPrivate::_initDomain(ElaActionCommandDomainData& domainData) const
{
    if (domainData.commanderModel)
    {
        return;
    }
    domainData.commanderModel = new ElaActionCommanderModel(_commanderDesign);
}