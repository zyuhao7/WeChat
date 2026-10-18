#include "cropdialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
const int VIEW_SIDE = 420;
const int MIN_SELECTION = 48;
const int HANDLE_SIZE = 12;
} // namespace

// A square selection that can be moved by dragging inside it and resized from the
// bottom-right handle; dragging elsewhere starts a new selection.
class CropView final : public QWidget
{
public:
    CropView(const QPixmap& source, QWidget* parent)
        : QWidget(parent),
          _source(source)
    {
        setFixedSize(VIEW_SIDE, VIEW_SIDE);
        setMouseTracking(true);
        _image_rect = CropDialog::FitImage(source.size(), size());
        ResetSelection();
    }

    QRect Selection() const
    {
        return _selection;
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.fillRect(rect(), QColor(30, 30, 30));
        p.drawPixmap(_image_rect.topLeft(), _source.scaled(_image_rect.size(), Qt::IgnoreAspectRatio,
                                                           Qt::SmoothTransformation));

        // darken everything outside the selection so the crop stands out
        p.setClipRegion(QRegion(_image_rect) - QRegion(_selection));
        p.fillRect(_image_rect, QColor(0, 0, 0, 140));
        p.setClipping(false);

        p.setPen(QPen(Qt::white, 2));
        p.drawRect(_selection.adjusted(0, 0, -1, -1));

        const QRect handle(_selection.bottomRight() - QPoint(HANDLE_SIZE / 2, HANDLE_SIZE / 2),
                           QSize(HANDLE_SIZE, HANDLE_SIZE));
        p.fillRect(handle, Qt::white);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() != Qt::LeftButton || !_image_rect.contains(event->position().toPoint()))
            return;

        _press = event->position().toPoint();
        _start = _selection;
        if (HandleRect().contains(_press))
            _drag = Drag::Resize;
        else if (_selection.contains(_press))
            _drag = Drag::Move;
        else
        {
            _drag = Drag::New;
            ApplyDrag(_press);
        }
        update();
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        const QPoint pos = event->position().toPoint();
        if (_drag != Drag::None)
        {
            ApplyDrag(pos);
            return;
        }

        if (HandleRect().contains(pos))
            setCursor(Qt::SizeFDiagCursor);
        else if (_selection.contains(pos))
            setCursor(Qt::SizeAllCursor);
        else
            setCursor(Qt::CrossCursor);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton)
            _drag = Drag::None;
    }

private:
    enum class Drag
    {
        None,
        New,
        Move,
        Resize
    };

    QRect HandleRect() const
    {
        return QRect(_selection.bottomRight() - QPoint(HANDLE_SIZE / 2, HANDLE_SIZE / 2),
                     QSize(HANDLE_SIZE, HANDLE_SIZE));
    }

    void ResetSelection()
    {
        if (_image_rect.isEmpty())
            return;
        const int side = qMin(_image_rect.width(), _image_rect.height()) * 3 / 5;
        _selection = CropDialog::ClampSelection(
            QRect(_image_rect.center() - QPoint(side / 2, side / 2), QSize(side, side)), _image_rect,
            MIN_SELECTION);
    }

    void ApplyDrag(const QPoint& pos)
    {
        if (_drag == Drag::Move)
        {
            const QPoint delta = pos - _press;
            _selection = CropDialog::ClampSelection(_start.translated(delta), _image_rect,
                                                    MIN_SELECTION);
        }
        else
        {
            // both drags grow the square away from its anchor; for a new selection the
            // anchor is the press point, for a resize it is the current top-left corner
            const QPoint anchor = _drag == Drag::New ? _press : _selection.topLeft();
            const int side = qMax(MIN_SELECTION, qMax(qAbs(pos.x() - anchor.x()),
                                                     qAbs(pos.y() - anchor.y())));
            const int x = pos.x() >= anchor.x() ? anchor.x() : anchor.x() - side;
            const int y = pos.y() >= anchor.y() ? anchor.y() : anchor.y() - side;
            _selection = CropDialog::ClampSelection(QRect(x, y, side, side), _image_rect,
                                                    MIN_SELECTION);
        }
        update();
    }

    QPixmap _source;
    QRect _image_rect;
    QRect _selection;
    Drag _drag = Drag::None;
    QPoint _press;
    QRect _start;
};

CropDialog::CropDialog(const QPixmap& source, QWidget* parent)
    : QDialog(parent),
      _source(source),
      _view(new CropView(source, this))
{
    setWindowTitle(QStringLiteral("裁剪头像"));
    setModal(true);

    auto* hint = new QLabel(QStringLiteral("拖动方框选择区域，拖动右下角缩放"), this);
    hint->setAlignment(Qt::AlignCenter);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    layout->addWidget(_view, 0, Qt::AlignCenter);
    layout->addWidget(hint);
    layout->addWidget(buttons);
}

QPixmap CropDialog::CroppedPixmap() const
{
    const QRect image_rect = FitImage(_source.size(), _view->size());
    const QRect source_rect = MapSelectionToSource(_view->Selection(), image_rect, _source.size());
    if (source_rect.isEmpty())
        return QPixmap();
    return _source.copy(source_rect);
}

QRect CropDialog::FitImage(const QSize& image, const QSize& area)
{
    if (image.isEmpty() || area.isEmpty())
        return QRect();
    const QSize scaled = image.scaled(area, Qt::KeepAspectRatio);
    return QRect(QPoint((area.width() - scaled.width()) / 2,
                        (area.height() - scaled.height()) / 2),
                 scaled);
}

QRect CropDialog::ClampSelection(const QRect& selection, const QRect& bounds, int min_side)
{
    if (bounds.isEmpty())
        return QRect();

    const int max_side = qMin(bounds.width(), bounds.height());
    const int side = qBound(qMin(min_side, max_side), qMax(selection.width(), selection.height()),
                            max_side);
    const int x = qBound(bounds.left(), selection.left(), bounds.right() - side + 1);
    const int y = qBound(bounds.top(), selection.top(), bounds.bottom() - side + 1);
    return QRect(x, y, side, side);
}

QRect CropDialog::MapSelectionToSource(const QRect& selection, const QRect& image_rect,
                                       const QSize& source)
{
    if (selection.isEmpty() || image_rect.isEmpty() || source.isEmpty())
        return QRect();

    const qreal scale = qreal(image_rect.width()) / source.width();
    if (scale <= 0)
        return QRect();

    const QRect mapped(qRound((selection.left() - image_rect.left()) / scale),
                       qRound((selection.top() - image_rect.top()) / scale),
                       qRound(selection.width() / scale), qRound(selection.height() / scale));
    return mapped.intersected(QRect(QPoint(0, 0), source));
}
