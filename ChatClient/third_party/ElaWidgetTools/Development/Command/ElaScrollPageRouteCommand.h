#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_COMMAND_ELASCROLLPAGEROUTECOMMAND_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_COMMAND_ELASCROLLPAGEROUTECOMMAND_H_

#include "ElaActionCommander.h"

class ElaScrollPagePrivate;
class ElaScrollPageRouteCommand : public ElaActionCommand
{
    Q_OBJECT
    Q_PRIVATE_CREATE(bool, IsBreadcrumbClicked)
    Q_PRIVATE_CREATE(ElaScrollPagePrivate*, ScrollPagePrivate)
    Q_PRIVATE_REF_CREATE(int, UndoPageIndex);
    Q_PRIVATE_REF_CREATE(int, RedoPageIndex);

    Q_PRIVATE_REF_CREATE(QStringList, UndoBreadcrumbList);
    Q_PRIVATE_REF_CREATE(QStringList, RedoBreadcrumbList);

public:
    explicit ElaScrollPageRouteCommand(QObject* parent = nullptr);
    ~ElaScrollPageRouteCommand() override;

    void undo() override;
    void redo() override;
};

#endif //ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_COMMAND_ELASCROLLPAGEROUTECOMMAND_H_

