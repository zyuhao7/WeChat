#include "liquidglass.h"

#include <QEnterEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>
#include <cmath>

namespace
{

// The backdrop is nothing but soft gradients, so rendering it at a fraction of the window
// size and letting the upscale smooth it costs nothing visible.
constexpr int kFrameDiv = 4;
// Shrinking the frame further and stretching it back is the frost behind the glass.
constexpr int kBlurDiv = 6;
constexpr int kFrameIntervalMs = 33;
constexpr qreal kTau = 6.283185307179586;

struct Blob
{
    QColor color;
    qreal radius; // fraction of the longer side
    qreal ax, ay; // drift amplitude, fraction of each side
    qreal periodX, periodY;
    qreal phase;
};

const Blob kBlobs[] = {
    {QColor(0xA6, 0x95, 0xF2, 230), 0.70, 0.30, 0.35, 23.0, 31.0, 0.0},
    {QColor(0x93, 0xD6, 0xEE, 210), 0.65, 0.35, 0.30, 29.0, 19.0, 2.1},
    {QColor(0xF4, 0xD3, 0xEE, 200), 0.55, 0.25, 0.40, 17.0, 27.0, 4.2},
    {QColor(0xFF, 0xFF, 0xFF, 170), 0.45, 0.30, 0.25, 21.0, 25.0, 1.3},
};

// Soft drop shadow around `shape`, kept out from under the translucent glass itself so
// it does not muddy the fill.
void paintSoftShadow(QPainter& p, const QRectF& bounds, const QRectF& shape, qreal radius,
                     int spread, qreal dy)
{
    QPainterPath body;
    body.addRoundedRect(shape, radius, radius);
    QPainterPath area;
    area.addRect(bounds);

    p.save();
    p.setClipPath(area.subtracted(body));
    p.setPen(Qt::NoPen);
    for (int i = spread; i >= 1; --i)
    {
        const qreal falloff = 1.0 - qreal(i) / (spread + 1);
        p.setBrush(QColor(60, 45, 120, qRound(10 * falloff * falloff)));
        const QRectF layer = shape.adjusted(-i, -i + dy, i, i + dy);
        p.drawRoundedRect(layer, radius + i, radius + i);
    }
    p.restore();
}

void paintRim(QPainter& p, const QRectF& shape, qreal radius)
{
    QLinearGradient rim(shape.topLeft(), shape.bottomRight());
    rim.setColorAt(0.0, QColor(255, 255, 255, 235));
    rim.setColorAt(0.5, QColor(255, 255, 255, 70));
    rim.setColorAt(1.0, QColor(255, 255, 255, 170));
    p.setPen(QPen(QBrush(rim), 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(shape.adjusted(0.6, 0.6, -0.6, -0.6), radius, radius);
}

void paintSheen(QPainter& p, const QPainterPath& shape, const QRectF& rect, int topAlpha)
{
    QLinearGradient sheen(rect.topLeft(), rect.bottomLeft());
    sheen.setColorAt(0.0, QColor(255, 255, 255, topAlpha));
    sheen.setColorAt(0.45, QColor(255, 255, 255, 0));
    sheen.setColorAt(1.0, QColor(255, 255, 255, 30));
    p.fillPath(shape, sheen);
}

} // namespace

QString glassFormStyleSheet()
{
    return QStringLiteral(R"(
        QLabel#title_label { color: #1C1C1E; font-size: 22px; font-weight: 600; }
        QLabel#err_tip { color: #2F9E5B; font-size: 12px; }
        QLabel#err_tip[state="err"] { color: #E5484D; }
        QLineEdit {
            background: rgba(255, 255, 255, 150);
            border: 1px solid rgba(255, 255, 255, 230);
            border-radius: 14px;
            padding: 0 14px;
            color: #1C1C1E;
            font-size: 14px;
            selection-background-color: #0A84FF;
        }
        QLineEdit:focus {
            background: rgba(255, 255, 255, 200);
            border: 1px solid rgba(10, 132, 255, 140);
        }
    )");
}

GlassBackdrop::GlassBackdrop(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    _tick.setInterval(kFrameIntervalMs);
    connect(&_tick, &QTimer::timeout, this, [this] {
        renderFrame();
        update();
    });
    _clock.start();
}

QPixmap roundedPixmap(const QPixmap& source, const QSize& size, qreal radius)
{
    const QPixmap scaled = source.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QPixmap out(size);
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    QPainterPath clip;
    clip.addRoundedRect(QRectF(QPointF(0, 0), size), radius, radius);
    p.setClipPath(clip);
    p.drawPixmap((size.width() - scaled.width()) / 2, (size.height() - scaled.height()) / 2, scaled);
    return out;
}

void GlassBackdrop::setAnimated(bool animated)
{
    _animated = animated;
    if (animated && isVisible())
    {
        _tick.start();
    }
    else
    {
        _tick.stop();
    }
}

void GlassBackdrop::renderFrame()
{
    if (size().isEmpty())
    {
        return;
    }
    const QSize small((width() + kFrameDiv - 1) / kFrameDiv, (height() + kFrameDiv - 1) / kFrameDiv);
    if (_frame.size() != small)
    {
        _frame = QImage(small, QImage::Format_ARGB32_Premultiplied);
    }

    const qreal t = _clock.elapsed() / 1000.0;
    QPainter p(&_frame);
    p.setRenderHint(QPainter::Antialiasing);

    QLinearGradient base(0, 0, small.width() * 0.4, small.height());
    base.setColorAt(0.0, QColor(0xB4, 0xA7, 0xEE));
    base.setColorAt(0.55, QColor(0xE6, 0xE3, 0xF5));
    base.setColorAt(1.0, QColor(0xBF, 0xDD, 0xEC));
    p.fillRect(_frame.rect(), base);

    const qreal span = qMax(small.width(), small.height());
    for (const Blob& b : kBlobs)
    {
        const QPointF center(small.width() * (0.5 + b.ax * std::sin(kTau * t / b.periodX + b.phase)),
                             small.height() * (0.5 + b.ay * std::cos(kTau * t / b.periodY + b.phase)));
        QColor edge = b.color;
        edge.setAlpha(0);
        QRadialGradient g(center, span * b.radius);
        g.setColorAt(0.0, b.color);
        g.setColorAt(1.0, edge);
        p.fillRect(_frame.rect(), g);
    }
    p.end();

    const QSize tiny(qMax(1, small.width() / kBlurDiv), qMax(1, small.height() / kBlurDiv));
    _blurred = _frame.scaled(tiny, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void GlassBackdrop::paintFrosted(QPainter& painter, const QWidget* surface, const QRectF& target)
{
    QWidget* win = surface->window();
    const GlassBackdrop* backdrop = win->findChild<GlassBackdrop*>();
    if (!backdrop || !backdrop->isVisible() || backdrop->_blurred.isNull())
    {
        return;
    }
    const QPointF origin = backdrop->mapFrom(win, surface->mapTo(win, QPoint(0, 0)));
    const qreal sx = qreal(backdrop->_blurred.width()) / backdrop->width();
    const qreal sy = qreal(backdrop->_blurred.height()) / backdrop->height();
    const QRectF source((target.x() + origin.x()) * sx, (target.y() + origin.y()) * sy,
                        target.width() * sx, target.height() * sy);

    painter.save();
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(target, backdrop->_blurred, source);
    painter.restore();
}

void GlassBackdrop::paintEvent(QPaintEvent*)
{
    if (_frame.isNull())
    {
        renderFrame();
    }
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(QRectF(rect()), _frame);
}

void GlassBackdrop::resizeEvent(QResizeEvent* event)
{
    renderFrame();
    QWidget::resizeEvent(event);
}

void GlassBackdrop::showEvent(QShowEvent* event)
{
    if (_animated)
    {
        _tick.start();
    }
    QWidget::showEvent(event);
}

void GlassBackdrop::hideEvent(QHideEvent* event)
{
    _tick.stop();
    QWidget::hideEvent(event);
}

GlassPanel::GlassPanel(QWidget* parent)
    : QWidget(parent)
{
    const int m = shadowMargin();
    setContentsMargins(m, m, m, m);
}

void GlassPanel::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int m = shadowMargin();
    const QRectF card = QRectF(rect()).adjusted(m, m, -m, -m);
    QPainterPath shape;
    shape.addRoundedRect(card, _radius, _radius);

    paintSoftShadow(p, rect(), card, _radius, m - 2, 4);

    p.save();
    p.setClipPath(shape);
    GlassBackdrop::paintFrosted(p, this, card);
    p.fillPath(shape, QColor(255, 255, 255, 140));
    paintSheen(p, shape, card, 110);
    p.restore();

    paintRim(p, card, _radius);
}

GlassButton::GlassButton(QWidget* parent)
    : QPushButton(parent), _scaleAnim(new QVariantAnimation(this))
{
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
    connect(_scaleAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        _scale = v.toReal();
        update();
    });
}

void GlassButton::setAccent(const QColor& accent)
{
    _accent = accent;
    update();
}

void GlassButton::animateScale(qreal to, int ms, bool spring)
{
    _scaleAnim->stop();
    _scaleAnim->setStartValue(_scale);
    _scaleAnim->setEndValue(to);
    _scaleAnim->setDuration(ms);
    QEasingCurve curve(spring ? QEasingCurve::OutBack : QEasingCurve::OutCubic);
    if (spring)
    {
        curve.setOvershoot(2.4);
    }
    _scaleAnim->setEasingCurve(curve);
    _scaleAnim->start();
}

void GlassButton::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    constexpr int m = 6;
    const QRectF pill = QRectF(rect()).adjusted(m, m, -m, -m);
    const qreal radius = pill.height() / 2;
    const QPointF c = pill.center();
    p.translate(c);
    p.scale(_scale, _scale);
    p.translate(-c);

    paintSoftShadow(p, rect(), pill, radius, m, 2);

    QPainterPath shape;
    shape.addRoundedRect(pill, radius, radius);
    const bool on = isEnabled();

    p.save();
    p.setClipPath(shape);
    GlassBackdrop::paintFrosted(p, this, pill);
    QColor fill = _accent.isValid() ? _accent : QColor(255, 255, 255);
    fill.setAlpha(!on ? 110 : _accent.isValid() ? (_hovered ? 240 : 215)
                                                : (_hovered ? 190 : 150));
    p.fillPath(shape, fill);
    paintSheen(p, shape, pill, _accent.isValid() ? 90 : 120);
    if (_hovered && on)
    {
        QRadialGradient glint(_cursor, pill.height() * 1.3);
        glint.setColorAt(0.0, QColor(255, 255, 255, 95));
        glint.setColorAt(1.0, QColor(255, 255, 255, 0));
        p.fillPath(shape, glint);
    }
    p.restore();

    paintRim(p, pill, radius);

    QFont f = font();
    f.setPixelSize(15);
    f.setWeight(QFont::DemiBold);
    p.setFont(f);
    QColor ink = _accent.isValid() ? QColor(255, 255, 255) : QColor(28, 28, 30);
    if (!on)
    {
        ink.setAlpha(120);
    }
    p.setPen(ink);
    p.drawText(pill, Qt::AlignCenter, text());
}

void GlassButton::enterEvent(QEnterEvent* event)
{
    _hovered = true;
    _cursor = event->position();
    update();
    QPushButton::enterEvent(event);
}

void GlassButton::leaveEvent(QEvent* event)
{
    _hovered = false;
    update();
    QPushButton::leaveEvent(event);
}

void GlassButton::mouseMoveEvent(QMouseEvent* event)
{
    _cursor = event->position();
    update();
    QPushButton::mouseMoveEvent(event);
}

void GlassButton::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        animateScale(0.93, 110, false);
    }
    QPushButton::mousePressEvent(event);
}

void GlassButton::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        animateScale(1.0, 480, true);
    }
    QPushButton::mouseReleaseEvent(event);
}
