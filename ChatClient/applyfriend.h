#ifndef APPLYFRIEND_H
#define APPLYFRIEND_H

#include <QDialog>
#include "clickedlabel.h"
#include "userdata.h"
#include "friendlabel.h"


namespace Ui {
class ApplyFriend;
}

class ApplyFriend : public QDialog
{
    Q_OBJECT

public:
    explicit ApplyFriend(QWidget *parent = nullptr);
    ~ApplyFriend();
    void InitTipLbs();
    void AddTipLbs(ClickedLabel*, QPoint cur_point, QPoint &next_point, int text_width, int text_height);
    bool eventFilter(QObject *obj, QEvent *event);
    void SetSearchInfo(std::shared_ptr<SearchInfo> si);
private:
    void resetLabels();
    Ui::ApplyFriend *ui;

    //已经创建好的标签
    QMap<QString, ClickedLabel*> _add_labels;
    std::vector<QString> _add_label_keys;
    QPoint _label_point;

    //用来在输入框显示添加新好友的标签
    QMap<QString, FriendLabel*> _friend_labels;
    std::vector<QString> _friend_label_keys;
    void addLabel(QString name);
    std::vector<QString> _tip_data; // 提示框中的数据
    QPoint _tip_cur_point;          // 当前提示框的位置
    std::shared_ptr<SearchInfo> _si;

public slots:
    //显示更多label标签
    void ShowMoreLabel();

    //按下回车键时，将输入的标签添加到展示区域
    void SlotLabelEnter();

    //移除好友标签
    void SlotRemoveFriendLabel(QString);

    //通过点击提示框来增加或减少好友标签
    void SlotChangeFriendLabelByTip(QString, ClickLbState);

    // 处理输入框文本的变化，更新提示框
    void SlotLabelTextChange(const QString& text);

    // 输入框编辑完成后触发
    void SlotLabelEditFinished();

   //点击提示框内容时，添加新的好友标签
    void SlotAddFirendLabelByClickTip(QString text);

//    //处理确认回调
    void SlotApplySure();

//    //处理取消回调
    void SlotApplyCancel();
};

#endif // APPLYFRIEND_H
