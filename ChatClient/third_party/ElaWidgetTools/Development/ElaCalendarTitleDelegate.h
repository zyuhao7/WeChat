#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACALENDARTITLEDELEGATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACALENDARTITLEDELEGATE_H_

#include <QStyledItemDelegate>

#include "ElaWidgetToolsDef.h"
class ElaCalendarTitleDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ElaCalendarTitleDelegate(QObject* parent = nullptr);
    ~ElaCalendarTitleDelegate();

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    ElaThemeType::ThemeMode _themeMode;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACALENDARTITLEDELEGATE_H_

