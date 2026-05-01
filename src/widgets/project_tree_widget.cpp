#include "project_tree_widget.h"
#include "qlineedit.h"
#include "qmenu.h"
#include <QVBoxLayout>
#include <QApplication>
#include <QStyle>
#include <QDebug>
#include <QMessageBox>

#include "form/add_device_wizard.h"
// #include "device/device_factory.h"
#include "model/task_localization.h"
#include "logger/app_logger.h"
#include "windows_helper.h"
#include "model/task_factory.h"

ProjectTreeWidget::ProjectTreeWidget(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);

    treeView = new QTreeView(this);
    layout->addWidget(treeView);

    model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({"Project Explorer"});
    treeView->setModel(model);

    // setup Root
    QIcon rootIcon = style()->standardIcon(QStyle::SP_DirHomeIcon);
    rootItem = new QStandardItem(rootIcon, tr("Vision Project"));
    rootItem->setEditable(false);
    model->appendRow(rootItem);

    // Setup branches Devices and Tasks
    QIcon devicesIcon = svgIcon(":/resrc/icon/plc_devices.svg");
    devicesBranch = new QStandardItem(devicesIcon, tr("Devices"));
    devicesBranch->setEditable(false);

    QIcon tasksIcon = style()->standardIcon(QStyle::SP_FileDialogListView);
    tasksBranch = new QStandardItem(tasksIcon, tr("Tasks"));
    tasksBranch->setEditable(false);

    rootItem->appendRow(devicesBranch);
    rootItem->appendRow(tasksBranch);
    treeView->expandAll();

    treeView->setExpandsOnDoubleClick(false);

    connect(treeView, &QTreeView::doubleClicked,
            this, &ProjectTreeWidget::onTreeItemDoubleClicked);

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView, &QTreeView::customContextMenuRequested,
            this, &ProjectTreeWidget::showContextMenu);

    access_block = true;
    dv_manager.reset();
    this->setEnabled(false);
}

void ProjectTreeWidget::clearManager() {
    access_block = true;
    dv_manager.reset();
    this->setEnabled(false);
}

void ProjectTreeWidget::setProject(std::shared_ptr<vc::model::Project> proj) {
    if (proj == nullptr) {
        projectPtr.reset();
        dv_manager.reset();
        this->setEnabled(false);
        LOG_DEV_ERR << "Device manager is null";
        return;
    }

    projectPtr = proj;
    dv_manager = projectPtr->deviceManager();
    access_block = false;
    this->setEnabled(true);
}

void ProjectTreeWidget::addDevice(vc::device::IDevice *device) {
    QIcon devIcon = style()->standardIcon(QStyle::SP_MessageBoxWarning);
    switch (device->deviceType()) {
    case vc::device::DeviceType::Camera:
        // devIcon = style()->standardIcon(QStyle::SP_DesktopIcon);
        devIcon = svgIcon(":/resrc/icon/camera.svg");
        break;
    case vc::device::DeviceType::McDevice:
        devIcon = style()->standardIcon(QStyle::SP_ComputerIcon);
        break;
    default:
        break;
    }

    auto *devItem = new QStandardItem(devIcon, QString("%1").arg(device->name()));
    devItem->setEditable(false);

    // Gán ID thiết bị vào UserRole để liên kết với các module traditional CV bên dưới
    devItem->setData(device->id(), Qt::UserRole);

    devicesBranch->appendRow(devItem);
    deviceRegistry.insert(device->id(), devItem);

    connect(device, &vc::device::IDevice::nameChanged, this, [this, device]() {
        this->onDeviceNameChanged(device->id());
    });

    treeView->expand(devicesBranch->index());
}

void ProjectTreeWidget::addTask(vc::model::ITask *task) {
    QIcon devIcon = style()->standardIcon(QStyle::SP_MessageBoxWarning);
    switch (task->taskType()) {
    case vc::model::TaskType::LocalizationTask:
    {
        QStandardItem *taskItem = new QStandardItem(devIcon, QString("%1").arg(task->name()));
        taskItem->setEditable(false);
        taskItem->setData(task->id(), Qt::UserRole);

        // LOG_DEV_DEBUG << "Added task:"
        //               << task->name() << " - "
        //               << taskItem->data(Qt::UserRole).toString();

        QStandardItem *item_dashboard = new QStandardItem(devIcon, QString("Dashboard"));
        QStandardItem *item_pattern = new QStandardItem(devIcon, QString("Patterns"));
        QStandardItem *item_calibration = new QStandardItem(devIcon, QString("Calibration"));

        item_dashboard->setEditable(false);
        item_pattern->setEditable(false);
        item_calibration->setEditable(false);

        taskItem->setRowCount(3);
        taskItem->setChild(0, item_dashboard);
        taskItem->setChild(1, item_pattern);
        taskItem->setChild(2, item_calibration);

        tasksBranch->appendRow(taskItem);
        taskRegistry.insert(task->id(), taskItem);
        treeView->expand(tasksBranch->index());
    }
        break;
    default:
        return;
    }

    connect(task, &vc::model::ITask::nameChanged, this, [this, task]() {
        this->onTaskNameChanged(task->id());
    });
}

void ProjectTreeWidget::assignDeviceToTask(const QString &deviceId, const QString &taskId) {
    if (!deviceRegistry.contains(deviceId) || !taskRegistry.contains(taskId)) {
        qWarning() << "Error: Device ID or Task ID not exists!";
        return;
    }

    QStandardItem *originalDev = deviceRegistry.value(deviceId);
    QStandardItem *taskItem = taskRegistry.value(taskId);

    // create coppy of item, include icon and text
    auto *clonedDevItem = new QStandardItem(originalDev->icon(), originalDev->text());
    clonedDevItem->setEditable(false);

    // QUAN TRỌNG: Gán cùng UserRole ID để định danh đây là cùng một phần cứng
    clonedDevItem->setData(deviceId, Qt::UserRole);

    taskItem->appendRow(clonedDevItem);
    treeView->expand(taskItem->index());
}

void ProjectTreeWidget::changeProjectName(QString name) {
    if (rootItem == nullptr) {
        return;
    }

    rootItem->setText(name);
}

void ProjectTreeWidget::enableProjectTree(bool ena) {
    access_block = !ena;
}

void ProjectTreeWidget::refreshDeviceBranch() {
    // remove all device items
    if (devicesBranch->hasChildren()) {
        devicesBranch->removeRows(0, devicesBranch->rowCount());
    }

    if (!dv_manager) {
        return;
    }

    const QMap<QString, std::shared_ptr<vc::device::IDevice>>& instances = dv_manager->getCurrentDevices();
    QMap<QString, std::shared_ptr<vc::device::IDevice>>::const_iterator it_start = instances.cbegin();
    QMap<QString, std::shared_ptr<vc::device::IDevice>>::const_iterator it_end = instances.cend();
    while (it_start != it_end) {
        std::shared_ptr<vc::device::IDevice> dv = it_start.value();
        // addDevice(dv->id(), dv->name(), dv->deviceType());
        addDevice(dv.get());
        it_start++;
    }
}

void ProjectTreeWidget::refreshTaskBranch() {
    if (tasksBranch->hasChildren()) {
        tasksBranch->removeRows(0, tasksBranch->rowCount());
    }

    if (!projectPtr) {
        return;
    }

    const QMap<QString, std::shared_ptr<vc::model::ITask>>& instances = projectPtr->getCurrentTasks();
    QMap<QString, std::shared_ptr<vc::model::ITask>>::const_iterator it_start = instances.cbegin();
    QMap<QString, std::shared_ptr<vc::model::ITask>>::const_iterator it_end = instances.cend();
    while (it_start != it_end) {
        std::shared_ptr<vc::model::ITask> dv = it_start.value();
        // addDevice(dv->id(), dv->name(), dv->deviceType());
        addTask(dv.get());
        it_start++;
    }
}

bool ProjectTreeWidget::showInputTaskNameDialog(QString &name) {
    bool ok = false;
    while (true) {
        name = QInputDialog::getText(this, tr("Create new task"),
                                     tr("Task name:"), QLineEdit::Normal,
                                     "", &ok);

        if (!ok) return false;

        name = name.trimmed();
        if (name.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("Task name can be empty!"));
            continue;
        }

        if (projectPtr->isTaskNameOccupied(name)) {
            QMessageBox::warning(this, tr("Error"), tr("Task name duplicated!"));
            continue;
        }

        return true;
    }
    return ok;
}

void ProjectTreeWidget::onTreeItemDoubleClicked(const QModelIndex &index) {
    if (access_block) {
        return;
    }

    if (!index.isValid()) {
        return;
    }

    QStandardItem *clickedItem = model->itemFromIndex(index);
    if (!clickedItem) {
        return;
    }

    // clicked at device item
    if (clickedItem->parent() == devicesBranch) {
        QString deviceId = clickedItem->data(Qt::UserRole).toString();
        LOG_DEV_DEBUG << "Clicked at devices item" << deviceId;
        emit deviceDoubleClicked(deviceId);

    // clicked at task item
    } else if (clickedItem->parent() == tasksBranch) {
        QString taskId = clickedItem->data(Qt::UserRole).toString();
        LOG_DEV_DEBUG << "Clicked at task item" << taskId;
        emit taskDoubleClicked(taskId, "");

    // clicked at sub item
    } else if ((clickedItem->parent() != nullptr) &&
               (clickedItem->parent()->parent() == tasksBranch)) {
        QStandardItem *taskItem = clickedItem->parent();
        QString taskId = taskItem->data(Qt::UserRole).toString();
        QString subId = clickedItem->data(Qt::UserRole).toString();
        LOG_DEV_DEBUG << "Clicked at task's child item" << taskId << clickedItem;
        emit taskDoubleClicked(taskId, clickedItem->text());
    }
}


void ProjectTreeWidget::showContextMenu(const QPoint &pos) {
    if (access_block) {
        return;
    }

    QModelIndex index = treeView->indexAt(pos);
    if (!index.isValid()) {
        // ignore clicked on empty space
        return;
    }

    QStandardItem *clickedItem = model->itemFromIndex(index);

    // create Menu
    QMenu contextMenu(this);
    if (clickedItem == devicesBranch) {
        QAction *addDeviceAction = contextMenu.addAction(
            style()->standardIcon(QStyle::SP_ComputerIcon),
            tr("Add new device..."));

        connect(addDeviceAction, &QAction::triggered, this, [this]() {
            this->createNewDeviceAction();
        });

    } else if (clickedItem == tasksBranch) {
        QAction *addTaskLocalizationAction = contextMenu.addAction(
            style()->standardIcon(QStyle::SP_FileIcon),
            tr("Add new Localization task..."));

        connect(addTaskLocalizationAction, &QAction::triggered, this, [this]() {
            this->createNewTaskAction_Localization();
        });

        QAction *addTaskPickandPlaceAction = contextMenu.addAction(
            style()->standardIcon(QStyle::SP_FileIcon),
            tr("Add new Pick and place task..."));

        connect(addTaskPickandPlaceAction, &QAction::triggered, this, [this]() {
            this->createNewTaskAction_PickandPlace();
        });

    } else if (clickedItem->parent() == devicesBranch) {
        QAction *deleteAction = contextMenu.addAction(
            style()->standardIcon(QStyle::SP_TrashIcon),
            tr("Remove device"));

        // get device's ID from UserRole
        QString deviceId = clickedItem->data(Qt::UserRole).toString();
        connect(deleteAction, &QAction::triggered, this, [this, clickedItem, deviceId]() {
            this->deleteDeviceAction(clickedItem, deviceId);
        });

    } else if (clickedItem->parent() == tasksBranch) {
        QAction *deleteAction = contextMenu.addAction(
            style()->standardIcon(QStyle::SP_TrashIcon),
            tr("Remove task"));

        // get task's ID from UserRole
        QString taskId = clickedItem->data(Qt::UserRole).toString();
        connect(deleteAction, &QAction::triggered, this, [this, clickedItem, taskId]() {
            this->deleteTaskAction(clickedItem, taskId);
        });
    }

    // show menu at clicked position, global position
    if (!contextMenu.isEmpty()) {
        contextMenu.exec(treeView->viewport()->mapToGlobal(pos));
    }
}

void ProjectTreeWidget::createNewDeviceAction() {
    if (!dv_manager) {
        LOG_DEV_ERR << "Device manager is null.";
        return;
    }

    AddDeviceWizard wizard(dv_manager, this);
    if (wizard.showWizard() == QDialog::Accepted) {

        QString newId = wizard.getDeviceId();
        QString newName = wizard.getDeviceName();
        QString devType = wizard.getDeviceType();

        refreshDeviceBranch();
        LOG_USER_INFO << QString("New device added: %1 - ID %2").arg(newName, newId);
    }
}

void ProjectTreeWidget::deleteDeviceAction(QStandardItem * clicked_item, QString dv_id) {
    if (!dv_manager) {
        LOG_DEV_ERR << "Device manager is null.";
        return;
    }

    // deleted confirmation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Delete device confirm"),
        QString("Would you like to delete device '%1'?\nThis operate could not undo.")
            .arg(clicked_item->text()),
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        this->devicesBranch->removeRow(clicked_item->row());
        this->deviceRegistry.remove(dv_id);
        dv_manager->releaseDevice(dv_id);
        LOG_USER_INFO << QString("Device factory deleted device: %1").arg(dv_id);
    }
}

void ProjectTreeWidget::createNewTaskAction_Localization() {
    QString task_name;
    if (!showInputTaskNameDialog(task_name)) {
        return;
    }

    vc::model::TaskLocalization *task = new vc::model::TaskLocalization(task_name);
    projectPtr->addTask(task);

    refreshTaskBranch();
    LOG_USER_INFO << QString("New task added: %1 - ID %2").arg(task_name, task->id());
}

void ProjectTreeWidget::createNewTaskAction_PickandPlace() {

}

void ProjectTreeWidget::deleteTaskAction(QStandardItem * clicked_item, QString task_id) {
    if (!projectPtr) {
        LOG_DEV_ERR << "Project is null.";
        return;
    }

    QString task_name = clicked_item->text();

    // deleted confirmation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Delete task confirm"),
        QString("Would you like to delete task '%1'?\nThis operate could not undo.")
            .arg(clicked_item->text()),
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        this->tasksBranch->removeRow(clicked_item->row());
        this->taskRegistry.remove(task_id);
        projectPtr->removeTask(task_id);
        LOG_USER_INFO << QString("Device factory deleted device: %1 - %2").arg(task_name, task_id);
    }
}

void ProjectTreeWidget::onDeviceNameChanged(QString id) {
    std::shared_ptr<vc::device::IDevice> device = dv_manager->deviceById(id);
    if (!device) {
        return;
    }

    if (!deviceRegistry.contains(id)) {
        return;
    }

    QStandardItem* dev_item = deviceRegistry[id];
    dev_item->setText(QString("%1").arg(device->name()));
}

void ProjectTreeWidget::onTaskNameChanged(QString id) {
    std::shared_ptr<vc::model::ITask> task = projectPtr->taskById(id);
    if (!task) {
        return;
    }

    if (!taskRegistry.contains(id)) {
        return;
    }

    QStandardItem* task_item = taskRegistry[id];
    task_item->setText(QString("%1").arg(task->name()));
}

