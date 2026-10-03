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
 This function extracts the message list (mGetMsgList) from the QTextEdit and returns it.
 the message list holds text and file messages (e.g. images, files).
*/

QVector<MsgInfo> MessageTextEdit::getMsgList()
{
    mGetMsgList.clear();

    QString doc = this->document()->toPlainText(); // get the plain text from the QTextEdit.
    QString text = "";  // temporarily store text messages
    int indexUrl = 0;  // index used to iterate mMsgList
    int count = mMsgList.size();

    for(int index = 0; index < doc.size(); index++)
    {
        // handle the file message
        if(doc[index] == QChar::ObjectReplacementCharacter)  // file message marker encountered
        {
            // if text is not empty there were pending text messages; insert them into mGetMsgList
            if(!text.isEmpty())
            {
                // handle the earlier text messages
                insertMsgList(mGetMsgList, "text", text, QPixmap());
                text.clear();
            }

            // iterate mMsgList to find the MsgInfo matching the current file message
            while(indexUrl < count)
            {
                MsgInfo msg = mMsgList[indexUrl];

                // if this->document()->toHtml() contains msg.content, it is a file message; add it to mGetMsgList.
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
            // handle the text message
            text.append(doc[index]);
        }
    }

    // handle the remaining text messages
    if(!text.isEmpty())
    {
        QPixmap pix;
        insertMsgList(mGetMsgList, "text", text, pix);
        text.clear();
    }

    mMsgList.clear();
    this->clear(); // clear the QTextEdit content
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
    insertFromMimeData(event->mimeData());  // get the dropped data; the type of event->mimeData() is const QMimeData*
    event->accept();                        // accept this drag; handled, do not propagate the event further
}

void MessageTextEdit::keyPressEvent(QKeyEvent *e)
{
    // // if the user pressed Enter or Return without holding Shift
    if((e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) && !(e->modifiers() & Qt::ShiftModifier))
    {
        emit send(); // trigger the send() signal to send the message
        return;      // prevent the default Enter-newline behavior
    }
    QTextEdit::keyPressEvent(e); // handle other keys normally by delegating to the base class
}

void MessageTextEdit::insertImages(const QString &url)
{
    QImage image(url);
    // scale the image proportionally
    if(image.width() > 120 || image.height() > 80)
    {
        if(image.width() > image.height())
        {
            image = image.scaledToWidth(120, Qt::SmoothTransformation);
        }
        else
            image = image.scaledToHeight(80, Qt::SmoothTransformation);
    }
    QTextCursor cursor = this->textCursor();    // get the current cursor position
    cursor.insertImage(image, url);             // insert the image into the text box

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

// build a QPixmap for the given file with icon, name and size, to show in the chat window
QPixmap MessageTextEdit::getFileIconPixmap(const QString &url)
{
    QFileIconProvider provider; // QFileIconProvider gets the system default file icon
    QFileInfo fileinfo(url);
    QIcon icon = provider.icon(fileinfo);

    QString strFileSize = getFileSize(fileinfo.size()); // turn the file size in bytes into a readable string (e.g. 1.23MB)

    QFont font(QString("宋体"), 10, QFont::Normal, false);
    QFontMetrics fontMetrics(font);
    QSize textSize = fontMetrics.size(Qt::TextSingleLine, fileinfo.fileName()); // compute the text size
    QSize FileSize =fontMetrics.size(Qt::TextSingleLine, strFileSize);          // compute the file size

    int maxWidth = textSize.width() > FileSize.width() ? textSize.width() : FileSize.width();
    QPixmap pix(50 + maxWidth + 10, 50); // total image width = icon width (50) + max text width + gap (10)
    pix.fill(); // clear the background

    QPainter painter;
    painter.begin(&pix);
    // file icon
    QRect rect(0, 0, 50, 50);
    painter.drawPixmap(rect, icon.pixmap(40, 40)); // draw the file icon in the left 50x50 area (inner icon 40x40)


    // file name
    painter.setPen(Qt::black);
    QRect rectText(50 + 10, 3, textSize.width(), textSize.height()); // draw the file name to the right of the icon, 3 pixels lower
    painter.drawText(rectText, fileinfo.fileName());

    // file size
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
//    convert num to a string with QString::number(), keeping 2 decimals ('f' means fixed-point format)
    return QString::number(num, 'f', 2) + " " + Unit;
}

void MessageTextEdit::textEditChanged()
{
    qDebug()<<"text Changed"<<Qt::endl;
}














