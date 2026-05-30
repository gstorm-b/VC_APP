#ifndef TASK_LOCALIZATION_CONFIG_H
#define TASK_LOCALIZATION_CONFIG_H

#include <QSharedData>
#include <QJsonObject>

#include "model/itask_config.h"
#include "model/task_device_binding.h"
#include "matching/pattern_group_manager.h"
#include "qgadget_marco.h"

namespace vc::model {

class TaskLocalizeConfigPrivate : public QSharedData {
public:
    TaskLocalizeConfigPrivate() {}

    TaskLocalizeConfigPrivate(const TaskLocalizeConfigPrivate &other) = default;

    // number signals
    QString m_nActiveCamera;
    QString m_nActivePattern;

    // bool signals
    QString m_bCameraValid;
    QString m_bPatternValid;
    QString m_bTaskReady;

    QString m_bExecuteTrigger;
    QString m_bMatchingFinished;
    QString m_bMatchingBusy;
    QString m_bMatchingDetected;
    QString m_bMatchingLowArea;
    QString m_nDetectedNumber;

    TaskDeviceBindings m_deviceBindings;
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

        obj["deviceBindings"] = d->m_deviceBindings.toJson();

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

        if (!d->m_deviceBindings.fromJson(obj["deviceBindings"])) {
            return false;
        }

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
