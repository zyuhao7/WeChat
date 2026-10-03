#include "global.h"

QString gate_url_prefix = "";

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
