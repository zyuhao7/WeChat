#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELAIMAGECARDPRIVATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELAIMAGECARDPRIVATE_H_

#include <QImage>
#include <QObject>

#include "ElaWidgetToolsDef.h"

class ElaImageCard;
class ElaImageCardPrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaImageCard)
    Q_PROPERTY_CREATE_D(QImage, CardImage);
    Q_PROPERTY_CREATE_D(int, BorderRadius)
    Q_PROPERTY_CREATE_D(bool, IsPreserveAspectCrop)
public:
    explicit ElaImageCardPrivate(QObject* parent = nullptr);
    ~ElaImageCardPrivate() override;

private:
    ElaThemeType::ThemeMode _themeMode;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELAIMAGECARDPRIVATE_H_

