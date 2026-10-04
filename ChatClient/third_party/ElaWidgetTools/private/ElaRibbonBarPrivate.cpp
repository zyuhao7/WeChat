#include "ElaRibbonBarPrivate.h"
#include "ElaRibbonBar.h"

#include <QPropertyAnimation>
ElaRibbonBarPrivate::ElaRibbonBarPrivate(QObject* parent)
    : QObject(parent)
{
}

ElaRibbonBarPrivate::~ElaRibbonBarPrivate()
{
}

ElaRibbonBarModel* ElaRibbonBarPrivate::getOrCreateMVDData(const QString& pageName)
{
    Q_Q(ElaRibbonBar);
    ElaRibbonBarModel* ribbonModel = nullptr;
    if (!_ribbonDataMap.contains(pageName))
    {
        if (_currentRibbonPage.isEmpty())
        {
            _currentRibbonPage = pageName;
        }
        ElaRibbonMVDData mvdData;
        auto ribbonView = new ElaRibbonBarView(q);
        ribbonView->setMinimumHeight(0);
        auto ribbonDelegate = new ElaRibbonBarDelegate(q);
        ribbonView->setItemDelegate(ribbonDelegate);
        ribbonView->setRibbonBarDelegate(ribbonDelegate);
        ribbonModel = new ElaRibbonBarModel(q);
        ribbonView->setRibbonBarModel(ribbonModel);
        ribbonView->setModel(ribbonModel);

        mvdData.ribbonModel = ribbonModel;
        mvdData.ribbonView = ribbonView;
        mvdData.ribbonDelegate = ribbonDelegate;
        _ribbonDataMap.insert(pageName, mvdData);
        _mainStackedWidget->addWidget(ribbonView);
        if (_ribbonTabBar)
        {
            _ribbonTabBar->blockSignals(true);
            _ribbonTabBar->addTab(pageName);
            _ribbonTabBar->blockSignals(false);
        }
    }
    else
    {
        ribbonModel = _ribbonDataMap[pageName].ribbonModel;
    }
    return ribbonModel;
}

void ElaRibbonBarPrivate::onRibbonTabBarCurrentChanged(int index)
{
    Q_Q(ElaRibbonBar);
    QString tabName = _ribbonTabBar->tabText(index);
    q->switchRibbonPage(tabName);
}

void ElaRibbonBarPrivate::_switchMainStackWidget(QWidget* targetWidget)
{
    Q_Q(ElaRibbonBar);
    _mainStackedWidget->setCurrentWidget(targetWidget);
    QPropertyAnimation* targetWidgetAnimation = new QPropertyAnimation(targetWidget, "pos");
    targetWidgetAnimation->setEasingCurve(QEasingCurve::OutCubic);
    targetWidgetAnimation->setDuration(300);
    targetWidgetAnimation->setStartValue(QPoint(-60, 0));
    targetWidgetAnimation->setEndValue(QPoint(0, 0));
    targetWidgetAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}