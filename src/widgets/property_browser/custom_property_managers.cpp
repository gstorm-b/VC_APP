#include "custom_property_managers.h"
#include <QLocale>

// ============================================================================
//  PositionPropertyManager
// ============================================================================

// ── Static helpers ───────────────────────────────────────────────────────────

int PositionPropertyManager::modeComponentCount(Mode m)
{
    switch (m) {
    case XY:     return 2;
    case XYZ:    return 3;
    case XYZRPY: return 6;
    }
    return 2;
}

QStringList PositionPropertyManager::modeLabels(Mode m)
{
    switch (m) {
    case XY:     return {QStringLiteral("X"),    QStringLiteral("Y")};
    case XYZ:    return {QStringLiteral("X"),    QStringLiteral("Y"),     QStringLiteral("Z")};
    case XYZRPY: return {QStringLiteral("X"),    QStringLiteral("Y"),     QStringLiteral("Z"),
                         QStringLiteral("Roll"), QStringLiteral("Pitch"), QStringLiteral("Yaw")};
    }
    return {};
}

// ── Construction ─────────────────────────────────────────────────────────────

PositionPropertyManager::PositionPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
    , m_doubleManager(new QtDoublePropertyManager(this))
{
    connect(m_doubleManager, &QtDoublePropertyManager::valueChanged,
            this, &PositionPropertyManager::slotDoubleChanged);
    connect(m_doubleManager, &QtAbstractPropertyManager::propertyDestroyed,
            this, &PositionPropertyManager::slotPropertyDestroyed);
}

PositionPropertyManager::~PositionPropertyManager() = default;

QtDoublePropertyManager *PositionPropertyManager::subDoubleManager() const
{
    return m_doubleManager;
}

// ── Value accessors ──────────────────────────────────────────────────────────

QVector<double> PositionPropertyManager::value(const QtProperty *property) const
{
    return m_values.value(property).values;
}

PositionPropertyManager::Mode PositionPropertyManager::mode(const QtProperty *property) const
{
    return m_values.value(property).mode;
}

int PositionPropertyManager::decimals(const QtProperty *property) const
{
    return m_values.value(property).decimals;
}

double PositionPropertyManager::minimum(const QtProperty *property) const
{
    return m_values.value(property).minimum;
}

double PositionPropertyManager::maximum(const QtProperty *property) const
{
    return m_values.value(property).maximum;
}

double PositionPropertyManager::singleStep(const QtProperty *property) const
{
    return m_values.value(property).singleStep;
}

// ── Mutators ─────────────────────────────────────────────────────────────────

void PositionPropertyManager::setValue(QtProperty *property, const QVector<double> &val)
{
    auto it = m_values.find(property);
    if (it == m_values.end()) return;

    Data &data = it.value();
    const int n = modeComponentCount(data.mode);
    data.values.resize(n);
    for (int i = 0; i < n; ++i)
        data.values[i] = (i < val.size()) ? val[i] : 0.0;

    for (int i = 0; i < qMin(n, data.subProps.size()); ++i)
        m_doubleManager->setValue(data.subProps[i], data.values[i]);

    emit propertyChanged(property);
    emit valueChanged(property, data.values);
}

void PositionPropertyManager::setMode(QtProperty *property, Mode newMode)
{
    auto it = m_values.find(property);
    if (it == m_values.end() || it->mode == newMode) return;

    Data &data = it.value();
    destroySubProperties(property, data);

    data.mode = newMode;
    const int n = modeComponentCount(newMode);
    data.values.resize(n, 0.0);
    createSubProperties(property, data);

    emit modeChanged(property, newMode);
    emit propertyChanged(property);
}

void PositionPropertyManager::setDecimals(QtProperty *property, int prec)
{
    auto it = m_values.find(property);
    if (it == m_values.end()) return;

    it->decimals = prec;
    for (auto *sub : std::as_const(it->subProps))
        m_doubleManager->setDecimals(sub, prec);

    emit decimalsChanged(property, prec);
    emit propertyChanged(property);
}

void PositionPropertyManager::setRange(QtProperty *property, double minVal, double maxVal)
{
    auto it = m_values.find(property);
    if (it == m_values.end()) return;

    it->minimum = minVal;
    it->maximum = maxVal;
    for (auto *sub : std::as_const(it->subProps))
        m_doubleManager->setRange(sub, minVal, maxVal);
}

void PositionPropertyManager::setSingleStep(QtProperty *property, double step)
{
    auto it = m_values.find(property);
    if (it == m_values.end()) return;

    it->singleStep = step;
    for (auto *sub : std::as_const(it->subProps))
        m_doubleManager->setSingleStep(sub, step);
}

// ── Display text ─────────────────────────────────────────────────────────────

QString PositionPropertyManager::valueText(const QtProperty *property) const
{
    const auto it = m_values.constFind(property);
    if (it == m_values.constEnd()) return {};

    const Data &data = it.value();
    const QStringList labels = modeLabels(data.mode);
    QStringList parts;
    parts.reserve(data.values.size());
    for (int i = 0; i < data.values.size(); ++i)
        parts << QStringLiteral("%1:%2").arg(labels.value(i))
                                        .arg(data.values[i], 0, 'f', data.decimals);
    return parts.join(QStringLiteral("  "));
}

// ── QtAbstractPropertyManager overrides ──────────────────────────────────────

void PositionPropertyManager::initializeProperty(QtProperty *property)
{
    Data data;
    data.values = QVector<double>(modeComponentCount(data.mode), 0.0);
    m_values[property] = data;
    createSubProperties(property, m_values[property]);
}

void PositionPropertyManager::uninitializeProperty(QtProperty *property)
{
    auto it = m_values.find(property);
    if (it == m_values.end()) return;

    destroySubProperties(property, it.value());
    m_values.erase(it);
}

// ── Private helpers ───────────────────────────────────────────────────────────

void PositionPropertyManager::createSubProperties(QtProperty *parent, Data &data)
{
    const QStringList labels = modeLabels(data.mode);
    const int n = modeComponentCount(data.mode);

    data.subProps.clear();
    for (int i = 0; i < n; ++i) {
        auto *sub = m_doubleManager->addProperty(labels[i]);
        m_doubleManager->setDecimals  (sub, data.decimals);
        m_doubleManager->setRange     (sub, data.minimum, data.maximum);
        m_doubleManager->setSingleStep(sub, data.singleStep);
        m_doubleManager->setValue     (sub, data.values.value(i, 0.0));
        parent->addSubProperty(sub);
        m_subToParent[sub] = parent;
        data.subProps.append(sub);
    }
}

void PositionPropertyManager::destroySubProperties(QtProperty *parent, Data &data)
{
    for (auto *sub : std::as_const(data.subProps)) {
        m_subToParent.remove(sub);
        parent->removeSubProperty(sub);
        delete sub;
    }
    data.subProps.clear();
}

// ── Private slots ─────────────────────────────────────────────────────────────

void PositionPropertyManager::slotDoubleChanged(QtProperty *sub, double /*value*/)
{
    auto parentIt = m_subToParent.find(sub);
    if (parentIt == m_subToParent.end()) return;

    QtProperty *parent = parentIt.value();
    auto dataIt = m_values.find(parent);
    if (dataIt == m_values.end()) return;

    Data &data = dataIt.value();
    const int idx = data.subProps.indexOf(sub);
    if (idx < 0) return;

    data.values[idx] = m_doubleManager->value(sub);
    emit propertyChanged(parent);
    emit valueChanged(parent, data.values);
}

void PositionPropertyManager::slotPropertyDestroyed(QtProperty *sub)
{
    m_subToParent.remove(sub);
}

// ============================================================================
//  SizePropertyManager
// ============================================================================

// ── Static helpers ───────────────────────────────────────────────────────────

int SizePropertyManager::modeComponentCount(Mode m)
{
    return (m == WH) ? 2 : 3;
}

QStringList SizePropertyManager::modeLabels(Mode m)
{
    if (m == WH)
        return {QStringLiteral("Width"), QStringLiteral("Height")};
    return {QStringLiteral("Width"), QStringLiteral("Height"), QStringLiteral("Depth")};
}

// ── Construction ─────────────────────────────────────────────────────────────

SizePropertyManager::SizePropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
    , m_doubleManager(new QtDoublePropertyManager(this))
{
    connect(m_doubleManager, &QtDoublePropertyManager::valueChanged,
            this, &SizePropertyManager::slotDoubleChanged);
    connect(m_doubleManager, &QtAbstractPropertyManager::propertyDestroyed,
            this, &SizePropertyManager::slotPropertyDestroyed);
}

SizePropertyManager::~SizePropertyManager() = default;

QtDoublePropertyManager *SizePropertyManager::subDoubleManager() const
{
    return m_doubleManager;
}

// ── Value accessors ──────────────────────────────────────────────────────────

QVector<double> SizePropertyManager::value(const QtProperty *property) const
{
    return m_values.value(property).values;
}

SizePropertyManager::Mode SizePropertyManager::mode(const QtProperty *property) const
{
    return m_values.value(property).mode;
}

int SizePropertyManager::decimals(const QtProperty *property) const
{
    return m_values.value(property).decimals;
}

double SizePropertyManager::minimum(const QtProperty *property) const
{
    return m_values.value(property).minimum;
}

double SizePropertyManager::maximum(const QtProperty *property) const
{
    return m_values.value(property).maximum;
}

double SizePropertyManager::singleStep(const QtProperty *property) const
{
    return m_values.value(property).singleStep;
}

// ── Mutators ─────────────────────────────────────────────────────────────────

void SizePropertyManager::setValue(QtProperty *property, const QVector<double> &val)
{
    auto it = m_values.find(property);
    if (it == m_values.end()) return;

    Data &data = it.value();
    const int n = modeComponentCount(data.mode);
    data.values.resize(n);
    for (int i = 0; i < n; ++i)
        data.values[i] = (i < val.size()) ? val[i] : 0.0;

    for (int i = 0; i < qMin(n, data.subProps.size()); ++i)
        m_doubleManager->setValue(data.subProps[i], data.values[i]);

    emit propertyChanged(property);
    emit valueChanged(property, data.values);
}

void SizePropertyManager::setMode(QtProperty *property, Mode newMode)
{
    auto it = m_values.find(property);
    if (it == m_values.end() || it->mode == newMode) return;

    Data &data = it.value();
    destroySubProperties(property, data);

    data.mode = newMode;
    const int n = modeComponentCount(newMode);
    data.values.resize(n, 0.0);
    createSubProperties(property, data);

    emit modeChanged(property, newMode);
    emit propertyChanged(property);
}

void SizePropertyManager::setDecimals(QtProperty *property, int prec)
{
    auto it = m_values.find(property);
    if (it == m_values.end()) return;

    it->decimals = prec;
    for (auto *sub : std::as_const(it->subProps))
        m_doubleManager->setDecimals(sub, prec);

    emit decimalsChanged(property, prec);
    emit propertyChanged(property);
}

void SizePropertyManager::setRange(QtProperty *property, double minVal, double maxVal)
{
    auto it = m_values.find(property);
    if (it == m_values.end()) return;

    it->minimum = minVal;
    it->maximum = maxVal;
    for (auto *sub : std::as_const(it->subProps))
        m_doubleManager->setRange(sub, minVal, maxVal);
}

void SizePropertyManager::setSingleStep(QtProperty *property, double step)
{
    auto it = m_values.find(property);
    if (it == m_values.end()) return;

    it->singleStep = step;
    for (auto *sub : std::as_const(it->subProps))
        m_doubleManager->setSingleStep(sub, step);
}

// ── Display text ─────────────────────────────────────────────────────────────

QString SizePropertyManager::valueText(const QtProperty *property) const
{
    const auto it = m_values.constFind(property);
    if (it == m_values.constEnd()) return {};

    const Data &data = it.value();
    const QStringList labels = modeLabels(data.mode);
    QStringList parts;
    parts.reserve(data.values.size());
    for (int i = 0; i < data.values.size(); ++i)
        parts << QStringLiteral("%1:%2").arg(labels.value(i))
                                        .arg(data.values[i], 0, 'f', data.decimals);
    return parts.join(QStringLiteral(" × "));
}

// ── QtAbstractPropertyManager overrides ──────────────────────────────────────

void SizePropertyManager::initializeProperty(QtProperty *property)
{
    Data data;
    data.values = QVector<double>(modeComponentCount(data.mode), 0.0);
    m_values[property] = data;
    createSubProperties(property, m_values[property]);
}

void SizePropertyManager::uninitializeProperty(QtProperty *property)
{
    auto it = m_values.find(property);
    if (it == m_values.end()) return;

    destroySubProperties(property, it.value());
    m_values.erase(it);
}

// ── Private helpers ───────────────────────────────────────────────────────────

void SizePropertyManager::createSubProperties(QtProperty *parent, Data &data)
{
    const QStringList labels = modeLabels(data.mode);
    const int n = modeComponentCount(data.mode);

    data.subProps.clear();
    for (int i = 0; i < n; ++i) {
        auto *sub = m_doubleManager->addProperty(labels[i]);
        m_doubleManager->setDecimals  (sub, data.decimals);
        m_doubleManager->setRange     (sub, data.minimum, data.maximum);
        m_doubleManager->setSingleStep(sub, data.singleStep);
        m_doubleManager->setValue     (sub, data.values.value(i, 0.0));
        parent->addSubProperty(sub);
        m_subToParent[sub] = parent;
        data.subProps.append(sub);
    }
}

void SizePropertyManager::destroySubProperties(QtProperty *parent, Data &data)
{
    for (auto *sub : std::as_const(data.subProps)) {
        m_subToParent.remove(sub);
        parent->removeSubProperty(sub);
        delete sub;
    }
    data.subProps.clear();
}

// ── Private slots ─────────────────────────────────────────────────────────────

void SizePropertyManager::slotDoubleChanged(QtProperty *sub, double /*value*/)
{
    auto parentIt = m_subToParent.find(sub);
    if (parentIt == m_subToParent.end()) return;

    QtProperty *parent = parentIt.value();
    auto dataIt = m_values.find(parent);
    if (dataIt == m_values.end()) return;

    Data &data = dataIt.value();
    const int idx = data.subProps.indexOf(sub);
    if (idx < 0) return;

    data.values[idx] = m_doubleManager->value(sub);
    emit propertyChanged(parent);
    emit valueChanged(parent, data.values);
}

void SizePropertyManager::slotPropertyDestroyed(QtProperty *sub)
{
    m_subToParent.remove(sub);
}
