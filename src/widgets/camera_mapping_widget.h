#ifndef CAMERAMAPPINGWIDGET_H
#define CAMERAMAPPINGWIDGET_H

#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMap>
#include <QStringList>
#include <QEvent>

// --- Component 1: Label có thể click để chuyển thành ComboBox ---
class EditableComboWidget : public QStackedWidget {
    Q_OBJECT
public:
    explicit EditableComboWidget(QWidget *parent = nullptr);
    void setText(const QString &text);
    QString text() const;
    void setOptions(const QStringList &options, const QString &currentSelection);

signals:
    void valueChanged(const QString &newValue);
    void editRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QLabel *m_label;
    QComboBox *m_comboBox;
};

// --- Component 2: Widget cho một Row bình thường ---
class CameraRowWidget : public QWidget {
    Q_OBJECT
public:
    explicit CameraRowWidget(QWidget *parent = nullptr);
    EditableComboWidget *nameWidget;
    EditableComboWidget *numberWidget;
    QPushButton *btnDelete;
};

// --- Component 3: Widget cho Row cuối cùng (Add) ---
class AddRowWidget : public QStackedWidget {
    Q_OBJECT
public:
    explicit AddRowWidget(QWidget *parent = nullptr);
    void setAvailableCameras(const QStringList &cameras);

signals:
    void addRequested(const QString &cameraName);

private:
    QWidget *createAddButtonPage();
    QWidget *createSelectCameraPage();

    QPushButton *btnAddRow;
    QComboBox *cbCameraSelect;
    QPushButton *btnCancel;
};

// --- Component 4: Custom Item để hỗ trợ Sorting ---
class SortableCameraItem : public QListWidgetItem {
public:
    explicit SortableCameraItem(QListWidget *view = nullptr) : QListWidgetItem(view) {}

    // Override toán tử so sánh để sort theo giá trị số (lưu trong Qt::UserRole)
    bool operator<(const QListWidgetItem &other) const override {
        return data(Qt::UserRole).toInt() < other.data(Qt::UserRole).toInt();
    }
};

// --- Component 5: Widget Chính (Manager) ---
class CameraMappingWidget : public QListWidget {
    Q_OBJECT
public:
    explicit CameraMappingWidget(QWidget *parent = nullptr);
    void setCameraList(const QStringList &cameras);
    void setNumberLimit(int limit);
    QMap<int, QString> getCurrentMapping() const;
    void setCurrentMapping(const QMap<int, QString> &mapping);

signals:
    void mappingChanged(const QMap<int, QString> &mapping);

private slots:
    void onRowAdded(const QString &cameraName);
    void onRowDeleted(QListWidgetItem *item);
    void onDataChanged();

    void provideNameOptions(EditableComboWidget *widget);
    void provideNumberOptions(EditableComboWidget *widget);

private:
    void setupAddRow();
    void addMappingRow(const QString &cameraName, int number);
    int getSmallestAvailableNumber() const;
    QStringList getAvailableCameras() const;
    QList<int> getAvailableNumbers() const;
    void updateSortingData();

    QStringList m_allCameras;
    int m_limitNumber = 8;
    SortableCameraItem *m_addRowItem = nullptr;
    AddRowWidget *m_addRowWidget = nullptr;
};

#endif // CAMERAMAPPINGWIDGET_H
