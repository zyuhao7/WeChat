#include "messagetextedit.h"
#include <QDebug>
#include <QMessageBox>

MessageTextEdit::MessageTextEdit(QWidget *parent)
    :QTextEdit(parent)
{
    this->setMaximumHeight(60);
}

MessageTextEdit::~MessageTextEdit()
{

}
/*
 该函数的主要功能是从 QTextEdit 控件中提取消息列表（mGetMsgList），并返回提取的消息列表。
 消息列表包括文本消息和文件消息（如图片、文件等）。
*/

QVector<MsgInfo> MessageTextEdit::getMsgList()
{
    mGetMsgList.clear();

    QString doc = this->document()->toPlainText(); // 获取 QTextEdit 中的纯文本内容。
    QString text = "";  // 临时存储文本消息
    int indexUrl = 0;  // 用于遍历 mMsgList 的索引
    int count = mMsgList.size();

    for(int index = 0; index < doc.size(); index++)
    {
        // 处理文件消息
        if(doc[index] == QChar::ObjectReplacementCharacter)  // 遇到文件消息标记
        {
            // 如果 text 不为空，说明之前有未处理的文本消息，将其插入到 mGetMsgList 中
            if(!text.isEmpty())
            {
                // 处理之前的文本消息
                insertMsgList(mGetMsgList, "text", text, QPixmap());
                text.clear();
            }

            // 遍历 mMsgList，找到与当前文件消息匹配的 MsgInfo 对象
            while(indexUrl < count)
            {
                MsgInfo msg = mMsgList[indexUrl];

                // 如果 this->document()->toHtml() 包含 msg.content，说明这是一个文件消息，将其添加到 mGetMsgList 中。
                if(this->document()->toHtml().contains(msg.content, Qt::CaseSensitive))
                {
                    indexUrl++;
                    mGetMsgList.append(msg);
                    break;
                }
                indexUrl++;
            }
        }
        else
        {
            // 处理文本消息
            text.append(doc[index]);
        }
    }

    // 处理剩余的文本消息
    if(!text.isEmpty())
    {
        QPixmap pix;
        insertMsgList(mGetMsgList, "text", text, pix);
        text.clear();
    }

    mMsgList.clear();
    this->clear(); // 清空 QTextEdit 的内容
    return mGetMsgList;
}

void MessageTextEdit::insertFileFromUrl(const QStringList &urls)
{
    if(urls.isEmpty())
        return;
    foreach (QString url, urls){
        if(isImage(url))
            insertImages(url);
        else
            insertTextFile(url);
    }
}

void MessageTextEdit::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->source() == this)
        event->ignore();
    else
        event->accept();
}

void MessageTextEdit::dropEvent(QDropEvent *event)
{
    insertFromMimeData(event->mimeData());  // 获取被拖拽过来的 数据内容，event->mimeData() 的类型是 const QMimeData*
    event->accept();                        // 表示接收了这次拖拽操作，事件处理完毕  !阻止事件往上传递
}

void MessageTextEdit::keyPressEvent(QKeyEvent *e)
{
    // // 如果用户按下的是 Enter 或 Return，并且没有按住 Shift 键
    if((e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) && !(e->modifiers() & Qt::ShiftModifier))
    {
        emit send(); // 触发 send() 信号，执行发送消息操作
        return;      // 阻止回车换行的默认行为
    }
    QTextEdit::keyPressEvent(e); // 其他按键正常处理，交给父类
}

void MessageTextEdit::insertImages(const QString &url)
{
    QImage image(url);
    // 按比例缩放图片
    if(image.width() > 120 || image.height() > 80)
    {
        if(image.width() > image.height())
        {
            image = image.scaledToWidth(120, Qt::SmoothTransformation);
        }
        else
            image = image.scaledToHeight(80, Qt::SmoothTransformation);
    }
    QTextCursor cursor = this->textCursor();    // 获取当前光标位置
    cursor.insertImage(image, url);             // 把图片插入到文本框中

    insertMsgList(mMsgList, "image", url, QPixmap::fromImage(image));
}

void MessageTextEdit::insertTextFile(const QString &url)
{
    QFileInfo fileInfo(url);
    if(fileInfo.isDir())
    {
        QMessageBox::information(this, "提示", "只允许拖动单个文件!");
        return;
    }
    if(fileInfo.size() > 100 * 1024 * 1024)
    {
        QMessageBox::information(this, "提示", "发送的文件大小不能大于 100M");
        return;
    }

    QPixmap pix = getFileIconPixmap(url);
    QTextCursor cursor = this->textCursor();
    cursor.insertImage(pix.toImage(), url);
    insertMsgList(mMsgList,"file", url, pix);
}

bool MessageTextEdit::canInsertFromMimeData(const QMimeData *source) const
{
    return QTextEdit::canInsertFromMimeData(source);
}

void MessageTextEdit::insertFromMimeData(const QMimeData *source)
{
    QStringList urls = getUrl(source->text());

    if(urls.isEmpty())
        return;
    foreach(QString url, urls)
    {
        if(isImage(url))
            insertImages(url);
        else
            insertTextFile(url);
    }
}

bool MessageTextEdit::isImage(QString url)
{
     QString imageFormat = "bmp,jpg,png,tif,gif,pcx,tga,exif,fpx,svg,psd,cdr,pcd,dxf,ufo,eps,ai,raw,wmf,webp";
     QStringList imageFormatList = imageFormat.split(",");

     QFileInfo fileInfo(url);
     QString suffix = fileInfo.suffix().toLower();
     if (imageFormatList.contains(suffix))
         return true;
     return false;
}

void MessageTextEdit::insertMsgList(QVector<MsgInfo> &list, QString flag, QString text, QPixmap pix)
{
    MsgInfo msg;
    msg.msgFlag = flag;
    msg.content = text;
    msg.pixmap = pix;
    list.append(msg);
}

QStringList MessageTextEdit::getUrl(QString text)
{
    QStringList urls;
    if(text.isEmpty()) return urls;

    QStringList list = text.split("\n");
    foreach (QString url, list)
    {
        if(!url.isEmpty())
        {
            QStringList str = url.split("///");
            if(str.size() >= 2)
                urls.append(str.at(1));
        }
    }
    return urls;
}

// 为传入的文件生成一张带有图标、文件名和文件大小信息的 QPixmap 图片，用于显示在聊天窗口里
QPixmap MessageTextEdit::getFileIconPixmap(const QString &url)
{
    QFileIconProvider provider; // QFileIconProvider 获取系统默认的文件图标
    QFileInfo fileinfo(url);
    QIcon icon = provider.icon(fileinfo);

    QString strFileSize = getFileSize(fileinfo.size()); // 把文件大小（字节数）转成可读字符串（比如 1.23MB）

    QFont font(QString("宋体"), 10, QFont::Normal, false);
    QFontMetrics fontMetrics(font);
    QSize textSize = fontMetrics.size(Qt::TextSingleLine, fileinfo.fileName()); // 计算文字的尺寸
    QSize FileSize =fontMetrics.size(Qt::TextSingleLine, strFileSize);          // 计算文件大小

    int maxWidth = textSize.width() > FileSize.width() ? textSize.width() : FileSize.width();
    QPixmap pix(50 + maxWidth + 10, 50); // 图片总宽度 = 图标宽（50）+ 最大文本宽 + 间隔（10)
    pix.fill(); // 清空背景

    QPainter painter;
    painter.begin(&pix);
    // 文件图标
    QRect rect(0, 0, 50, 50);
    painter.drawPixmap(rect, icon.pixmap(40, 40)); // 在左侧（50x50）区域画文件图标（内部图标 40x40）


    // 文件名称
    painter.setPen(Qt::black);
    QRect rectText(50 + 10, 3, textSize.width(), textSize.height()); // 文件名绘制在图标右侧，稍微往下 3 像素
    painter.drawText(rectText, fileinfo.fileName());

    // 文件大小
    QRect rectFile(50 + 10, textSize.height() + 5, FileSize.width(), FileSize.height());
    painter.drawText(rectFile, strFileSize);
    painter.end();
    return pix;
}

QString MessageTextEdit::getFileSize(qint64 size)
{
    QString Unit;
    double num;
    if(size < 1024)
    {
        num = size;
        Unit = "B";
    }
    else if(size < 1024 * 1024)
    {
        num = size / 1024.0;
        Unit = "KB";
    }
    else
    {
        num = size / 1024.0 / 1024.0 / 1024.0;
        Unit = "GB";
    }
//    使用 QString::number() 方法将 num 转换为字符串，保留 2 位小数（'f' 格式代表固定小数点格式）
    return QString::number(num, 'f', 2) + " " + Unit;
}

void MessageTextEdit::textEditChanged()
{
    qDebug()<<"text Changed"<<Qt::endl;
}














