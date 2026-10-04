#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELADRAWERAREAPRIVATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELADRAWERAREAPRIVATE_H_

#include "ElaWidgetToolsDef.h"
#include "ElaDrawerContainer.h"
#include "ElaDrawerHeader.h"
#include <QObject>
class ElaDrawerArea;
class ElaDrawerAreaPrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaDrawerArea)
public:
    explicit ElaDrawerAreaPrivate(QObject* parent = nullptr);
    ~ElaDrawerAreaPrivate() override;

    Q_SLOT void onDrawerHeaderClicked(bool isExpand);

private:
    ElaThemeType::ThemeMode _themeMode;
    ElaDrawerHeader* _drawerHeader{nullptr};
    ElaDrawerContainer* _drawerContainer{nullptr};
};

#endif //ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELADRAWERAREAPRIVATE_H_

