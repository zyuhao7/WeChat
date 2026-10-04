#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELAMENUBAR_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELAMENUBAR_H_

#include <QMenuBar>

#include "ElaWidgetToolsDef.h"
#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"

class ElaMenu;
class ELA_EXPORT ElaMenuBar : public QMenuBar
{
    Q_OBJECT
public:
    explicit ElaMenuBar(QWidget* parent = nullptr);
    ~ElaMenuBar();

    QAction* addMenu(QMenu* menu);
    ElaMenu* addMenu(const QString& title);
    ElaMenu* addMenu(const QIcon& icon, const QString& title);
    ElaMenu* addMenu(ElaIconType::IconName, const QString& title);

    QAction* addElaIconAction(ElaIconType::IconName icon, const QString& text);
    QAction* addElaIconAction(ElaIconType::IconName icon, const QString& text, const QKeySequence& shortcut);
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELAMENUBAR_H_
