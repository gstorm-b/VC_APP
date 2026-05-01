#ifndef TASK_LOCALIZATION_CONFIG_H
#define TASK_LOCALIZATION_CONFIG_H

#include <QSharedData>
#include <QJsonObject>
#include <QJsonArray>

#include "model/itask_config.h"
#include "matching/pattern_group_manager.h"
#include "qgadget_marco.h"

template<typename K, typename V>
static QJsonArray mapToJsonObject(QMap<K, V> &mapping) {
    QJsonArray arr;
    auto map_it = mapping.cbegin();
    while (map_it != mapping.cend()) {
        QJsonObject obj;
        obj["key"] = map_it.key();
        obj["value"] = map_it.value();
        arr.append(obj);
        map_it++;
    }
    return arr;
}

template<typename K, typename V>
static QMap<K, V> mapFromJsonObject(QJsonArray &arr) {
    QMap<K, V> mapping;
    for (int idx=0;idx<arr.size();idx++) {
        QJsonObject obj = arr[idx].toObject();
        K key = qvariant_cast<K>(obj["key"].toVariant());
        V value = qvariant_cast<V>(obj["value"].toVariant());
        mapping.insert(key, value);
    }
    return mapping;
}

namespace vc::model {

class TaskLocalizeConfigPrivate : public QSharedData {
public:
    TaskLocalizeConfigPrivate() {}

    TaskLocalizeConfigPrivate(const TaskLocalizeConfigPrivate &other) = default;

    QString m_outputDevice;
    QStringList m_cameras;

    // num
    QString m_nActiveCamera;
    QString m_nActivePattern;

    // bool
    QString m_bCameraValid;
    QString m_bPatternValid;
    QString m_bTaskReady;

    QString m_bExecuteTrigger;
    QString m_bMatchingFinished;
    QString m_bMatchingBusy;
    QString m_bMatchingDetected;
    QString m_bMatchingLowArea;
    QString m_nDetectedNumber;

    QString m_sCommDeviceId;
    QString m_sOutputDeviceId;

    // <active number, Device id>
    QMap<int, QString> m_sCameraNumberMap;


};

class TaskLocalizeConfig : public ITaskConfig {
    Q_GADGET

    P_PROPERTY_STRING_READWRITE(QString, nActiveCamera, "Camera selection")
    P_PROPERTY_STRING_READWRITE(QString, nActivePattern, "Pattern selection")

    P_PROPERTY_STRING_READWRITE(QString, bCameraValid, "Camera ready")
    P_PROPERTY_STRING_READWRITE(QString, bPatternValid, "Pattern ready")
    P_PROPERTY_STRING_READWRITE(QString, bTaskReady, "Task ready")

    P_PROPERTY_STRING_READWRITE(QString, bExecuteTrigger, "Trigger")
    P_PROPERTY_STRING_READWRITE(QString, bMatchingFinished, "Finished")
    P_PROPERTY_STRING_READWRITE(QString, bMatchingBusy, "Busy")
    P_PROPERTY_STRING_READWRITE(QString, bMatchingDetected, "Detected")
    P_PROPERTY_STRING_READWRITE(QString, bMatchingLowArea, "Low Area")
    P_PROPERTY_STRING_READWRITE(QString, nDetectedNumber, "Detected number")

public:
    explicit TaskLocalizeConfig()
        : ITaskConfig(), d(new TaskLocalizeConfigPrivate()) {}

    TaskType taskType() const override {
        return TaskType::LocalizationTask;
    }

    const QMetaObject &getMetaObject() const override {
        return vc::model::TaskLocalizeConfig::staticMetaObject;
    }

    QJsonObject toJson() const override {
        QJsonObject obj;
        obj["nActiveCamera"]     = d->m_nActiveCamera;
        obj["nActivePattern"]    = d->m_nActivePattern;
        obj["nDetectedNumber"]   = d->m_nDetectedNumber;

        obj["bCameraValid"]      = d->m_bCameraValid;
        obj["bPatternValid"]     = d->m_bPatternValid;
        obj["bTaskReady"]        = d->m_bTaskReady;
        obj["bExecuteTrigger"]   = d->m_bExecuteTrigger;
        obj["bMatchingFinished"] = d->m_bMatchingFinished;
        obj["bMatchingBusy"]     = d->m_bMatchingBusy;
        obj["bMatchingDetected"] = d->m_bMatchingDetected;
        obj["bMatchingLowArea"]  = d->m_bMatchingLowArea;

        obj["sCommDeviceId"]  = d->m_sCommDeviceId;
        obj["sOutputDeviceId"]  = d->m_sOutputDeviceId;

        QMap<int, QString> map = d->m_sCameraNumberMap;
        obj["sCameraNumberMap"] = mapToJsonObject<int, QString>(map);

        return obj;
    }


    bool fromJson(const QJsonObject& obj) override {
        if (obj.empty()) {
            return false;
        }

        d->m_nActiveCamera      = obj["nActiveCamera"].toString("");
        d->m_nActivePattern     = obj["nActivePattern"].toString("");
        d->m_nDetectedNumber    = obj["nDetectedNumber"].toString("");

        d->m_bCameraValid       = obj["bCameraValid"].toString("");
        d->m_bPatternValid      = obj["bPatternValid"].toString("");
        d->m_bTaskReady         = obj["bTaskReady"].toString("");
        d->m_bExecuteTrigger    = obj["bExecuteTrigger"].toString("");
        d->m_bMatchingFinished  = obj["bMatchingFinished"].toString("");
        d->m_bMatchingBusy      = obj["bMatchingBusy"].toString("");
        d->m_bMatchingDetected  = obj["bMatchingDetected"].toString("");
        d->m_bMatchingLowArea   = obj["bMatchingLowArea"].toString("");

        d->m_sCommDeviceId   = obj["sCommDeviceId"].toString("");
        d->m_sOutputDeviceId   = obj["sOutputDeviceId"].toString("");

        QJsonArray map_arr = obj["sCameraNumberMap"].toArray();
        d->m_sCameraNumberMap = mapFromJsonObject<int, QString>(map_arr);

        return true;
    }

    TaskLocalizeConfig(const TaskLocalizeConfig &other) : d(other.d) {}

    TaskLocalizeConfig &operator=(const TaskLocalizeConfig &other) {
        if (this != &other) d = other.d;
        return *this;
    }

    virtual ITaskConfig* copy() override {
        TaskLocalizeConfig *ptr = new TaskLocalizeConfig();
        *ptr = *this;
        return ptr;
    }

public:
    QSharedDataPointer<TaskLocalizeConfigPrivate> d;
};


} // namespace vc::model


#endif // TASK_LOCALIZATION_CONFIG_H
