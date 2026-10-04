#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELARIBBONBARPRIVATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELARIBBONBARPRIVATE_H_

#include <QObject>

#include "ElaWidgetToolsDef.h"
#include "ElaRibbonBarDelegate.h"
#include "ElaRibbonBarModel.h"
#include "ElaRibbonBarView.h"
#include <QStackedWidget>
class ElaBaseListView;
class ElaRibbonBar;
struct ElaRibbonMVDData {
    ElaRibbonBarModel* ribbonModel{nullptr};
    ElaRibbonBarView* ribbonView{nullptr};
    ElaRibbonBarDelegate* ribbonDelegate{nullptr};
};

class ElaRibbonBarPrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaRibbonBar);

public:
    explicit ElaRibbonBarPrivate(QObject* parent = nullptr);
    ~ElaRibbonBarPrivate() override;

    ElaRibbonBarModel* getOrCreateMVDData(const QString& pageName);

    Q_SLOT void onRibbonTabBarCurrentChanged(int index);

private:
    void _switchMainStackWidget(QWidget* targetWidget);
    QStackedWidget* _mainStackedWidget{nullptr};
    QTabBar* _ribbonTabBar{nullptr};
    QString _currentRibbonPage{""};
    QMap<QString, ElaRibbonMVDData> _ribbonDataMap;
};

#endif //ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELARIBBONBARPRIVATE_H_

