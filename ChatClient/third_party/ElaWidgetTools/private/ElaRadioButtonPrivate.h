#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELARADIOBUTTONPRIVATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELARADIOBUTTONPRIVATE_H_
#include <QObject>

#include "ElaWidgetToolsDef.h"
class ElaRadioButton;
class ElaRadioButtonPrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaRadioButton)
public:
    explicit ElaRadioButtonPrivate(QObject* parent = nullptr);
    ~ElaRadioButtonPrivate() override;
    Q_SLOT void onThemeChanged(ElaThemeType::ThemeMode themeMode);

private:
    ElaThemeType::ThemeMode _themeMode;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELARADIOBUTTONPRIVATE_H_

