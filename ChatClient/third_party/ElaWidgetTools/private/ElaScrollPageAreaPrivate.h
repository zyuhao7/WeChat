#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELASCROLLPAGEAREAPRIVATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELASCROLLPAGEAREAPRIVATE_H_

#include <QObject>

#include "ElaWidgetToolsDef.h"
class ElaScrollPageArea;
class ElaScrollPageAreaPrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaScrollPageArea)
    Q_PROPERTY_CREATE_D(int, BorderRadius)
public:
    explicit ElaScrollPageAreaPrivate(QObject* parent = nullptr);
    ~ElaScrollPageAreaPrivate() override;

private:
    ElaThemeType::ThemeMode _themeMode;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELASCROLLPAGEAREAPRIVATE_H_

