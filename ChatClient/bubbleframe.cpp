#include "bubbleframe.h"
#include <QPainter>
#include <QPainterPath>

BubbleFrame::BubbleFrame(ChatRole role, QWidget *parent)
    : QFrame(parent),
      m_role(role),
      m_margin(4)
{
    m_pHLayout = new QHBoxLayout();
    m_pHLayout->setContentsMargins(m_margin + 6, m_margin, m_margin + 6, m_margin);
    this->setLayout(m_pHLayout);
}

void BubbleFrame::setMargin(int margin)
{
    Q_UNUSED(margin);
}

void BubbleFrame::setWidget(QWidget *w)
{
    // check whether the layout already has child widgets
    if(m_pHLayout->count() > 0)
        return;
    else
        m_pHLayout->addWidget(w);
}

ChatRole BubbleFrame::role() const
{
    return m_role;
}

void BubbleFrame::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF body = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal radius = qMin<qreal>(16, body.height() / 2);
    QPainterPath shape;
    shape.addRoundedRect(body, radius, radius);

    if(m_role == ChatRole::Self)
    {
        QLinearGradient fill(body.topLeft(), body.bottomLeft());
        fill.setColorAt(0.0, QColor(0x3D, 0x9C, 0xFF));
        fill.setColorAt(1.0, QColor(0x0A, 0x84, 0xFF));
        painter.fillPath(shape, fill);
        painter.setPen(QPen(QColor(255, 255, 255, 90), 1));
    }
    else
    {
        painter.fillPath(shape, QColor(255, 255, 255, 215));
        painter.setPen(QPen(QColor(255, 255, 255, 240), 1));
    }
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(shape);

    QFrame::paintEvent(e);
}
