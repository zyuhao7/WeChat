#ifndef GLOBAL_H
#define GLOBAL_H

#include<QWidget>
#include<functional>
#include<iostream>
#include<memory>
#include<mutex>
#include<QByteArray>
#include<QNetworkReply>
#include<QJsonObject>
#include<QDir>
#include<QSettings>

#include "QStyle"

/**
 * @brief repolish used to refresh the qss
 */
extern std::function<void(QWidget*)> repolish;
extern std::function<QString(QString)> xorString;

enum ReqId{
       ID_GET_VERIFY_CODE = 1001, //get the verify code
       ID_REG_USER = 1002, //register the user
       ID_RESET_PWD = 1003, //reset the password
       ID_LOGIN_USER = 1004, //user login
       ID_CHAT_LOGIN = 1005, //log in to the chat server
       ID_CHAT_LOGIN_RSP = 1006, //chat-server login reply
       ID_SEARCH_USER_REQ = 1007, //user search request
       ID_SEARCH_USER_RSP = 1008, //search user reply
       ID_ADD_FRIEND_REQ = 1009,  //add the friend apply
       ID_ADD_FRIEND_RSP = 1010, //add-friend apply reply
       ID_NOTIFY_ADD_FRIEND_REQ = 1011,  //notify the user of the add-friend apply
       ID_AUTH_FRIEND_REQ = 1013,  //auth friend request
       ID_AUTH_FRIEND_RSP = 1014,  //auth friend reply
       ID_NOTIFY_AUTH_FRIEND_REQ = 1015, //notify the user of the friend auth apply
       ID_TEXT_CHAT_MSG_REQ  = 1017,  //text chat message request
       ID_TEXT_CHAT_MSG_RSP  = 1018,  //text chat message reply
       ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //notify the user of the text chat message
       ID_NOTIFY_OFF_LINE_REQ = 1021, // notify the user of going offline
       ID_HEART_BEAT_REQ = 1023,     // heartbeat request
       ID_HEART_BEAT_RSP = 1024     // heartbeat reply
};

enum Modules{
    REGISTERMOD = 0,
    RESETMOD = 1,
    LOGINMOD = 2,
};

enum TipErr{
    TIP_SUCCESS = 0,
    TIP_EMAIL_ERR = 1,
    TIP_PWD_ERR = 2,
    TIP_CONFIRM_ERR = 3,
    TIP_PWD_CONFIRM = 4,
    TIP_VERIFY_ERR = 5,
    TIP_USER_ERR = 6
};

enum ErrorCodes{
    SUCCESS = 0,
    ERR_JSON = 1,    // JSON parse failed
    ERR_NETWORK = 2, // network error
};

enum ClickLbState{
    Normal = 0,
    Selected = 1
};

extern QString gate_url_prefix;

struct ServerInfo
{
    QString Host;
    QString Port;
    QString Token;
    int Uid;
};

enum class ChatRole
{

    Self,
    Other
};

struct MsgInfo{
    QString msgFlag; //"text,image,file"
    QString content; //the url and text info of the file or image
    QPixmap pixmap; //thumbnail of the file or image
};

//the chat page modes
enum ChatUIMode{
    SearchMode, //search mode
    ChatMode, //chat mode
    ContactMode, //contact mode
};

//the custom QListWidgetItem types
enum ListItemType{
    CHAT_USER_ITEM, //chat user
    CONTACT_USER_ITEM, //contact user
    SEARCH_USER_ITEM, //the searched user
    ADD_USER_TIP_ITEM, //prompt to add a user
    INVALID_ITEM,  //non-clickable item
    GROUP_TIP_ITEM, //group tip item
    LINE_ITEM,  //separator line
    APPLY_FRIEND_ITEM, //friend apply
};

//minimum length of the friend-apply label input
const int MIN_APPLY_LABEL_ED_LEN = 40;

const QString add_prefix = "添加标签 ";

const int  tip_offset = 5;


const std::vector<QString>  strs ={"hello world !",
                             "nice to meet u",
                             "New year，new life",
                            "You have to love yourself",
                            "My love is written in the wind ever since the whole world is you"};

const std::vector<QString> heads = {
    ":/res/head_1.jpg",
    ":/res/head_2.jpg",
    ":/res/head_3.jpg",
    ":/res/head_4.jpg",
    ":/res/head_5.jpg",
    ":/res/head_6.jpg",
    ":/res/head_7.jpg",
    ":/res/head_8.jpg"
};

const std::vector<QString> names = {
    "HanMeiMei",
    "Lily",
    "Ben",
    "Androw",
    "Max",
    "Summer",
    "Candy",
    "Hunter"
};

const int CHAT_COUNT_PER_PAGE = 13;

#endif // GLOBAL_H
