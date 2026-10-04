#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELAWINDOW_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELAWINDOW_H_

#include <QMainWindow>

#include "ElaAppBar.h"
#include "ElaWidgetToolsDef.h"
#include "ElaSuggestBox.h"
class ElaWindowPrivate;
class ELA_EXPORT ElaWindow : public QMainWindow
{
    Q_OBJECT
    Q_Q_CREATE(ElaWindow)
    Q_PROPERTY_CREATE_Q_H(bool, IsStayTop)
    Q_PROPERTY_CREATE_Q_H(bool, IsFixedSize)
    Q_PROPERTY_CREATE_Q_H(bool, IsDefaultClosed)
    Q_PROPERTY_CREATE_Q_H(int, AppBarHeight)
    Q_PROPERTY_CREATE_Q_H(int, RibbonHeight)
    Q_PROPERTY_CREATE_Q_H(int, ThemeChangeTime)
    Q_PROPERTY_CREATE_Q_H(bool, IsCentralStackedWidgetTransparent)
    Q_PROPERTY_CREATE_Q_H(bool, IsAllowPageOpenInNewWindow)
    Q_PROPERTY_CREATE_Q_H(bool, IsNavigationBarEnable)
    Q_PROPERTY_CREATE_Q_H(int, NavigationBarWidth)
    Q_PROPERTY_CREATE_Q_H(int, CurrentStackIndex)
    Q_PROPERTY_CREATE_Q_H(ElaNavigationType::NavigationDisplayMode, NavigationBarDisplayMode)
    Q_PROPERTY_CREATE_Q_H(ElaWindowType::StackSwitchMode, StackSwitchMode)
    Q_PROPERTY_CREATE_Q_H(ElaWindowType::PaintMode, WindowPaintMode)
    Q_TAKEOVER_NATIVEEVENT_H
public:
    Q_INVOKABLE explicit ElaWindow(QWidget* parent = nullptr);
    ~ElaWindow() override;

    void moveToCenter();

    void setCustomWidget(ElaAppBarType::CustomArea customArea, QWidget* customWidget, QObject* hitTestObject = nullptr, const QString& hitTestFunctionName = "");
    QWidget* getCustomWidget(ElaAppBarType::CustomArea customArea) const;

    void setCentralCustomWidget(QWidget* customWidget);
    QWidget* getCentralCustomWidget() const;

    void setCustomMenu(QMenu* customMenu);
    QMenu* getCustomMenu() const;

    void setUserInfoCardVisible(bool isVisible);
    void setUserInfoCardPixmap(const QPixmap& pix);
    void setUserInfoCardTitle(const QString& title);
    void setUserInfoCardSubTitle(const QString& subTitle);
    ElaNavigationType::NodeResult addExpanderNode(const QString& expanderTitle, QString& expanderKey, ElaIconType::IconName awesome = ElaIconType::None) const;
    ElaNavigationType::NodeResult addExpanderNode(const QString& expanderTitle, QString& expanderKey, const QString& targetExpanderKey, ElaIconType::IconName awesome = ElaIconType::None) const;
    ElaNavigationType::NodeResult addPageNode(const QString& pageTitle, QWidget* page, ElaIconType::IconName awesome = ElaIconType::None);
    ElaNavigationType::NodeResult addPageNode(const QString& pageTitle, QWidget* page, int keyPoints, ElaIconType::IconName awesome = ElaIconType::None);
    ElaNavigationType::NodeResult addPageNode(const QString& pageTitle, QWidget* page, const QString& targetExpanderKey, ElaIconType::IconName awesome = ElaIconType::None);
    ElaNavigationType::NodeResult addPageNode(const QString& pageTitle, QWidget* page, const QString& targetExpanderKey, int keyPoints, ElaIconType::IconName awesome = ElaIconType::None);
    ElaNavigationType::NodeResult addFooterNode(const QString& footerTitle, QString& footerKey, int keyPoints = 0, ElaIconType::IconName awesome = ElaIconType::None) const;
    ElaNavigationType::NodeResult addFooterNode(const QString& footerTitle, QWidget* page, QString& footerKey, int keyPoints = 0, ElaIconType::IconName awesome = ElaIconType::None);
    ElaNavigationType::NodeResult addCategoryNode(const QString& categoryTitle, QString& categoryKey);
    ElaNavigationType::NodeResult addCategoryNode(const QString& categoryTitle, QString& categoryKey, const QString& targetExpanderKey);

    void addCentralWidget(QWidget* centralWidget);
    QWidget* getCentralWidget(int index) const;

    bool getNavigationNodeIsExpanded(const QString& expanderKey) const;
    void expandNavigationNode(const QString& expanderKey);
    void collapseNavigationNode(const QString& expanderKey);
    void removeNavigationNode(const QString& nodeKey) const;
    int getPageOpenInNewWindowCount(const QString& nodeKey) const;
    void backtrackNavigationNode(const QString& nodeKey);

    void setNodeKeyPoints(const QString& nodeKey, int keyPoints);
    int getNodeKeyPoints(const QString& nodeKey) const;

    void setNavigationNodeTitle(const QString& nodeKey, const QString& nodeTitle);
    QString getNavigationNodeTitle(const QString& nodeKey) const;

    void navigation(const QString& pageKey);
    int getCurrentNavigationIndex() const;
    QString getCurrentNavigationPageKey() const;

    const QList<ElaSuggestBox::SuggestData>& getNavigationSuggestDataList() const;

    void setWindowButtonFlag(ElaAppBarType::ButtonType buttonFlag, bool isEnable = true);
    void setWindowButtonFlags(ElaAppBarType::ButtonFlags buttonFlags);
    ElaAppBarType::ButtonFlags getWindowButtonFlags() const;

    void setWindowMoviePath(ElaThemeType::ThemeMode themeMode, const QString& moviePath);
    QString getWindowMoviePath(ElaThemeType::ThemeMode themeMode) const;

    void setWindowPixmap(ElaThemeType::ThemeMode themeMode, const QPixmap& pixmap);
    const QPixmap& getWindowPixmap(ElaThemeType::ThemeMode themeMode) const;

    void setWindowMovieRate(qreal rate);
    qreal getWindowMovieRate() const;

    void tabifyDockWidget(QDockWidget* targetDockWidget, QDockWidget* dockWidget);
    void tabifyDockWidget(Qt::DockWidgetArea area, const QString& targetDockTitle, QDockWidget* dockWidget);

Q_SIGNALS:
    Q_SIGNAL void userInfoCardClicked();
    Q_SIGNAL void closeButtonClicked();
    Q_SIGNAL void navigationNodeClicked(ElaNavigationType::NavigationNodeType nodeType, QString nodeKey);
    Q_SIGNAL void customWidgetChanged();
    Q_SIGNAL void centralCustomWidgetChanged();
    Q_SIGNAL void customMenuChanged();
    Q_SIGNAL void pageOpenInNewWindow(QString nodeKey);

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event) override;
    virtual QMenu* createPopupMenu() override;
    virtual void paintEvent(QPaintEvent* event) override;

private:
    QWidget* centralWidget() const;
    void setCentralWidget(QWidget* widget);
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELAWINDOW_H_

