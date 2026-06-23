#ifndef CAMERA_WORKSPACE_H
#define CAMERA_WORKSPACE_H

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QMetaType>
#include <QString>

#include <opencv2/core.hpp>

namespace vc::model {

// ---------------------------------------------------------------------------
// CameraWorkspace — per-camera workspace (ROI) used to crop the grabbed image
// before matching.
//
// Keyed by camera device id (stable across logical camera-number
// reassignment). The reference image (the last image used to define the ROI)
// is kept in memory so the workspace dialog can reopen it; it is persisted
// out-of-band as a BLOB through the project_images table (see
// CameraWorkspaceMap::imageKey). Only the ROI rect + flag are serialized to
// JSON.
//
// Default is "workspace off": matching runs on the full grabbed frame.
// ---------------------------------------------------------------------------
struct CameraWorkspace {
    // ── Working workspace — crops the grabbed frame before matching ────────
    bool       useWorkspace = false;          // false => match the full frame
    cv::Rect2f roi{0.0f, 0.0f, 0.0f, 0.0f};   // image-pixel coordinates

    // ── Condition workspace — task-dependent region of interest ────────────
    // Unlike the working workspace (which crops the input), the condition
    // workspace is interpreted by the owning task. For a localization task it
    // is the region used to filter which detected objects count as valid.
    // Defined in the same frame's pixel coordinates as the working ROI and
    // shares this camera's reference image. LocalizationPipeline passes it to
    // ImageMatcher::setMatchingConditionROI() when enabled.
    bool       useConditionWorkspace = false;
    cv::Rect2f conditionRoi{0.0f, 0.0f, 0.0f, 0.0f};   // image-pixel coordinates

    cv::Mat    referenceImage;                // in-memory only; not in JSON

    bool hasRoi() const { return roi.width > 0.0f && roi.height > 0.0f; }
    bool hasConditionRoi() const
    {
        return conditionRoi.width > 0.0f && conditionRoi.height > 0.0f;
    }
};

// Container of per-camera workspaces, keyed by camera device id.
class CameraWorkspaceMap {
public:
    bool isEmpty() const { return m_workspaces.isEmpty(); }
    bool contains(const QString &cameraId) const { return m_workspaces.contains(cameraId); }
    QList<QString> cameraIds() const { return m_workspaces.keys(); }

    CameraWorkspace workspace(const QString &cameraId) const
    {
        return m_workspaces.value(cameraId);
    }

    void setWorkspace(const QString &cameraId, const CameraWorkspace &ws)
    {
        if (cameraId.isEmpty()) return;
        m_workspaces.insert(cameraId, ws);
    }

    void remove(const QString &cameraId) { m_workspaces.remove(cameraId); }

    // ── JSON — ROI + flag only; the reference image travels as a BLOB ──────
    QJsonArray toJson() const
    {
        QJsonArray arr;
        for (auto it = m_workspaces.cbegin(); it != m_workspaces.cend(); ++it) {
            const CameraWorkspace &ws = it.value();
            QJsonObject obj;
            obj["cameraId"]     = it.key();
            obj["useWorkspace"] = ws.useWorkspace;
            obj["roi"] = QJsonObject{{ "x", ws.roi.x },     { "y", ws.roi.y },
                                     { "w", ws.roi.width },  { "h", ws.roi.height }};
            obj["useConditionWorkspace"] = ws.useConditionWorkspace;
            obj["conditionRoi"] = QJsonObject{
                { "x", ws.conditionRoi.x },     { "y", ws.conditionRoi.y },
                { "w", ws.conditionRoi.width }, { "h", ws.conditionRoi.height }};
            arr.append(obj);
        }
        return arr;
    }

    bool fromJson(const QJsonValue &value)
    {
        m_workspaces.clear();
        if (value.isUndefined() || value.isNull())
            return true;
        if (!value.isArray())
            return false;

        for (const QJsonValue &item : value.toArray()) {
            const QJsonObject obj = item.toObject();
            const QString cameraId = obj["cameraId"].toString();
            if (cameraId.isEmpty())
                continue;

            CameraWorkspace ws;
            ws.useWorkspace = obj["useWorkspace"].toBool(false);
            const QJsonObject r = obj["roi"].toObject();
            ws.roi = cv::Rect2f(static_cast<float>(r["x"].toDouble(0.0)),
                                static_cast<float>(r["y"].toDouble(0.0)),
                                static_cast<float>(r["w"].toDouble(0.0)),
                                static_cast<float>(r["h"].toDouble(0.0)));

            ws.useConditionWorkspace = obj["useConditionWorkspace"].toBool(false);
            const QJsonObject cr = obj["conditionRoi"].toObject();
            ws.conditionRoi = cv::Rect2f(static_cast<float>(cr["x"].toDouble(0.0)),
                                         static_cast<float>(cr["y"].toDouble(0.0)),
                                         static_cast<float>(cr["w"].toDouble(0.0)),
                                         static_cast<float>(cr["h"].toDouble(0.0)));
            m_workspaces.insert(cameraId, ws);
        }
        return true;
    }

    // ── Reference-image BLOB helpers (key form: "ws_{cameraId}") ──────────
    static QString imageKey(const QString &cameraId)
    {
        return QStringLiteral("ws_") + cameraId;
    }

    static bool parseImageKey(const QString &key, QString &cameraId)
    {
        if (!key.startsWith(QStringLiteral("ws_")))
            return false;
        cameraId = key.mid(3);
        return !cameraId.isEmpty();
    }

    QMap<QString, cv::Mat> getImageMap() const
    {
        QMap<QString, cv::Mat> map;
        for (auto it = m_workspaces.cbegin(); it != m_workspaces.cend(); ++it) {
            if (!it.value().referenceImage.empty())
                map.insert(imageKey(it.key()), it.value().referenceImage);
        }
        return map;
    }

    void setReferenceImage(const QString &cameraId, const cv::Mat &image)
    {
        if (cameraId.isEmpty()) return;
        m_workspaces[cameraId].referenceImage = image;   // creates entry if absent
    }

private:
    QMap<QString, CameraWorkspace> m_workspaces;  // cameraId -> workspace
};

} // namespace vc::model

Q_DECLARE_METATYPE(vc::model::CameraWorkspace)

#endif // CAMERA_WORKSPACE_H
