#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELATOGGLEBUTTONPRIVATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELATOGGLEBUTTONPRIVATE_H_

#include <QObject>

#include "ElaWidgetToolsDef.h"
class ElaToggleButton;
class ElaToggleButtonPrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaToggleButton)
    Q_PROPERTY_CREATE_D(int, BorderRadius)
    Q_PROPERTY_CREATE_D(QString, Text)
    Q_PROPERTY_CREATE(int, ToggleAlpha)
public:
    explicit ElaToggleButtonPrivate(QObject* parent = nullptr);
    ~ElaToggleButtonPrivate() override;

private:
    bool _isAlphaAnimationFinished{true};
    bool _isToggled{false};
    bool _isPressed{false};
    ElaThemeType::ThemeMode _themeMode;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELATOGGLEBUTTONPRIVATE_H_

