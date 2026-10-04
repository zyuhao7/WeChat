#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELASUGGESTDELEGATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELASUGGESTDELEGATE_H_

#include <QStyledItemDelegate>

#include "ElaWidgetToolsDef.h"
class ElaSuggestDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ElaSuggestDelegate(QObject* parent = nullptr);
    ~ElaSuggestDelegate();

protected:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    ElaThemeType::ThemeMode _themeMode;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELASUGGESTDELEGATE_H_

