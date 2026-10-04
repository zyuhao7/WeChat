#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELATOOLBUTTON_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELATOOLBUTTON_H_

#include <QToolButton>

#include "ElaWidgetToolsDef.h"
class ElaMenu;
class ElaToolButtonPrivate;
class ELA_EXPORT ElaToolButton : public QToolButton
{
    Q_OBJECT
    Q_Q_CREATE(ElaToolButton)
    Q_PROPERTY_CREATE_Q_H(int, BorderRadius);
    Q_PROPERTY_CREATE_Q_H(bool, IsSelected);

public:
    explicit ElaToolButton(QWidget* parent = nullptr);
    ~ElaToolButton() override;

    void setIsTransparent(bool isTransparent);
    bool getIsTransparent() const;

    void setMenu(ElaMenu* menu);
    void setElaIcon(ElaIconType::IconName icon);
    void setElaIcon(ElaIconType::IconName icon, int rotate);

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELATOOLBUTTON_H_
