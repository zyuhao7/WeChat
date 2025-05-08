#ifndef CUSTOMIZEEDIT_H
#define CUSTOMIZEEDIT_H
#include <QLineEdit>
#include <QDebug>

class CustomizeEdit : public QLineEdit
{
    Q_OBJECT
public:
    CustomizeEdit(QWidget* parent = nullptr);
    void SetMaxLength(int maxLen);
protected:
    void focusOutEvent(QFocusEvent* event) override
    {
        // 执行失去焦点时候的处理逻辑
        QLineEdit::focusOutEvent(event);
        // 发送失去焦点的信号
        emit sig_focus_out();
    }
private:
    void limitTextLength(QString text)
    {
        if(_max_len <= 0) return;
        QByteArray byteArray = text.toUtf8();

        if(byteArray.size() > _max_len)
        {
            byteArray = byteArray.left(_max_len);
            this->setText(QString::fromUtf8(byteArray));
        }
    }
    int _max_len;
signals:
    void sig_focus_out();
};

#endif // CUSTOMIZEEDIT_H
