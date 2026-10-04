#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELAAPPLICATION_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELAAPPLICATION_H_

#include <QIcon>
#include <QObject>

#include "ElaSingletonMacro.h"
#include "ElaWidgetToolsDef.h"
#define eApp ElaApplication::getInstance()
class ElaApplicationPrivate;
class ELA_EXPORT ElaApplication : public QObject
{
    Q_OBJECT
    Q_Q_CREATE(ElaApplication)
    Q_SINGLETON_CREATE_H(ElaApplication)
    Q_PROPERTY_CREATE_Q_H(ElaApplicationType::WindowDisplayMode, WindowDisplayMode)
    Q_PROPERTY_REF_CREATE_Q_H(QString, ElaMicaImagePath)
    Q_PROPERTY_CREATE_Q_H(int, FontPixelSize)
private:
    explicit ElaApplication(QObject* parent = nullptr);
    ~ElaApplication() override;

public:
    void init();
    void syncWindowDisplayMode(QWidget* widget, bool isSync = true);
    static bool containsCursorToItem(QWidget* item);
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELAAPPLICATION_H_
