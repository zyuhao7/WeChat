#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELANAVIGATIONVIEW_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELANAVIGATIONVIEW_H_

#include "ElaWidgetToolsDef.h"
#include "ElaNavigationBarPrivate.h"
#include <QObject>
#include <QTreeView>
class ElaScrollBar;
class ElaNavigationStyle;
class ElaToolTip;
class ElaNavigationView : public QTreeView
{
    Q_OBJECT
    Q_PRIVATE_CREATE(const ElaNavigationBarPrivate*, NavigationBarPrivate)
public:
    explicit ElaNavigationView(QWidget* parent = nullptr);
    ~ElaNavigationView() override;
    void navigationNodeStateChange(const QVariantMap& data);

    Q_SLOT void onCustomContextMenuRequested(const QPoint& pos);
Q_SIGNALS:
    Q_SIGNAL void navigationClicked(const QModelIndex& index);
    Q_SIGNAL void navigationOpenNewWindow(QString nodeKey);

protected:
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual bool eventFilter(QObject* watched, QEvent* event) override;

private:
    ElaNavigationStyle* _navigationStyle{nullptr};
    ElaToolTip* _compactToolTip{nullptr};
    void _doCompactToolTip();
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELANAVIGATIONVIEW_H_

