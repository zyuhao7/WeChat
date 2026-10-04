#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELACOLORDIALOG_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELACOLORDIALOG_H_

#include <QDialog>

#include "ElaAppBar.h"
#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"
class ElaColorDialogPrivate;
class ELA_EXPORT ElaColorDialog : public QDialog
{
    Q_OBJECT
    Q_Q_CREATE(ElaColorDialog)
    Q_PROPERTY_REF_CREATE_Q_H(QColor, CurrentColor)
    Q_TAKEOVER_NATIVEEVENT_H
public:
    explicit ElaColorDialog(QWidget* parent = nullptr);
    ~ElaColorDialog() override;

    const QList<QColor>& getCustomColorList() const;
    QColor getCustomColor(int index) const;
    QString getCurrentColorRGB() const;
Q_SIGNALS:
    Q_SIGNAL void colorSelected(const QColor& color);

protected:
    virtual void paintEvent(QPaintEvent* event) override;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELACOLORDIALOG_H_
