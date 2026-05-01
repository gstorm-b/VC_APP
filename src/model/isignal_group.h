#ifndef ISIGNAL_GROUP_H
#define ISIGNAL_GROUP_H

#include <QObject>

namespace vc::model {

class ISignalGroup : public QObject   {
    Q_OBJECT

public:
    explicit ISignalGroup(QObject* parent = nullptr)
        : QObject(parent) {

    }

    virtual ~ISignalGroup() = default;

    virtual QJsonObject toJson() const = 0;
    virtual bool fromJson(const QObject& obj) = 0;
};

}

#endif // ISIGNAL_GROUP_H
