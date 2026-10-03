#include "textbubble.h"
#include "global.h"
#include <QEvent>
#include <QFontMetricsF>
#include <QFont>
#include <QTimer>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextLayout>

TextBubble::TextBubble(ChatRole role, const QString &text, QWidget *parent)
    :BubbleFrame(role, parent)
{
    m_pTextEdit = new QTextEdit();
    m_pTextEdit->setReadOnly(true);
    m_pTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTextEdit->installEventFilter(this);

    QFont font("Microsoft YaHei");
    font.setPointSize(12);
    m_pTextEdit->setFont(font);
    setPlainText(text);
    setWidget(m_pTextEdit);
    initStyleSheet();
}

bool TextBubble::eventFilter(QObject *o, QEvent *e)
{
    if(m_pTextEdit == o && e->type() == QEvent::Paint)
    {
        adjustTextHeight();
    }
    return BubbleFrame::eventFilter(o, e);
}

void TextBubble::adjustTextHeight()
{
        qreal doc_margin = m_pTextEdit->document()->documentMargin();    //default text-to-border distance is 4
        QTextDocument *doc = m_pTextEdit->document();
        qreal text_height = 0;

        //sum the segment heights = text height
        for (QTextBlock it = doc->begin(); it != doc->end(); it = it.next())
        {
            QTextLayout *pLayout = it.layout();
            QRectF text_rect = pLayout->boundingRect();                  // get the bounding rect of the current text block
            text_height += text_rect.height();
        }

        int vMargin = this->layout()->contentsMargins().top();

        //set the bubble height: text height + text margin + distance from the TextEdit border to the bubble border
        setFixedHeight(text_height + doc_margin * 2 + vMargin * 2 );
}

void TextBubble::setPlainText(const QString &text)
{
        m_pTextEdit->setPlainText(text);

       //find the max width among the segments
       qreal doc_margin = m_pTextEdit->document()->documentMargin();

       int margin_left = this->layout()->contentsMargins().left();
       int margin_right = this->layout()->contentsMargins().right();

       QFontMetricsF fm(m_pTextEdit->font());
       QTextDocument *doc = m_pTextEdit->document();
       int max_width = 0;

       //iterate the segments to find the widest one
       for (QTextBlock it = doc->begin(); it != doc->end(); it = it.next())    //total text length
       {
           int txtW = int(fm.horizontalAdvance(it.text()));
           max_width = max_width < txtW ? txtW : max_width;                 //find the longest segment
       }

       //set this bubble's max width; only needs to be set once
       setMaximumWidth(max_width + doc_margin * 2 + (margin_left + margin_right) + 10);        //set the max width
}

void TextBubble::initStyleSheet()
{
    m_pTextEdit->setStyleSheet("QTextEdit{background:transparent;border:none}");
}
