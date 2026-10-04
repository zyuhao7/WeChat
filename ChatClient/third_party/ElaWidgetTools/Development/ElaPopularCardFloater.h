#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAPOPULARCARDFLOATER_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAPOPULARCARDFLOATER_H_

#include <QWidget>

#include "ElaWidgetToolsDef.h"
class ElaPushButton;
class ElaPopularCard;
class ElaPopularCardPrivate;
class QGraphicsOpacityEffect;
class ElaPopularCardFloater : public QWidget
{
    Q_OBJECT
    Q_PROPERTY_CREATE(qreal, HoverYOffset);
    Q_PROPERTY_CREATE(qreal, HoverOpacity);

public:
    explicit ElaPopularCardFloater(ElaPopularCard* card, ElaPopularCardPrivate* cardPrivate, QWidget* parent = nullptr);
    ~ElaPopularCardFloater();
    void showFloater();
    void hideFloater();

protected:
    virtual bool event(QEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;

private:
    friend class ElaPopularCard;
    ElaThemeType::ThemeMode _themeMode;
    ElaPushButton* _overButton{nullptr};
    QGraphicsOpacityEffect* _opacityEffect{nullptr};
    bool _isHideAnimationFinished{true};
    ElaPopularCard* _card{nullptr};
    ElaPopularCardPrivate* _cardPrivate{nullptr};
    QRect _calculateTargetGeometry(QRect cardGeometry);
    int _floatGeometryOffset{25};
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELAPOPULARCARDFLOATER_H_

