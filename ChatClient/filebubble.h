#ifndef FILEBUBBLE_H
#define FILEBUBBLE_H

#include <QLabel>
#include <QString>

#include "bubbleframe.h"

// A file message is one bubble: name on top, size and transfer status underneath. The bytes
// never travel with the message — the bubble only learns the local path once the transfer has
// finished, so a click before that has nothing to open.
class FileBubble : public BubbleFrame
{
    Q_OBJECT
public:
    FileBubble(ChatRole role, const QString& name, qint64 size, QWidget* parent = nullptr);

    void setPath(const QString& path);
    void setStatus(const QString& status);
    void setFailed(const QString& reason);

    static QString FormatSize(qint64 size);

protected:
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    void ApplyStatusColor();

    QString _path;
    bool _failed = false;
    QLabel* _icon_lb = nullptr;
    QLabel* _name_lb = nullptr;
    QLabel* _status_lb = nullptr;
};

#endif // FILEBUBBLE_H
