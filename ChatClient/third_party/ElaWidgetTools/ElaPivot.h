#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELAPIVOT_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELAPIVOT_H_
#include <QWidget>

#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"

class ElaPivotPrivate;
class ELA_EXPORT ElaPivot : public QWidget
{
    Q_OBJECT
    Q_Q_CREATE(ElaPivot)
    Q_PROPERTY_CREATE_Q_H(int, TextPixelSize)
    Q_PROPERTY_CREATE_Q_H(int, CurrentIndex)
    Q_PROPERTY_CREATE_Q_H(int, PivotSpacing)
    Q_PROPERTY_CREATE_Q_H(int, MarkWidth)
public:
    explicit ElaPivot(QWidget* parent = nullptr);
    ~ElaPivot();

    void appendPivot(const QString& pivotTitle);
    void removePivot(const QString& pivotTitle);

Q_SIGNALS:
    Q_SIGNAL void pivotClicked(int index);
    Q_SIGNAL void pivotDoubleClicked(int index);
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELAPIVOT_H_
