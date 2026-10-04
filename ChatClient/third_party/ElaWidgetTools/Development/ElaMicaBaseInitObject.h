#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAMICABASEINITOBJECT_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAMICABASEINITOBJECT_H_

#include <QImage>
#include <QObject>
class ElaApplicationPrivate;
class ElaMicaBaseInitObject : public QObject
{
    Q_OBJECT
public:
    explicit ElaMicaBaseInitObject(ElaApplicationPrivate* appPrivate, QObject* parent = nullptr);
    ~ElaMicaBaseInitObject();
    Q_SLOT void onInitMicaBase(QImage img);
Q_SIGNALS:
    Q_SIGNAL void initFinished();

private:
    ElaApplicationPrivate* _appPrivate{nullptr};
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAMICABASEINITOBJECT_H_

