#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELALOG_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELALOG_H_

#include <QObject>

#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"
#include "ElaSingletonMacro.h"

class ElaLogPrivate;
class ELA_EXPORT ElaLog : public QObject
{
    Q_OBJECT
    Q_Q_CREATE(ElaLog)
    Q_PROPERTY_REF_CREATE_Q_H(QString, LogSavePath)
    Q_PROPERTY_REF_CREATE_Q_H(QString, LogFileName)
    Q_PROPERTY_CREATE_Q_H(bool, IsLogFileNameWithTime)
    Q_SINGLETON_CREATE_H(ElaLog);

private:
    explicit ElaLog(QObject* parent = nullptr);
    ~ElaLog();

public:
    void initMessageLog(bool isEnable);
Q_SIGNALS:
    void logMessage(const QString& log);
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELALOG_H_
