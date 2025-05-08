#ifndef LISTITEMBASE_H
#define LISTITEMBASE_H
#include <QWidget>
#include "global.h"

class ListItemBase : public QWidget
{
   Q_OBJECT

public:
    explicit ListItemBase(QWidget* parent = nullptr);
    void SetItemType(ListItemType itemType);

    ListItemType GetItemType();
protected:
    void paintEvent(QPaintEvent* event);
private:
        ListItemType _itemType;
};

#endif // LISTITEMBASE_H
