#ifndef QGADGET_MARCO_H
#define QGADGET_MARCO_H

#include <qtmetamacros.h>
#include <QtTypes>
#include <QString>
#include <QMetaEnum>

#define G_PROPERTY_NUMBER_READWRITE(type, name, minVal, maxVal, displayName) \
    Q_PROPERTY(type name READ name WRITE set##name) \
    Q_CLASSINFO(#name "_min", #minVal) \
    Q_CLASSINFO(#name "_max", #maxVal) \
    Q_CLASSINFO(#name "_name", displayName) \
    public: \
    type name() const { return m_##name; } \
    void set##name(type val) { m_##name = val; } \

#define G_PROPERTY_STRING_READWRITE(type, name, displayName) \
    Q_PROPERTY(type name READ name WRITE set##name) \
    Q_CLASSINFO(#name "_name", displayName) \
    public: \
    type name() const { return m_##name; } \
    void set##name(type val) { m_##name = val; } \

#define G_PROPERTY_STRING_READ(type, name, displayName) \
    Q_PROPERTY(type name READ name CONSTANT) \
    Q_CLASSINFO(#name "_name", displayName) \
    public: \
    type name() const { return m_##name; } \
    // void set##name(type val) { m_##name = val; } \

#define G_PROPERTY_BOOL_READWRITE(type, name, displayName) \
Q_PROPERTY(type name READ name WRITE set##name) \
    Q_CLASSINFO(#name "_name", displayName) \
    public: \
    type name() const { return m_##name; } \
    void set##name(type val) { m_##name = val; } \


#define G_PROPERTY_ENUM_READWRITE(type, name, displayName) \
Q_PROPERTY(type name READ name WRITE set##name) \
    Q_CLASSINFO(#name "_name", displayName) \
    public: \
    type name() const { return m_##name; } \
    void set##name(type val) { m_##name = val; } \

#define P_PROPERTY_STRING_READWRITE(type, name, displayName) \
Q_PROPERTY(type name READ name WRITE set##name) \
    Q_CLASSINFO(#name "_name", displayName) \
    public: \
    type name() const { return d->m_##name; } \
    void set##name(type val) { d->m_##name = val; } \

#endif // QGADGET_MARCO_H
