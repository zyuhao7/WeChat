#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELALCDNUMBERPRIVATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELALCDNUMBERPRIVATE_H_

#include "ElaWidgetToolsDef.h"

#include <QObject>
class QTimer;
class ElaLCDNumber;
class ElaLCDNumberStyle;
class ElaLCDNumberPrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaLCDNumber)
    Q_PROPERTY_CREATE_D(bool, IsUseAutoClock)
    Q_PROPERTY_CREATE_D(QString, AutoClockFormat)
public:
    explicit ElaLCDNumberPrivate(QObject* parent = nullptr);
    ~ElaLCDNumberPrivate() override;

    Q_SLOT void onThemeModeChanged(ElaThemeType::ThemeMode themeMode);

private:
    ElaThemeType::ThemeMode _themeMode;
    QTimer* _clockTimer{nullptr};
    ElaLCDNumberStyle* _lcdNumberStyle{nullptr};
};

#endif //ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELALCDNUMBERPRIVATE_H_

