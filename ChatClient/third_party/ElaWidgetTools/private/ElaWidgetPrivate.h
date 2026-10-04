#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELAWIDGETPRIVATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELAWIDGETPRIVATE_H_

#include <QObject>

#include "ElaWidgetToolsDef.h"
class ElaWidget;
class ElaAppBar;
class ElaWidgetPrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaWidget)
public:
    explicit ElaWidgetPrivate(QObject* parent = nullptr);
    ~ElaWidgetPrivate() override;

private:
    ElaThemeType::ThemeMode _themeMode;
    ElaApplicationType::WindowDisplayMode _windowDisplayMode;
    ElaAppBar* _appBar{nullptr};
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELAWIDGETPRIVATE_H_

