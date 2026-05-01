#ifndef PROJECT_TREE_WIDGET_H
#define PROJECT_TREE_WIDGET_H

#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>
#include <QMap>
#include <QString>

#include "device/device_manager.h"
#include "model/itask.h"
#include "model/project.h"

class ProjectTreeWidget : public QWidget {
    Q_OBJECT

public:
    explicit ProjectTreeWidget(QWidget *parent = nullptr);

    void clearManager();
    void setProject(std::shared_ptr<vc::model::Project> proj);

    void addDevice(vc::device::IDevice *device);
    void addTask(vc::model::ITask *task);
    void assignDeviceToTask(const QString &deviceId, const QString &taskId);

    void changeProjectName(QString name);
    void enableProjectTree(bool ena);

public slots:
    void refreshDeviceBranch();
    void refreshTaskBranch();
    bool showInputTaskNameDialog(QString &name);

private slots:
    void onTreeItemDoubleClicked(const QModelIndex &index);
    void showContextMenu(const QPoint &pos);
    void createNewDeviceAction();
    void deleteDeviceAction(QStandardItem * clicked_item, QString dv_id);
    void createNewTaskAction_Localization();
    void createNewTaskAction_PickandPlace();
    void deleteTaskAction(QStandardItem * clicked_item, QString task_id);
    void onDeviceNameChanged(QString id);
    void onTaskNameChanged(QString id);

signals:
    void deviceDoubleClicked(QString id);
    void taskDoubleClicked(QString task_id, QString wg_name);

private:
    QTreeView *treeView;
    QStandardItemModel *model;

    std::shared_ptr<vc::model::Project> projectPtr;
    std::shared_ptr<vc::device::DeviceManager> dv_manager;

    QStandardItem *rootItem{nullptr};
    QStandardItem *devicesBranch{nullptr};
    QStandardItem *tasksBranch{nullptr};

    QMap<QString, QStandardItem*> deviceRegistry;
    QMap<QString, QStandardItem*> taskRegistry;

    bool access_block{false};
};

#endif // PROJECT_TREE_WIDGET_H
