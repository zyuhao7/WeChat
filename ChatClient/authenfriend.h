#ifndef AUTHENFRIEND_H
#define AUTHENFRIEND_H

#include <QDialog>
#include "clickedlabel.h"
#include "userdata.h"
#include "friendlabel.h"

namespace Ui {
class AuthenFriend;
}

class AuthenFriend : public QDialog
{
    Q_OBJECT

public:
    explicit AuthenFriend(QWidget *parent = nullptr);
    ~AuthenFriend();
    void InitTipLbs();
    void AddTipLbs(ClickedLabel*, QPoint cur_point, QPoint &next_point, int text_width, int text_height);
    bool eventFilter(QObject *obj, QEvent *event);
    void SetApplyInfo(std::shared_ptr<ApplyInfo> apply_info);
private:
    void resetLabels();

    //an already-created label
    QMap<QString, ClickedLabel*> _add_labels;
    std::vector<QString> _add_label_keys;
    QPoint _label_point;

    //used to show new-friend labels in the input box
    QMap<QString, FriendLabel*> _friend_labels;
    std::vector<QString> _friend_label_keys;
    void addLabel(QString name);
    std::vector<QString> _tip_data; // data in the tip box
    QPoint _tip_cur_point;          // current tip box position
    std::shared_ptr<SearchInfo> _si;

public slots:
    //show more labels
    void ShowMoreLabel();

//    on Enter, add the entered label to the display area
    void SlotLabelEnter();

    //remove the friend label
    void SlotRemoveFriendLabel(QString);

    //add or remove friend labels by clicking the tip box
    void SlotChangeFriendLabelByTip(QString, ClickLbState);

    // handle input-box text changes and update the tip box
    void SlotLabelTextChange(const QString& text);

    // triggered after the input box finishes editing
    void SlotLabelEditFinished();

   //on clicking the tip content, add a new friend label
    void SlotAddFirendLabelByClickTip(QString text);

//    // handle the confirm-verify callback
    void SlotAuthenSure();

//    // handle the cancel-verify callback
    void SlotAuthenCancel();

private:
    std::shared_ptr<ApplyInfo> _apply_info;
    Ui::AuthenFriend *ui;
};

#endif // AUTHENFRIEND_H
