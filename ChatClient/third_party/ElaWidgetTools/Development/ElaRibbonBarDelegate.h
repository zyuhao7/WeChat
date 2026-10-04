#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELARIBBONBARDELEGATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELARIBBONBARDELEGATE_H_

#include <QModelIndex>
#include <QStyledItemDelegate>

#include "ElaWidgetToolsDef.h"
class ElaRibbonBarDelegate : public QStyledItemDelegate
{
    Q_OBJECT
    Q_PRIVATE_REF_CREATE(QModelIndex, PressIndex);
    Q_PRIVATE_CREATE(int, BottomMargin);

public:
    explicit ElaRibbonBarDelegate(QObject* parent = nullptr);
    ~ElaRibbonBarDelegate() override;

protected:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    ElaThemeType::ThemeMode _themeMode;
};

#endif //ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELARIBBONBARDELEGATE_H_

