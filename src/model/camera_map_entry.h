#ifndef CAMERA_MAP_ENTRY_H
#define CAMERA_MAP_ENTRY_H

#include <QString>
#include <QJsonObject>
#include <QMetaType>

namespace vc::model {

class CameraMapEntry {
    Q_GADGET

    Q_PROPERTY(int     signalValue
                   READ signalValue WRITE setSignalValue)
    Q_PROPERTY(QString cameraDeviceId
                   READ cameraDeviceId WRITE setCameraDeviceId)

public:
    CameraMapEntry() = default;
    CameraMapEntry(int sv, const QString& camId)
        : m_signalValue(sv), m_cameraDeviceId(camId) {}

    int     signalValue()    const { return m_signalValue;    }
    QString cameraDeviceId() const { return m_cameraDeviceId; }

    void setSignalValue(int v)           { m_signalValue    = v; }
    void setCameraDeviceId(const QString& v) { m_cameraDeviceId = v; }

    QJsonObject toJson() const {
        return QJsonObject {
                           { "signalValue",    m_signalValue    },
                           { "cameraDeviceId", m_cameraDeviceId },
                           };
    }

    static CameraMapEntry fromJson(const QJsonObject& o) {
        return CameraMapEntry {
            o["signalValue"].toInt(),
            o["cameraDeviceId"].toString()
        };
    }

private:
    int     m_signalValue    = 0;
    QString m_cameraDeviceId;
};

} // namespace vc::model

Q_DECLARE_METATYPE(vc::model::CameraMapEntry)

#endif // CAMERA_MAP_ENTRY_H
