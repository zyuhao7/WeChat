#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELATEXTPRIVATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELATEXTPRIVATE_H_

#include <QObject>

#include "ElaWidgetToolsDef.h"
class ElaText;
class ElaTextPrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaText)
    Q_PROPERTY_CREATE_D(ElaTextType::TextStyle, TextStyle)
    Q_PROPERTY_CREATE_D(ElaIconType::IconName, ElaIcon)
public:
    explicit ElaTextPrivate(QObject* parent = nullptr);
    ~ElaTextPrivate() override;
    Q_SLOT void onThemeChanged(ElaThemeType::ThemeMode themeMode);

private:
    ElaThemeType::ThemeMode _themeMode;
    bool _isWrapAnywhere{false};
    qreal _textSpacing{0.5};
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELATEXTPRIVATE_H_

