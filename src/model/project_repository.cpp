#include "project_repository.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QDateTime>

namespace vc::model {

QString ProjectRepository::m_lastMsg;

// ─── Schema ──────────────────────────────────────────────────

static const char* kSchema = R"(
CREATE TABLE IF NOT EXISTS project_info (
    key   TEXT PRIMARY KEY,
    value TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS project_data (
    id   INTEGER PRIMARY KEY CHECK (id = 1),
    json TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS project_images (
    task_id      TEXT    NOT NULL,
    image_name   TEXT    NOT NULL,
    image_data   BLOB    NOT NULL,
    PRIMARY KEY (task_id, image_name)
);
)";

// ─── Helpers ─────────────────────────────────────────────────

static bool openDB(const QString& path, const QString& connName) {
    auto db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(path);
    if (!db.open()) {
        return false;
    }
    // WAL mode: safety function for read/write at the same time
    QSqlQuery(db).exec("PRAGMA journal_mode=WAL");
    QSqlQuery(db).exec("PRAGMA foreign_keys=ON");
    return true;
}

static void closeDB(const QString& connName) {
    QSqlDatabase::database(connName).close();
    QSqlDatabase::removeDatabase(connName);
}


// ─── Image helpers ───────────────────────────────────────────

static QByteArray serializeImage(const cv::Mat& image) {
    if (image.empty()) {
        return QByteArray();
    }

    std::vector<uchar> buffer;
    std::vector<int> params = {cv::IMWRITE_PNG_COMPRESSION, 3};
    cv::imencode(".bmp", image, buffer, params);

    // convert vector std::uchar to QByteArray
    return QByteArray(reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size()));
}

static cv::Mat deserializeImage(const QByteArray& byteArray) {
    if (byteArray.isEmpty()) {
        return cv::Mat();
    }

    // convert from QByteArray to vector
    std::vector<uchar> buffer(byteArray.constData(), byteArray.constData() + byteArray.size());
    return cv::imdecode(buffer, cv::IMREAD_UNCHANGED);
}


// ─── Schema creation ─────────────────────────────────────────

bool ProjectRepository::createSchema(const QString& connName) {
    auto db = QSqlDatabase::database(connName);
    for (const QString& stmt :
         QString(kSchema).split(';', Qt::SkipEmptyParts)) {
        const QString s = stmt.trimmed();
        if (s.isEmpty()) continue;
        QSqlQuery q(db);
        if (!q.exec(s)) {
            m_lastMsg = q.lastError().text();
            return false;
        }
    }
    return true;
}

// ─── Public API ──────────────────────────────────────────────

bool ProjectRepository::createNew(const QString& path,
                                  const QString& projectName,
                                  const QString& str_time) {
    const QString conn = "proj_new_" + path;
    if (!openDB(path, conn)) {
        m_lastMsg = QSqlDatabase::database(conn).lastError().text();
        closeDB(conn);
        return false;
    }

    bool ok = createSchema(conn);
    if (ok) {
        auto db = QSqlDatabase::database(conn);
        QSqlQuery q(db);

        const QMap<QString,QString> info {
            { "name",           projectName         },
            { "version",        SOFTWARE_VERSION    },
            { "created_at",     str_time            },
            { "updated_at",     str_time            },
            { "description",    ""                  },
            { "schema_version", "1.0"               },
        };

        q.prepare("INSERT OR REPLACE INTO project_info "
                  "VALUES (?, ?)");
        for (auto it = info.begin(); it != info.end(); ++it) {
            q.addBindValue(it.key());
            q.addBindValue(it.value());
            ok = q.exec() && ok;
        }
    }

    closeDB(conn);
    return ok;
}

bool ProjectRepository::save(const QString& path,
                             const Project& project) {
    const QString conn = "proj_save_" + path;
    bool ok = true;

    if (!openDB(path, conn)) {
        m_lastMsg = QSqlDatabase::database(conn).lastError().text();
        closeDB(conn);
        return false;
    }

    {
        auto db = QSqlDatabase::database(conn);
        db.transaction();

        // 1. Upsert project_info
        QSqlQuery q(db);
        q.prepare("INSERT OR REPLACE INTO project_info VALUES (?,?)");
        for (auto& [k, v] : std::initializer_list<std::pair<QString,QString>>{
                                                                               { "name",          project.name()              },
                                                                               { "version",       project.version()           },
                                                                               { "updated_at",    project.updatedAt()         },
                                                                               { "description",   project.description()       },
                                                                               }) {

            q.addBindValue(k);
            q.addBindValue(v);
            ok = q.exec() && ok;
        }

        // 2. Serialize Project -> JSON
        const QByteArray json =
            QJsonDocument(project.toJson()).toJson(QJsonDocument::Compact);

        qDebug() << "Project respository:" << json;

        q.prepare("INSERT OR REPLACE INTO project_data (id, json) "
                  "VALUES (1, ?)");
        q.addBindValue(QString::fromUtf8(json));
        ok = q.exec() && ok;

        // 3. Save image BLOBs
        QVector<ITask*> tasks;
        const QMap<QString, std::shared_ptr<vc::model::ITask>>& proj_tasks = project.getCurrentTasks();
        auto task_it = proj_tasks.cbegin();
        while (task_it != proj_tasks.cend()) {
            tasks.append(task_it.value().get());
            task_it++;
        }

        ok = ok && saveImages(conn, tasks);

        if (ok) db.commit();
        else {
            m_lastMsg = db.lastError().text();
            db.rollback();
        }
    }

    closeDB(conn);
    return ok;
}

bool ProjectRepository::load(const QString& path, Project& out) {
    const QString conn = "vc_load_" + path;
    bool ok = true;
    bool is_empty = false;
    QMap<QString,QString> info;
    QByteArray json;

    if (!openDB(path, conn)) {
        m_lastMsg = QSqlDatabase::database(conn).lastError().text();
        closeDB(conn);
        return false;
    }

    {
        auto db = QSqlDatabase::database(conn);

        // 1. Load project_info
        QSqlQuery q(db);
        q.exec("SELECT key, value FROM project_info");

        while (q.next())
            info[q.value(0).toString()] = q.value(1).toString();
        // 2. Parse JSON -> Project struct
        q.exec("SELECT json FROM project_data WHERE id = 1");
        if (!q.next()) {
            m_lastMsg = "project_data is empty";
            is_empty = true;
        }

        if (!is_empty) {
            json = q.value(0).toString().toUtf8();
        }

        const QJsonObject root =
            QJsonDocument::fromJson(json).object();
        out.fromJson(root);
        out.setName(info.value("name"));
        out.setDescription(info.value("description"));
        out.setCreatedAt(info.value("created_at"));
        out.setUpdatedAt(info.value("updated_at"));

        // 3. Inject image BLOBs to task
        QVector<ITask*> tasks;
        const QMap<QString, std::shared_ptr<vc::model::ITask>>& proj_tasks = out.getCurrentTasks();
        auto task_it = proj_tasks.cbegin();
        while (task_it != proj_tasks.cend()) {
            tasks.append(task_it.value().get());
            task_it++;
        }
        ok = loadImages(conn, tasks);
        if (!ok) m_lastMsg = db.lastError().text();
    }

    if (is_empty) {
        closeDB(conn);
        return false;
    }

    closeDB(conn);
    return ok;
}

QString ProjectRepository::lastMsg() {
    return m_lastMsg;
}

bool ProjectRepository::saveImages(const QString& connName,
                                   const QVector<ITask*>& tasks) {
    auto db = QSqlDatabase::database(connName);
    QSqlQuery q(db);
    q.exec("DELETE FROM project_images");

    q.prepare("INSERT INTO project_images "
              "(task_id, image_name, image_data) "
              "VALUES (?, ?, ?)");

    for (const auto& task : tasks) {
        QMap<QString, cv::Mat> image_map = task->getTaskImageMap();

        auto map_it = image_map.cbegin();
        while (map_it != image_map.cend()) {
            q.addBindValue(task->id());
            q.addBindValue(map_it.key());
            q.addBindValue(serializeImage(map_it.value()));
            if (!q.exec()) {
                m_lastMsg = q.lastError().text();
                return false;
            }
            map_it++;
        }
    }
    return true;
}

bool ProjectRepository::loadImages(const QString& connName,
                                   QVector<ITask*>& tasks) {
    auto db = QSqlDatabase::database(connName);
    QSqlQuery q(db);
    q.prepare("SELECT image_name, image_data "
              "FROM project_images WHERE task_id = ?");

    for (auto& task : tasks) {
        q.addBindValue(task->id());
        if (!q.exec()) {
            m_lastMsg = q.lastError().text();
            return false;
        }

        QMap<QString, cv::Mat> iamge_map;
        while (q.next()) {
            const QString image_name = q.value(0).toString();
            const auto blob  = q.value(1).toByteArray();
            cv::Mat image = deserializeImage(blob);
            iamge_map.insert(image_name, image);
        }

        task->loadTaskImageMap(iamge_map);
    }

    return true;
}

} // namespace vc::model
