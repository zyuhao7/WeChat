#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELADIALOGPRIVATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELADIALOGPRIVATE_H_

#include <QObject>

#include "ElaWidgetToolsDef.h"
class ElaAppBar;
class ElaDialog;
class ElaDialogPrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaDialog)
public:
    explicit ElaDialogPrivate(QObject* parent = nullptr);
    ~ElaDialogPrivate() override;

private:
    ElaThemeType::ThemeMode _themeMode;
    ElaApplicationType::WindowDisplayMode _windowDisplayMode;
    ElaAppBar* _appBar{nullptr};
};

#endif //ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELADIALOGPRIVATE_H_

