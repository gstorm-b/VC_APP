#ifndef LOCALIZATION_SIGNAL_MAPPER_H
#define LOCALIZATION_SIGNAL_MAPPER_H

#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>

#include "model/task_localization_config.h"

namespace vc::model {

struct LocalizationSignalEvent {
    QString tag;
    QString name;
    QVariant value;
};

class LocalizationSignalMapper {
public:
    void configure(const TaskLocalizeConfig &config);
    void clear();

    QString signalNameForTag(const QString &tag) const;
    QList<LocalizationSignalEvent> mapValues(const QMap<QString, QVariant> &values) const;
    bool isEmpty() const { return m_tagToSignalName.isEmpty(); }

private:
    QMap<QString, QString> m_tagToSignalName;
};

} // namespace vc::model

#endif // LOCALIZATION_SIGNAL_MAPPER_H
