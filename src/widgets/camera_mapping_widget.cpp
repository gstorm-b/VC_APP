#include "camera_mapping_widget.h"
#include <QMouseEvent>
#include <QSet>
#include <algorithm>

// ==========================================
// EditableComboWidget Implementation
// ==========================================
EditableComboWidget::EditableComboWidget(QWidget *parent) : QStackedWidget(parent) {
    m_label = new QLabel(this);
    m_label->setStyleSheet("QLabel { padding: 4px; border: 1px solid transparent; } "
                           "QLabel:hover { border: 1px solid #aaa; background-color: #f0f0f0; }");
    m_label->installEventFilter(this);

    m_comboBox = new QComboBox(this);

    addWidget(m_label);
    addWidget(m_comboBox);
    setCurrentWidget(m_label);

    connect(m_comboBox, &QComboBox::activated, this, [this](int index) {
        if (index >= 0) {
            setText(m_comboBox->itemText(index));
            emit valueChanged(m_label->text());
        }
        setCurrentWidget(m_label);
    });

    m_comboBox->installEventFilter(this);
}

bool EditableComboWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_label && event->type() == QEvent::MouseButtonPress) {
        emit editRequested();
        setCurrentWidget(m_comboBox);
        m_comboBox->showPopup();
        return true;
    }
    if (watched == m_comboBox && event->type() == QEvent::FocusIn) {
        setCurrentWidget(m_label);
    }
    return QStackedWidget::eventFilter(watched, event);
}

void EditableComboWidget::setText(const QString &text) {
    m_label->setText(text);
}

QString EditableComboWidget::text() const {
    return m_label->text();
}

void EditableComboWidget::setOptions(const QStringList &options, const QString &currentSelection) {
    m_comboBox->blockSignals(true);
    m_comboBox->clear();
    m_comboBox->addItems(options);
    int idx = m_comboBox->findText(currentSelection);
    if (idx >= 0) m_comboBox->setCurrentIndex(idx);
    m_comboBox->blockSignals(false);
}

// ==========================================
// CameraRowWidget Implementation
// ==========================================
CameraRowWidget::CameraRowWidget(QWidget *parent) : QWidget(parent) {
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    nameWidget = new EditableComboWidget(this);
    numberWidget = new EditableComboWidget(this);
    btnDelete = new QPushButton("Delete", this);

    layout->addWidget(new QLabel("Camera:", this));
    layout->addWidget(nameWidget, 1);
    layout->addWidget(new QLabel("Number:", this));
    layout->addWidget(numberWidget, 1);
    layout->addWidget(btnDelete);
}

// ==========================================
// AddRowWidget Implementation
// ==========================================
AddRowWidget::AddRowWidget(QWidget *parent) : QStackedWidget(parent) {
    addWidget(createAddButtonPage());
    addWidget(createSelectCameraPage());
    setCurrentIndex(0);
}

QWidget* AddRowWidget::createAddButtonPage() {
    QWidget *w = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(w);
    layout->setContentsMargins(0, 0, 0, 0);
    btnAddRow = new QPushButton("+ Add New Row", this);
    layout->addWidget(btnAddRow);

    connect(btnAddRow, &QPushButton::clicked, this, [this]() {
        setCurrentIndex(1);
    });
    return w;
}

QWidget* AddRowWidget::createSelectCameraPage() {
    QWidget *w = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(w);
    layout->setContentsMargins(0, 0, 0, 0);

    cbCameraSelect = new QComboBox(this);
    btnCancel = new QPushButton("Cancel", this);

    layout->addWidget(cbCameraSelect, 1);
    layout->addWidget(btnCancel);

    connect(btnCancel, &QPushButton::clicked, this, [this]() {
        setCurrentIndex(0);
    });

    connect(cbCameraSelect, &QComboBox::activated, this, [this](int index) {
        // Index 0 luôn là "-- Select Camera to Add --"
        if(index > 0) {
            emit addRequested(cbCameraSelect->currentText());
            setCurrentIndex(0);
        }
    });

    return w;
}

void AddRowWidget::setAvailableCameras(const QStringList &cameras) {
    cbCameraSelect->blockSignals(true);
    cbCameraSelect->clear();
    cbCameraSelect->addItem("-- Select Camera to Add --");
    cbCameraSelect->addItems(cameras);
    cbCameraSelect->setCurrentIndex(0);
    cbCameraSelect->blockSignals(false);
}

// ==========================================
// CameraMappingWidget Implementation
// ==========================================
CameraMappingWidget::CameraMappingWidget(QWidget *parent) : QListWidget(parent) {
    setSelectionMode(QAbstractItemView::NoSelection);
    setupAddRow();
}

void CameraMappingWidget::setCameraList(const QStringList &cameras) {
    m_allCameras = cameras;
    onDataChanged();
}

void CameraMappingWidget::setNumberLimit(int limit) {
    m_limitNumber = limit > 0 ? limit : 8;
}

void CameraMappingWidget::setupAddRow() {
    m_addRowItem = new SortableCameraItem(this);
    // Gán giá trị cực lớn để khi sort, item này luôn nằm ở cuối
    m_addRowItem->setData(Qt::UserRole, INT_MAX);

    m_addRowWidget = new AddRowWidget(this);
    m_addRowItem->setSizeHint(QSize(0, 40));
    setItemWidget(m_addRowItem, m_addRowWidget);

    connect(m_addRowWidget, &AddRowWidget::addRequested, this, &CameraMappingWidget::onRowAdded);
}

void CameraMappingWidget::addMappingRow(const QString &cameraName, int number) {
    SortableCameraItem *newItem = new SortableCameraItem();
    newItem->setSizeHint(QSize(0, 40));

    CameraRowWidget *rowWidget = new CameraRowWidget(this);
    rowWidget->nameWidget->setText(cameraName);
    rowWidget->numberWidget->setText(QString::number(number));

    // Connect các signals cho row mới
    connect(rowWidget->btnDelete, &QPushButton::clicked, this, [this, newItem]() {
        onRowDeleted(newItem);
    });

    connect(rowWidget->nameWidget, &EditableComboWidget::editRequested, this, [this, rowWidget]() {
        provideNameOptions(rowWidget->nameWidget);
    });
    connect(rowWidget->nameWidget, &EditableComboWidget::valueChanged, this, &CameraMappingWidget::onDataChanged);

    connect(rowWidget->numberWidget, &EditableComboWidget::editRequested, this, [this, rowWidget]() {
        provideNumberOptions(rowWidget->numberWidget);
    });
    connect(rowWidget->numberWidget, &EditableComboWidget::valueChanged, this, &CameraMappingWidget::onDataChanged);

    // Luôn chèn vào trước nút "Add Row" (đang nằm ở count() - 1)
    insertItem(count() - 1, newItem);
    setItemWidget(newItem, rowWidget);
}

void CameraMappingWidget::onRowAdded(const QString &cameraName) {
    int autoNumber = getSmallestAvailableNumber();
    if (autoNumber > m_limitNumber) return;

    // Sử dụng Custom Item
    SortableCameraItem *newItem = new SortableCameraItem();
    newItem->setSizeHint(QSize(0, 40));

    CameraRowWidget *rowWidget = new CameraRowWidget(this);
    rowWidget->nameWidget->setText(cameraName);
    rowWidget->numberWidget->setText(QString::number(autoNumber));

    // Xử lý connections
    connect(rowWidget->btnDelete, &QPushButton::clicked, this, [this, newItem]() {
        onRowDeleted(newItem);
    });

    connect(rowWidget->nameWidget, &EditableComboWidget::editRequested, this, [this, rowWidget]() {
        provideNameOptions(rowWidget->nameWidget);
    });
    connect(rowWidget->nameWidget, &EditableComboWidget::valueChanged, this, &CameraMappingWidget::onDataChanged);

    connect(rowWidget->numberWidget, &EditableComboWidget::editRequested, this, [this, rowWidget]() {
        provideNumberOptions(rowWidget->numberWidget);
    });
    connect(rowWidget->numberWidget, &EditableComboWidget::valueChanged, this, &CameraMappingWidget::onDataChanged);

    // Thêm vào list
    insertItem(count() - 1, newItem);
    setItemWidget(newItem, rowWidget);

    onDataChanged();
}

void CameraMappingWidget::onRowDeleted(QListWidgetItem *item) {
    int r = row(item);
    delete takeItem(r);
    onDataChanged();
}

void CameraMappingWidget::updateSortingData() {
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem *item = this->item(i);
        if (item == m_addRowItem) continue; // Bỏ qua Add Row (đã set INT_MAX)

        CameraRowWidget *rowWidget = qobject_cast<CameraRowWidget*>(itemWidget(item));
        if (rowWidget) {
            // Cập nhật UserRole bằng giá trị number hiện tại để chuẩn bị sort
            int num = rowWidget->numberWidget->text().toInt();
            item->setData(Qt::UserRole, num);
        }
    }
}

void CameraMappingWidget::onDataChanged() {
    // 1. Cập nhật dữ liệu số cho các Item
    updateSortingData();

    // 2. Thực hiện sort (Add Row sẽ tự động ở lại cuối nhờ INT_MAX)
    sortItems(Qt::AscendingOrder);

    // 3. Cập nhật UI cho Add Row
    m_addRowWidget->setAvailableCameras(getAvailableCameras());

    // 4. Phát event ra ngoài hệ thống
    emit mappingChanged(getCurrentMapping());
}

void CameraMappingWidget::provideNameOptions(EditableComboWidget *widget) {
    QStringList available = getAvailableCameras();
    QString current = widget->text();
    if (!current.isEmpty()) {
        available.append(current);
    }

    QStringList sortedOptions;
    for (const QString &cam : m_allCameras) {
        if (available.contains(cam)) sortedOptions.append(cam);
    }

    widget->setOptions(sortedOptions, current);
}

void CameraMappingWidget::provideNumberOptions(EditableComboWidget *widget) {
    QList<int> availableNums = getAvailableNumbers();
    int currentNum = widget->text().toInt();
    if (currentNum > 0) {
        availableNums.append(currentNum);
    }
    std::sort(availableNums.begin(), availableNums.end());

    QStringList options;
    for (int n : availableNums) {
        options.append(QString::number(n));
    }
    widget->setOptions(options, widget->text());
}

QMap<int, QString> CameraMappingWidget::getCurrentMapping() const {
    QMap<int, QString> map;
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem *item = this->item(i);
        if (item == m_addRowItem) continue;

        CameraRowWidget *rowWidget = qobject_cast<CameraRowWidget*>(itemWidget(item));
        if (rowWidget) {
            QString name = rowWidget->nameWidget->text();
            int num = rowWidget->numberWidget->text().toInt();
            if (!name.isEmpty() && num > 0) {
                map.insert(num, name);
            }
        }
    }
    return map;
}

void CameraMappingWidget::setCurrentMapping(const QMap<int, QString> &mapping) {
    // 1. Chặn tín hiệu tạm thời để tránh gọi onDataChanged quá nhiều lần gây giật lag
    this->blockSignals(true);

    // 2. Xóa các row cũ (giữ lại row cuối cùng là m_addRowItem)
    while (count() > 1) {
        QListWidgetItem *item = takeItem(0);
        delete item;
    }

    // 3. Thêm các row từ mapping
    for (auto it = mapping.constBegin(); it != mapping.constEnd(); ++it) {
        addMappingRow(it.value(), it.key());
    }

    // 4. Mở lại tín hiệu và cập nhật UI/Sorting một lần duy nhất
    this->blockSignals(false);
    onDataChanged();
}

QStringList CameraMappingWidget::getAvailableCameras() const {
    QStringList available = m_allCameras;
    QMap<int, QString> currentMap = getCurrentMapping();
    for (auto it = currentMap.constBegin(); it != currentMap.constEnd(); ++it) {
        available.removeOne(it.value());
    }
    return available;
}

QList<int> CameraMappingWidget::getAvailableNumbers() const {
    QList<int> available;
    QMap<int, QString> currentMap = getCurrentMapping();
    QSet<int> usedNumbers;
    for (auto it = currentMap.constBegin(); it != currentMap.constEnd(); ++it) {
        usedNumbers.insert(it.key());
    }

    for (int i = 1; i <= m_limitNumber; ++i) {
        if (!usedNumbers.contains(i)) {
            available.append(i);
        }
    }
    return available;
}

int CameraMappingWidget::getSmallestAvailableNumber() const {
    QList<int> available = getAvailableNumbers();
    if (available.isEmpty()) return m_limitNumber + 1;
    std::sort(available.begin(), available.end());
    return available.first();
}
