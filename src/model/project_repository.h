#ifndef PROJECT_REPOSITORY_H
#define PROJECT_REPOSITORY_H

#include "model/project.h"
#include <QString>

#define SOFTWARE_VERSION    "0.01"

namespace vc::model {

class ProjectRepository {
public:
    static bool createNew(const QString& path,
                          const QString& projectName,
                          const QString& str_time);
    static bool load(const QString& path, Project& out);
    static bool save(const QString& path, const Project& project);
    static QString lastMsg();

private:
    static bool createSchema(const QString& connName);
    static bool saveImages(const QString& connName,
                           const QVector<ITask*>& tasks);
    static bool loadImages(const QString& connName,
                           QVector<ITask*>& tasks);
    static QString m_lastMsg;
};

} // namespace vc::model

#endif // PROJECT_REPOSITORY_H
