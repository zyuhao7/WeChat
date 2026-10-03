#ifndef CLICKEDBTN_H
#define CLICKEDBTN_H

#include <QPushButton>
#include <QEnterEvent>

class ClickedBtn : public QPushButton
{
    Q_OBJECT
public:
    ClickedBtn(QWidget* parent = nullptr);
    ~ClickedBtn();
    void SetState(QString normal, QString hover, QString press);
protected:
    virtual void enterEvent(QEnterEvent* event) override ; // mouse enter
    virtual void leaveEvent(QEvent* event) override;  //mouse leave
    virtual void mousePressEvent(QMouseEvent* event) override; // mouse press
    virtual void mouseReleaseEvent(QMouseEvent* event) override; //mouse release
private:
    QString _normal;
    QString _hover;
    QString _press;
};

#endif // CLICKEDBTN_H
