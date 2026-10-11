#ifndef LIQUIDGLASS_H
#define LIQUIDGLASS_H

#include <QColor>
#include <QElapsedTimer>
#include <QImage>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

class QVariantAnimation;

// Stylesheet for a form page laid out on glass: title, tip line and translucent fields.
QString glassFormStyleSheet();

// `source` cropped to fill `size`, with rounded corners.
QPixmap roundedPixmap(const QPixmap& source, const QSize& size, qreal radius);

// Slowly drifting pastel background. Glass surfaces above it sample its blurred copy, so
// it must sit underneath them in the same window.
class GlassBackdrop : public QWidget
{
    Q_OBJECT

public:
    explicit GlassBackdrop(QWidget* parent = nullptr);

    // A still backdrop spares busy pages from repainting every frame.
    void setAnimated(bool animated);

    // Paints the frosted backdrop region under `target` (a rect in `surface` coordinates).
    static void paintFrosted(QPainter& painter, const QWidget* surface, const QRectF& target);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void renderFrame();

    QImage _frame;
    QImage _blurred;
    QTimer _tick;
    QElapsedTimer _clock;
    bool _animated = true;
};

// Rounded frosted-glass card. The outer shadow is drawn inside the widget's own bounds, so
// the visible card is inset by shadowMargin() on every side.
class GlassPanel : public QWidget
{
    Q_OBJECT

public:
    explicit GlassPanel(QWidget* parent = nullptr);

    static constexpr int shadowMargin() { return 14; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    qreal _radius = 24;
};

// Pill-shaped glass button that squeezes on press and springs back on release, with a
// specular highlight that follows the cursor.
class GlassButton : public QPushButton
{
    Q_OBJECT

public:
    explicit GlassButton(QWidget* parent = nullptr);

    void setAccent(const QColor& accent);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void animateScale(qreal to, int ms, bool spring);

    QColor _accent;
    QPointF _cursor;
    bool _hovered = false;
    qreal _scale = 1.0;
    QVariantAnimation* _scaleAnim;
};

#endif // LIQUIDGLASS_H
