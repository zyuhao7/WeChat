#pragma once
#include <string>
struct UserInfo
{
    UserInfo()
        : name(""),
          pwd(""),
          uid(0),
          nick(""),
          desc(""),
          sex(0),
          icon(""),
          back("")
    {
    }

    std::string name;
    std::string pwd;
    int uid;
    std::string email;
    std::string nick;
    std::string desc;
    int sex;
    std::string icon;
    std::string back;
};

struct ApplyInfo
{
    ApplyInfo(int uid, std::string name, std::string desc, std::string icon, std::string nick, int sex, int status)
        : _uid(uid),
          _name(name),
          _desc(desc),
          _icon(icon),
          _nick(nick),
          _sex(sex),
          _status(status)
    {
    }

    int _uid;
    std::string _name;
    std::string _desc;
    std::string _icon;
    std::string _nick;
    int _sex;
    int _status;
};

struct ChatMessageInfo
{
    ChatMessageInfo(long long seq, std::string msg_id, int from_uid, int to_uid, std::string content)
        : _seq(seq), _msg_id(msg_id), _from_uid(from_uid), _to_uid(to_uid), _content(content)
    {
    }

    long long _seq; // auto-increment id, also the ordering key
    std::string _msg_id;
    int _from_uid;
    int _to_uid;
    std::string _content;
};