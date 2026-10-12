#include "global.h"

QString gate_url_prefix = "";

QString serverErrorText(int error)
{
    switch (error) {
    case 1:
    case 2:
    case 1002: return QStringLiteral("服务异常，请稍后重试");
    case 3: return QStringLiteral("邮箱格式不正确");
    case 4: return QStringLiteral("请求过于频繁，请稍后再试");
    case 1001: return QStringLiteral("请求参数错误");
    case 1003: return QStringLiteral("验证码已过期，请重新获取");
    case 1004: return QStringLiteral("验证码错误");
    case 1005: return QStringLiteral("用户名或邮箱已被注册");
    case 1006: return QStringLiteral("两次输入的密码不一致");
    case 1007: return QStringLiteral("用户名与邮箱不匹配");
    case 1008: return QStringLiteral("密码更新失败，请稍后重试");
    case 1009: return QStringLiteral("邮箱或密码错误");
    case 1010: return QStringLiteral("登录已失效，请重新登录");
    case 1011: return QStringLiteral("用户不存在或无效");
    case 1012: return QStringLiteral("数据库异常，请稍后重试");
    default: return QStringLiteral("请求失败 (错误码 %1)").arg(error);
    }
}

std::function<void(QWidget*)> repolish = [](QWidget* w)
{
    w->style()->unpolish(w);
    w->style()->polish(w);
};

std::function<QString(QString)> xorString = [](QString input){
    QString result = input; // copy the original string so it can be modified
     int length = input.length(); // get the string length
     ushort xor_code = length % 255;
     for (int i = 0; i < length; ++i) {
         // XOR each character
         // Note: characters are assumed ASCII here, so they convert directly to QChar
         result[i] = QChar(static_cast<ushort>(input[i].unicode() ^ xor_code));
     }
     return result;
};
