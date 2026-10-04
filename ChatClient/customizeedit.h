#ifndef CUSTOMIZEEDIT_H
#define CUSTOMIZEEDIT_H
#include <QDebug>
#include "ElaLineEdit.h"

// Extends the Fluent ElaLineEdit so the friend-apply inputs and the chat
// search box render in the Ela style while keeping the custom max-length
// and focus-out signal this app relies on.
class CustomizeEdit : public ElaLineEdit
{
    Q_OBJECT
public:
    CustomizeEdit(QWidget* parent = nullptr);
    void SetMaxLength(int maxLen);
protected:
    void focusOutEvent(QFocusEvent* event) override
    {
        // run the Ela focus-out handling (hides the clear button, animates the mark)
        ElaLineEdit::focusOutEvent(event);
        // emit the focus-out signal
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
