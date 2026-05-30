#include "model/localization_signal_mapper.h"

#include <QMetaProperty>

#include "logger/app_logger.h"

namespace vc::model {

namespace {

constexpr const char *kSignalFields[] = {
    "nActiveCamera",
    "nActivePattern",
    "nDetectedNumber",
    "bCameraValid",
    "bPatternValid",
    "bTaskReady",
    "bExecuteTrigger",
    "bMatchingFinished",
    "bMatchingBusy",
    "bMatchingDetected",
    "bMatchingLowArea",
};

QString readSignalTag(const TaskLocalizeConfig &config, const char *fieldName)
{
    const QMetaObject &meta = TaskLocalizeConfig::staticMetaObject;
    const int idx = meta.indexOfProperty(fieldName);
    if (idx < 0)
        return QString();

    return meta.property(idx).readOnGadget(&config).toString();
}

} // namespace

void LocalizationSignalMapper::configure(const TaskLocalizeConfig &config)
{
    m_tagToSignalName.clear();

    for (const char *fieldName : kSignalFields) {
        const QString tag = readSignalTag(config, fieldName).trimmed();
        if (tag.isEmpty())
            continue;

        if (m_tagToSignalName.contains(tag)) {
            LOG_DEV_ERR << "LocalizationSignalMapper: duplicate PLC tag"
                        << tag
                        << "existing signal"
                        << m_tagToSignalName.value(tag)
                        << "new signal"
                        << fieldName;
        }

        m_tagToSignalName.insert(tag, QString::fromUtf8(fieldName));
    }
}

void LocalizationSignalMapper::clear()
{
    m_tagToSignalName.clear();
}

QString LocalizationSignalMapper::signalNameForTag(const QString &tag) const
{
    return m_tagToSignalName.value(tag);
}

QList<LocalizationSignalEvent> LocalizationSignalMapper::mapValues(
    const QMap<QString, QVariant> &values) const
{
    QList<LocalizationSignalEvent> events;
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        const QString signalName = signalNameForTag(it.key());
        if (signalName.isEmpty()) {
            LOG_DEV_ERR << "LocalizationSignalMapper: unmapped PLC tag" << it.key();
            continue;
        }

        events.append({it.key(), signalName, it.value()});
    }
    return events;
}

} // namespace vc::model
