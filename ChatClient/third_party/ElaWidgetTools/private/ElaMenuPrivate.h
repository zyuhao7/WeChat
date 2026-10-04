#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELAMENUPRIVATE_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELAMENUPRIVATE_H_

#include <QObject>
#include <QPixmap>
#include <QPoint>

#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"
class ElaMenu;
class ElaMenuStyle;
class ElaMenuPrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaMenu)
    Q_PROPERTY_CREATE(int, AnimationImagePosY)
public:
    explicit ElaMenuPrivate(QObject* parent = nullptr);
    ~ElaMenuPrivate() override;

private:
    QPixmap _animationPix;
    bool _isCloseAnimation{false};
    QPoint _mousePressPoint;
    ElaMenuStyle* _menuStyle{nullptr};
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_PRIVATE_ELAMENUPRIVATE_H_

