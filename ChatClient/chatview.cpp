#include "chatview.h"
#include <QScrollBar>
#include <QEvent>
#include <QStyleOption>
#include <QPainter>

ChatView::ChatView(QWidget* parent)
    : QWidget(parent),
      isAppended(false)
{
    QVBoxLayout* pMainLayout = new QVBoxLayout();
    this->setLayout(pMainLayout);
    pMainLayout->setContentsMargins(0, 0, 0, 0);

    m_pScrollArea = new QScrollArea();
    m_pScrollArea->setObjectName("chat_area");
    pMainLayout->addWidget(m_pScrollArea);

    QWidget* w = new QWidget(this);
    w->setObjectName("chat_bg");

    QVBoxLayout* pVLayout_1 = new QVBoxLayout();
    pVLayout_1->addWidget(new QWidget(), 10000);
    w->setLayout(pVLayout_1);
    m_pScrollArea->setWidget(w);

    m_pScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QScrollBar* pVScrollBar = m_pScrollArea->verticalScrollBar();
    connect(pVScrollBar, &QScrollBar::rangeChanged, this, &ChatView::onVScrollBarMoved);
    connect(pVScrollBar, &QScrollBar::rangeChanged, this, [this](int, int maximum) {
        if (!_prepend_pending)
            return;
        const int delta = maximum - _prepend_anchor_max;
        if (delta <= 0)
            return;
        QScrollBar* bar = m_pScrollArea->verticalScrollBar();
        bar->setValue(_prepend_anchor_value + delta);
        _prepend_anchor_max = maximum;
        _prepend_anchor_value = bar->value();
        _prepend_settle_timer->start();
    });

    _prepend_settle_timer = new QTimer(this);
    _prepend_settle_timer->setSingleShot(true);
    _prepend_settle_timer->setInterval(50);
    connect(_prepend_settle_timer, &QTimer::timeout, this, [this]() { _prepend_pending = false; });
    connect(pVScrollBar, &QScrollBar::valueChanged, this, [this](int value) {
        QScrollBar* bar = m_pScrollArea->verticalScrollBar();
        const bool at_top = value == 0 && bar->maximum() > 0;
        if (at_top && !_at_top_notified)
            emit sig_reached_top();
        _at_top_notified = at_top;
    });

    QHBoxLayout* pHLayout_2 = new QHBoxLayout();
    pHLayout_2->addWidget(pVScrollBar, 0, Qt::AlignRight);
    pHLayout_2->setContentsMargins(0, 0, 0, 0);
    m_pScrollArea->setLayout(pHLayout_2);
    pVScrollBar->setHidden(true);

    m_pScrollArea->setWidgetResizable(true);
    m_pScrollArea->installEventFilter(this);
    initStyleSheet();
}

void ChatView::appendChatItem(QWidget* item)
{
    QVBoxLayout* vl = qobject_cast<QVBoxLayout*>(m_pScrollArea->widget()->layout());
    vl->insertWidget(vl->count() - 1, item);
    isAppended = true;
}

void ChatView::prependChatItem(QWidget* item)
{
    prependChatItems({item});
}

void ChatView::prependChatItems(const QList<QWidget*>& items)
{
    if (items.isEmpty())
        return;

    // a pending append would otherwise snap the view back to the bottom after the insert
    isAppended = false;

    QVBoxLayout* vl = qobject_cast<QVBoxLayout*>(m_pScrollArea->widget()->layout());
    QScrollBar* bar = m_pScrollArea->verticalScrollBar();
    _prepend_anchor_max = bar->maximum();
    _prepend_anchor_value = bar->value();
    _prepend_pending = true;

    // items arrive oldest first, so index 0 keeps the visual order
    for (int i = 0; i < items.size(); ++i)
        vl->insertWidget(i, items[i]);

    // the range may stay unchanged (content still fits), which would leave the anchor pending
    _prepend_settle_timer->start();
}

void ChatView::insertChatItem(QWidget* before, QWidget* item)
{
    Q_UNUSED(before);
    Q_UNUSED(item);
}

void ChatView::removeAllItem()
{
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(m_pScrollArea->widget()->layout());

    int count = layout->count();

    for (int i = 0; i < count - 1; ++i)
    {
        QLayoutItem* item = layout->takeAt(0); // always delete starting from the first widget
        if (item)
        {
            if (QWidget* widget = item->widget())
            {
                delete widget;
            }
            delete item;
        }
    }
}

bool ChatView::eventFilter(QObject* o, QEvent* e)
{
    if (e->type() == QEvent::Enter && o == m_pScrollArea)
    {
        m_pScrollArea->verticalScrollBar()->setHidden(m_pScrollArea->verticalScrollBar()->maximum() == 0);
    }
    else if (e->type() == QEvent::Leave && o == m_pScrollArea)
    {
        m_pScrollArea->verticalScrollBar()->setHidden(true);
    }
    return QWidget::eventFilter(o, e);
}

void ChatView::paintEvent(QPaintEvent* event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ChatView::onVScrollBarMoved(int min, int max)
{
    if (isAppended) // adding an item may be called multiple times
    {
        QScrollBar* pVScrollBar = m_pScrollArea->verticalScrollBar();
        pVScrollBar->setSliderPosition(pVScrollBar->maximum());

        // may fire multiple times within 500 ms
        QTimer::singleShot(500, [this]() {
            isAppended = false;
        });
    }
}

void ChatView::initStyleSheet()
{
    m_pScrollArea->setFrameShape(QFrame::NoFrame);
    m_pScrollArea->setStyleSheet("QScrollArea#chat_area, QWidget#chat_bg { background: transparent; }");
    m_pScrollArea->viewport()->setAutoFillBackground(false);
}
