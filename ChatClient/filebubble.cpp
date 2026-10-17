#include "filebubble.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QMouseEvent>
#include <QUrl>
#include <QVBoxLayout>

#include "ElaIcon.h"

namespace
{
const int BUBBLE_WIDTH = 240;
const int ICON_SIZE = 36;
} // namespace

FileBubble::FileBubble(ChatRole role, const QString& name, qint64 size, QWidget* parent)
    : BubbleFrame(role, parent)
{
    setFixedWidth(BUBBLE_WIDTH);

    auto* body = new QWidget(this);
    body->setAttribute(Qt::WA_StyledBackground, false);

    _icon_lb = new QLabel(body);
    _icon_lb->setFixedSize(ICON_SIZE, ICON_SIZE);
    _icon_lb->setAlignment(Qt::AlignCenter);
    _icon_lb->setPixmap(ElaIcon::getInstance()->getElaIcon(ElaIconType::File, 22).pixmap(22, 22));
    _icon_lb->setStyleSheet("background: rgba(255, 255, 255, 40); border-radius: 8px;");

    _name_lb = new QLabel(name, body);
    _name_lb->setTextInteractionFlags(Qt::TextSelectableByMouse);
    _name_lb->setWordWrap(false);
    // a long name is elided rather than stretched, so the bubble keeps a fixed width
    _name_lb->setFixedWidth(BUBBLE_WIDTH - ICON_SIZE - 12 - 30);
    _name_lb->setStyleSheet(role == ChatRole::Self ? "color: #FFFFFF;" : "color: #1F1F1F;");

    _status_lb = new QLabel(FormatSize(size), body);
    _status_lb->setStyleSheet("color: rgba(0, 0, 0, 0.55);");

    auto* text_layout = new QVBoxLayout();
    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(2);
    text_layout->addWidget(_name_lb);
    text_layout->addWidget(_status_lb);

    auto* body_layout = new QHBoxLayout(body);
    body_layout->setContentsMargins(0, 0, 0, 0);
    body_layout->setSpacing(10);
    body_layout->addWidget(_icon_lb);
    body_layout->addLayout(text_layout);

    setWidget(body);
    setCursor(Qt::PointingHandCursor);
    ApplyStatusColor();
}

void FileBubble::setPath(const QString& path)
{
    // a send finishes without a local path of its own, and must not wipe the one it started with
    if (path.isEmpty())
        return;
    _path = path;
    setToolTip(path);
}

void FileBubble::setStatus(const QString& status)
{
    _status_lb->setText(status);
    ApplyStatusColor();
}

void FileBubble::setFailed(const QString& reason)
{
    _failed = true;
    setStatus(reason);
}

void FileBubble::ApplyStatusColor()
{
    if (_failed)
        _status_lb->setStyleSheet("color: #E5484D;");
    else
        _status_lb->setStyleSheet(role() == ChatRole::Self ? "color: rgba(255, 255, 255, 0.80);"
                                                           : "color: rgba(0, 0, 0, 0.55);");
}

QString FileBubble::FormatSize(qint64 size)
{
    if (size < 1024)
        return QStringLiteral("%1 B").arg(size);
    if (size < 1024 * 1024)
        return QStringLiteral("%1 KB").arg(double(size) / 1024.0, 0, 'f', 1);
    if (size < 1024LL * 1024 * 1024)
        return QStringLiteral("%1 MB").arg(double(size) / (1024.0 * 1024.0), 0, 'f', 1);
    return QStringLiteral("%1 GB").arg(double(size) / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
}

void FileBubble::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton && !_path.isEmpty() && QFileInfo::exists(_path))
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(_path));
    }
    BubbleFrame::mouseReleaseEvent(e);
}
