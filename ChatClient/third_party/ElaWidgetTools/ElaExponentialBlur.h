#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELAEXPONENTIALBLUR_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELAEXPONENTIALBLUR_H_

#include <QObject>

#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"
#include "ElaSingletonMacro.h"

class ElaExponentialBlurPrivate;
class ELA_EXPORT ElaExponentialBlur : public QObject
{
    Q_OBJECT
    Q_SINGLETON_CREATE_H(ElaExponentialBlur)
    Q_Q_CREATE(ElaExponentialBlur)
private:
    explicit ElaExponentialBlur(QObject* parent = nullptr);
    ~ElaExponentialBlur();

public:
    static QPixmap doExponentialBlur(QImage img, const quint16& blurRadius);
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELAEXPONENTIALBLUR_H_
