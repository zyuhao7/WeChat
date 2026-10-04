#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_COMMAND_ELANAVIGATIONROUTECOMMAND_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_COMMAND_ELANAVIGATIONROUTECOMMAND_H_

#include "ElaActionCommander.h"
class ElaNavigationBar;
class ElaNavigationRouteCommand : public ElaActionCommand
{
    Q_OBJECT
    Q_PRIVATE_CREATE(ElaNavigationBar*, NavigationBar);
    Q_PRIVATE_REF_CREATE(QString, UndoPageKey);
    Q_PRIVATE_REF_CREATE(QString, RedoPageKey);

public:
    explicit ElaNavigationRouteCommand(QObject* parent = nullptr);
    ~ElaNavigationRouteCommand() override;

    void undo() override;
    void redo() override;
};

#endif //ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_COMMAND_ELANAVIGATIONROUTECOMMAND_H_

