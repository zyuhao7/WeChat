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

    void insertFileFromUrl(const QStringList& urls); // 从给定的 URL 列表插入文件
signals:
    void send(); // 发送信号，用于触发消息发送事件

protected:
    void dragEnterEvent(QDragEnterEvent *event);  // 处理拖拽进入事件
    void dropEvent(QDropEvent *event);            // 处理拖拽释放事件
    void keyPressEvent(QKeyEvent *e);
private:
    void insertImages(const QString& url);
    void insertTextFile(const QString& url);

    bool canInsertFromMimeData(const QMimeData *source) const; // 判断是否可以从 MimeData 插入内容, source 是拖拽或粘贴的数据
    void insertFromMimeData(const QMimeData *source);

private:
    bool isImage(QString url);                                  // 判断文件是否为图片
    void insertMsgList(QVector<MsgInfo>& list, QString flag, QString text, QPixmap pix); // 向消息列表中插入一条消息

    QStringList getUrl(QString text);                           // 从文本中提取 URL
    QPixmap getFileIconPixmap(const QString& url);              // 获取文件图标 并转化为图片
    QString getFileSize(qint64 size);                           // 获取文件大小并格式化字符串

private slots:
    void textEditChanged();                                     // 处理文本内容变化事件. 当文本编辑框内容发生变化时调用，用于更新消息列表或其他操作

private:
    QVector<MsgInfo> mMsgList;      // 存储当前的消息列表
    QVector<MsgInfo> mGetMsgList;   // 存储获取的消息列表
};

#endif // MESSAGETEXTEDIT_H
