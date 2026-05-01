#include "add_device_wizard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMessageBox>
#include <QJsonObject>
#include <QApplication>
#include <QStyle>
#include <QFont>
#include <QLabel>

#include "logger/app_logger.h"
#include "device/device_factory.h"
#include "device/camera/camera_device.h"
#include "device/plc/mc_protocol_device.h"
#include "windows_helper.h"

// ──────────────────────────────────────────────────────────────────────────────
//  Static card metadata
// ──────────────────────────────────────────────────────────────────────────────
struct CardMeta {
    vc::device::DeviceType type;
    QString                label;
    QString                desc;
    QString                defaultName;
    QString                iconPath;
    QColor                 color;
};

static const QList<CardMeta> kCards = {
    { vc::device::DeviceType::Camera,
      "Camera",
      "Basler GigE / USB3 Vision",
      "Basler_cam_01",
      ":/resrc/icon/camera.svg",
      QColor(0x2b, 0x8c, 0xe8) },

    { vc::device::DeviceType::McDevice,
      "PLC",
      "Mitsubishi / Omron / Siemens",
      "PLC_Mitsu_01",
      ":/resrc/icon/plc_icon.svg",
      QColor(0x22, 0xd1, 0x7a) },

    { vc::device::DeviceType::Robot,
      "Robot Controller",
      "Fanuc / KUKA / ABB",
      "Robot_Fanuc_01",
      ":/resrc/icon/robot_movement.svg",
      QColor(0xf5, 0xa6, 0x23) },
};

// ──────────────────────────────────────────────────────────────────────────────
//  Constructor
// ──────────────────────────────────────────────────────────────────────────────
AddDeviceWizard::AddDeviceWizard(std::shared_ptr<vc::device::DeviceManager> mng,
                                 const QString &taskName,
                                 QWidget *parent)
    : QDialog(parent), m_manager(mng)
{
    setWindowTitle(tr("Add Device"));
    setMinimumWidth(520);
    setModal(true);

    for (const auto &c : kCards) {
        m_defaultNames.insert(c.type, c.defaultName);
        m_cardColors.insert(c.type, c.color);
    }

    if (mng) {
        m_cbxCameraType = new QComboBox(this);
        m_cbxCameraType->addItems(mng->getSubDeviceTypeList(vc::device::Camera));
        m_cbxCameraType->hide();

        m_cbxMcFrameType = new QComboBox(this);
        m_cbxMcFrameType->addItems(mng->getSubDeviceTypeList(vc::device::McDevice));
        m_cbxMcFrameType->hide();

        m_cbxMcCode = new QComboBox(this);
        m_cbxMcCode->addItem(
            vc::device::mc::McDataCodeToString(vc::device::mc::McDataCode::Binary));
        m_cbxMcCode->hide();
    }

    buildUi(taskName);
    selectCard(vc::device::DeviceType::Camera);
}

// ──────────────────────────────────────────────────────────────────────────────
//  UI construction
// ──────────────────────────────────────────────────────────────────────────────
void AddDeviceWizard::buildUi(const QString &taskName)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Dialog header ────────────────────────────────────────────────────────
    auto *headerFrame = new QFrame(this);
    headerFrame->setObjectName("adwHeader");
    headerFrame->setStyleSheet(
        "#adwHeader { background: #0e1520; border-bottom: 1px solid #1a2540; }");

    auto *headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(20, 14, 20, 14);
    headerLayout->setSpacing(0);

    auto *titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);

    auto *titleLbl = new QLabel(tr("Add Device"), headerFrame);
    QFont titleFont = titleLbl->font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    titleLbl->setFont(titleFont);
    titleLbl->setStyleSheet("color: #dce8f5;");
    titleCol->addWidget(titleLbl);

    if (!taskName.isEmpty()) {
        auto *subLbl = new QLabel(tr("Task: ") + taskName, headerFrame);
        QFont subFont = subLbl->font();
        subFont.setPointSize(9);
        subLbl->setFont(subFont);
        subLbl->setStyleSheet("color: #3a4f6a;");
        titleCol->addWidget(subLbl);
    }

    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();
    root->addWidget(headerFrame);

    // ── Body ─────────────────────────────────────────────────────────────────
    auto *bodyWidget = new QWidget(this);
    auto *body = new QVBoxLayout(bodyWidget);
    body->setContentsMargins(24, 22, 24, 22);
    body->setSpacing(16);

    auto makeSectionLabel = [&](const QString &text) -> QLabel * {
        auto *lbl = new QLabel(text, bodyWidget);
        QFont f = lbl->font();
        f.setPointSizeF(8.0);
        f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
        lbl->setFont(f);
        lbl->setStyleSheet("color: #6b7ea0;");
        return lbl;
    };

    // Device type section
    body->addWidget(makeSectionLabel(tr("DEVICE TYPE")));

    auto *cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(8);

    for (const auto &info : kCards) {
        auto *card = new QFrame(bodyWidget);
        card->setFixedHeight(116);
        card->setCursor(Qt::PointingHandCursor);
        card->installEventFilter(this);

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(12, 14, 12, 10);
        cardLayout->setSpacing(5);
        cardLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

        auto *iconLbl = new QLabel(card);
        iconLbl->setPixmap(svgIcon(info.iconPath).pixmap(22, 22));
        iconLbl->setAlignment(Qt::AlignCenter);
        iconLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
        cardLayout->addWidget(iconLbl);

        auto *nameLbl = new QLabel(info.label, card);
        QFont nf = nameLbl->font();
        nf.setPointSizeF(9.5);
        nf.setBold(true);
        nameLbl->setFont(nf);
        nameLbl->setAlignment(Qt::AlignCenter);
        nameLbl->setObjectName("cardTitle");
        nameLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
        cardLayout->addWidget(nameLbl);

        auto *descLbl = new QLabel(info.desc, card);
        QFont df = descLbl->font();
        df.setPointSizeF(7.5);
        descLbl->setFont(df);
        descLbl->setAlignment(Qt::AlignCenter);
        descLbl->setWordWrap(true);
        descLbl->setStyleSheet("color: #3a4f6a;");
        descLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
        cardLayout->addWidget(descLbl);

        m_cards.insert(info.type, card);
        cardsLayout->addWidget(card);
    }

    body->addLayout(cardsLayout);

    // Device name section
    body->addWidget(makeSectionLabel(tr("DEVICE NAME")));

    m_nameEdit = new QLineEdit(bodyWidget);
    QFont monoFont("JetBrains Mono");
    if (!monoFont.exactMatch())
        monoFont.setFamily("Consolas");
    monoFont.setPointSizeF(10);
    m_nameEdit->setFont(monoFont);
    m_nameEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #0d1420; border: 1px solid #243044;"
        "  border-radius: 5px; padding: 8px 11px; color: #dce8f5;"
        "}"
        "QLineEdit:focus { border-color: #2b8ce8; }");
    connect(m_nameEdit, &QLineEdit::textChanged, this, &AddDeviceWizard::onNameChanged);
    body->addWidget(m_nameEdit);

    // Info strip
    auto *infoFrame = new QFrame(bodyWidget);
    infoFrame->setStyleSheet(
        "QFrame { background: #0a1018; border: 1px solid #1a2540; border-radius: 6px; }");
    auto *infoLayout = new QHBoxLayout(infoFrame);
    infoLayout->setContentsMargins(14, 10, 14, 10);
    infoLayout->setSpacing(10);

    auto *warnIcon = new QLabel(infoFrame);
    warnIcon->setPixmap(
        QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(14, 14));
    warnIcon->setStyleSheet("border: none;");

    auto *infoText = new QLabel(
        tr("Device can be configured after adding. You can move it to another task at any time."),
        infoFrame);
    QFont infoFont = infoText->font();
    infoFont.setPointSizeF(8.5);
    infoText->setFont(infoFont);
    infoText->setWordWrap(true);
    infoText->setStyleSheet("color: #6b7ea0; border: none;");

    infoLayout->addWidget(warnIcon, 0, Qt::AlignTop);
    infoLayout->addWidget(infoText, 1);
    body->addWidget(infoFrame);

    // Button row
    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);
    btnRow->addStretch();

    auto *cancelBtn = new QPushButton(tr("Cancel"), bodyWidget);
    cancelBtn->setStyleSheet(
        "QPushButton {"
        "  background: transparent; border: 1px solid #243044;"
        "  border-radius: 5px; padding: 9px 22px; color: #6b7ea0; font-size: 12px;"
        "}"
        "QPushButton:hover { border-color: #4a6080; color: #8aabcc; }");
    connect(cancelBtn, &QPushButton::clicked, this, &AddDeviceWizard::reject);
    btnRow->addWidget(cancelBtn);

    m_addBtn = new QPushButton(tr("Add Device"), bodyWidget);
    m_addBtn->setEnabled(false);
    connect(m_addBtn, &QPushButton::clicked, this, &AddDeviceWizard::onAddClicked);
    btnRow->addWidget(m_addBtn);

    body->addLayout(btnRow);
    root->addWidget(bodyWidget);
}

// ──────────────────────────────────────────────────────────────────────────────
//  Card selection
// ──────────────────────────────────────────────────────────────────────────────
void AddDeviceWizard::selectCard(vc::device::DeviceType type)
{
    m_selectedType = type;

    for (auto it = m_cards.cbegin(); it != m_cards.cend(); ++it) {
        QFrame *card  = it.value();
        bool   active = (it.key() == type);
        QColor col    = m_cardColors.value(it.key());

        if (active) {
            card->setStyleSheet(QString(
                "QFrame {"
                "  border: 1px solid %1;"
                "  background-color: rgba(%2, %3, %4, 18);"
                "  border-radius: 7px;"
                "}")
                .arg(col.name())
                .arg(col.red()).arg(col.green()).arg(col.blue()));

            auto *titleLbl = card->findChild<QLabel *>("cardTitle");
            if (titleLbl) titleLbl->setStyleSheet("color: #dce8f5;");
        } else {
            card->setStyleSheet(
                "QFrame {"
                "  border: 1px solid #1f2d42;"
                "  background-color: #0d1420;"
                "  border-radius: 7px;"
                "}");
            auto *titleLbl = card->findChild<QLabel *>("cardTitle");
            if (titleLbl) titleLbl->setStyleSheet("color: #6b7ea0;");
        }
    }

    // Auto-populate device name
    if (m_nameEdit)
        m_nameEdit->setText(m_defaultNames.value(type));

    // Recolor Add Device button to match selected type
    if (m_addBtn) {
        QColor col   = m_cardColors.value(type);
        QColor hover = col.lighter(115);
        m_addBtn->setStyleSheet(QString(
            "QPushButton {"
            "  background: %1; border: none;"
            "  border-radius: 5px; padding: 9px 22px; color: #fff;"
            "  font-size: 12px; font-weight: bold;"
            "}"
            "QPushButton:hover { background: %2; }"
            "QPushButton:disabled { background-color: rgba(%3, %4, %5, 60); color: #ffffff60; }")
            .arg(col.name(), hover.name())
            .arg(col.red()).arg(col.green()).arg(col.blue()));
    }
}

// ──────────────────────────────────────────────────────────────────────────────
//  Public API
// ──────────────────────────────────────────────────────────────────────────────
int AddDeviceWizard::showWizard()
{
    if (!m_manager) return QDialog::Rejected;
    m_pendingDeviceId = m_manager->allocatePendingId();
    if (m_nameEdit) m_nameEdit->setFocus();
    return exec();
}

QString AddDeviceWizard::getDeviceName() const
{
    return m_nameEdit ? m_nameEdit->text().trimmed() : QString();
}

QString AddDeviceWizard::getDeviceType() const
{
    return vc::device::DeviceTypeToString(m_selectedType);
}

void AddDeviceWizard::reject()
{
    if (m_manager && !m_pendingDeviceId.isEmpty())
        m_manager->releasePendingId(m_pendingDeviceId);
    m_pendingDeviceId.clear();
    QDialog::reject();
}

// ──────────────────────────────────────────────────────────────────────────────
//  Slots
// ──────────────────────────────────────────────────────────────────────────────
void AddDeviceWizard::onCardClicked(vc::device::DeviceType type)
{
    selectCard(type);
}

void AddDeviceWizard::onNameChanged(const QString &text)
{
    if (m_addBtn)
        m_addBtn->setEnabled(!text.trimmed().isEmpty());
}

void AddDeviceWizard::onAddClicked()
{
    const QString name = m_nameEdit->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Add Device"), tr("Please enter a device name."));
        m_nameEdit->setFocus();
        return;
    }

    if (m_manager->isNameExists(name)) {
        QMessageBox::warning(this, tr("Add Device"), tr("Device name already exists."));
        m_nameEdit->selectAll();
        return;
    }

    QJsonObject jobj = buildDeviceJson(m_selectedType);
    jobj[DEVICE_JSK_ID]   = m_pendingDeviceId;
    jobj[DEVICE_JSK_NAME] = name;
    jobj[DEVICE_JSK_TYPE] = vc::device::DeviceTypeToString(m_selectedType);

    std::shared_ptr<vc::device::IDevice> dv(
        vc::device::DeviceFactory::create(m_selectedType, jobj));

    if (m_manager->commitDevice(m_pendingDeviceId, name, dv)) {
        LOG_USER_INFO << QString("Device added: %1 (%2)").arg(name, m_pendingDeviceId);
        accept();
    } else {
        QMessageBox::critical(this, tr("Add Device"),
            tr("Failed to register device. Please close and try again."));
    }
}

// ──────────────────────────────────────────────────────────────────────────────
//  Device JSON builder (uses defaults from hidden combo boxes)
// ──────────────────────────────────────────────────────────────────────────────
QJsonObject AddDeviceWizard::buildDeviceJson(vc::device::DeviceType type)
{
    QJsonObject obj;

    switch (type) {
    case vc::device::DeviceType::Camera:
        if (m_cbxCameraType && m_cbxCameraType->count() > 0)
            obj[DEVICE_JSK_CAM_TYPE] = m_cbxCameraType->currentText();
        break;

    case vc::device::McDevice: {
        vc::device::McProtocolConfig config;
        auto frameType = (m_cbxMcFrameType && m_cbxMcFrameType->count() > 0)
            ? vc::device::mc::McFrameTypeFromString(m_cbxMcFrameType->currentText())
            : vc::device::mc::McFrameType::Frame_3E;
        auto dataCode = (m_cbxMcCode && m_cbxMcCode->count() > 0)
            ? vc::device::mc::McDataCodeFromString(m_cbxMcCode->currentText())
            : vc::device::mc::McDataCode::Binary;
        config.configMcProtocol(frameType, dataCode);
        obj[DEVICE_JSK_CONFIG] = config.toJson();
        break;
    }

    default:
        break;
    }

    return obj;
}

// ──────────────────────────────────────────────────────────────────────────────
//  Event handling
// ──────────────────────────────────────────────────────────────────────────────
bool AddDeviceWizard::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonPress) {
        for (auto it = m_cards.cbegin(); it != m_cards.cend(); ++it) {
            if (obj == it.value()) {
                onCardClicked(it.key());
                return true;
            }
        }
    }
    return QDialog::eventFilter(obj, ev);
}

void AddDeviceWizard::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        event->ignore();
    } else if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
               && m_addBtn && m_addBtn->isEnabled()) {
        onAddClicked();
    } else {
        QDialog::keyPressEvent(event);
    }
}
