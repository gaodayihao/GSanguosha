#ifndef _QSAN_JSON_UTILS_H
#define _QSAN_JSON_UTILS_H

#include <string>
#include <list>
#include <json/json.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qlist.h>

namespace QSanProtocol
{
    namespace Utils
    {        
        inline QString toQString(const Json::Value& value)
        {
            Q_ASSERT(value.isString());
            return QString(value.asCString());
        }
        inline Json::Value toJsonString(const QString& s)
        {            
            return Json::Value(s.toAscii().constData());
        }
        Json::Value toJsonArray(const QString& s1, const QString& s2);
        Json::Value toJsonArray(const QString& s1, const Json::Value& s2);
        Json::Value toJsonArray(const QString& s1, const QString& s2, const QString& s3);
        Json::Value toJsonIntArray(const QList<int>&);
        Json::Value toJsonStringArray(const QList<QString>&);
        Json::Value toJsonStringArray(const QStringList&);
        bool tryParse(const Json::Value&, QList<int> &);
        bool tryParse(const Json::Value&, QStringList &);
    }
}

#endif