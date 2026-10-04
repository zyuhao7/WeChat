#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELADXGI_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELADXGI_H_

#include <QObject>
#ifdef Q_OS_WIN
#include <QElapsedTimer>
#include <QPixmap>
#include <d3d11.h>
#include <dxgi1_6.h>

#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"

#include <QMutex>

class ElaDxgi : public QObject
{
    Q_OBJECT
    Q_PRIVATE_REF_CREATE(QStringList, DxDeviceList)
    Q_PRIVATE_REF_CREATE(QStringList, OutputDeviceList)
    Q_PRIVATE_CREATE(int, DxDeviceID);
    Q_PRIVATE_CREATE(int, OutputDeviceID);
    Q_PRIVATE_REF_CREATE(QString, LastError)
    Q_PRIVATE_CREATE(QRect, GrabArea);
    Q_PRIVATE_CREATE(int, GrabFrameRate);  // 截图帧数
    Q_PRIVATE_CREATE(int, TimeoutMsValue); // 超时等待
    Q_PRIVATE_CREATE(bool, IsInitSuccess);
    Q_PRIVATE_CREATE(bool, IsGrabStoped);
    Q_PRIVATE_CREATE(bool, IsGrabCenter);

public:
    explicit ElaDxgi(QObject* parent = nullptr);
    ~ElaDxgi() override;
    bool initialize(int dxID, int outputID);

    void setIsGrabActive(bool isGrabActive);
    bool getIsGrabActive() const;

    QImage getGrabImage();
    Q_SLOT void onGrabScreen();
    Q_SIGNAL void grabScreenOver(const QImage& img);

private:
    QMutex _grabMutex;
    std::atomic_bool _isGrabActive{false};
    IDXGIOutputDuplication* _duplication{nullptr};
    ID3D11Device* _device{nullptr};
    ID3D11DeviceContext* _context{nullptr};
    ID3D11Texture2D* _texture{nullptr};
    QElapsedTimer _grabTimer;
    qreal _lastGrabTime{0};
    qreal _cpuSleepTime{0};
    uchar* _imageBits{nullptr};
    int _descWidth{0};
    int _descHeight{0};
    void releaseInterface();
    static void waitElapsedTime(qreal usec);
};
#endif
#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELADXGI_H_

