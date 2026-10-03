#ifndef MESSAGETEXTEDIT_H
#define MESSAGETEXTEDIT_H
#include <QObject>
#include <QTextEdit>
#include <QMouseEvent>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMimeType>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QPainter>
#include <QVector>
#include "global.h"


class MessageTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit MessageTextEdit(QWidget* parent = nullptr);

    ~MessageTextEdit();

    QVector<MsgInfo> getMsgList();

    void insertFileFromUrl(const QStringList& urls); // insert files from the given URL list
signals:
    void send(); // emit a signal to trigger the message-send event

protected:
    void dragEnterEvent(QDragEnterEvent *event);  // handle the drag-enter event
    void dropEvent(QDropEvent *event);            // handle the drag-drop event
    void keyPressEvent(QKeyEvent *e);
private:
    void insertImages(const QString& url);
    void insertTextFile(const QString& url);

    bool canInsertFromMimeData(const QMimeData *source) const; // check whether content can be inserted from MimeData; source is dropped or pasted data
    void insertFromMimeData(const QMimeData *source);

private:
    bool isImage(QString url);                                  // check whether the file is an image
    void insertMsgList(QVector<MsgInfo>& list, QString flag, QString text, QPixmap pix); // append a message to the message list

    QStringList getUrl(QString text);                           // extract URLs from the text
    QPixmap getFileIconPixmap(const QString& url);              // get the file icon and convert it to an image
    QString getFileSize(qint64 size);                           // get the file size and format the string

private slots:
    void textEditChanged();                                     // handle the text-changed event; called when the editor content changes, to update the message list or do other work

private:
    QVector<MsgInfo> mMsgList;      // store the current message list
    QVector<MsgInfo> mGetMsgList;   // store the fetched message list
};

#endif // MESSAGETEXTEDIT_H
