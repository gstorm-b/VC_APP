#pragma once

#include <QObject>

class FrameToolTests : public QObject
{
    Q_OBJECT

private slots:
    void toolRegistryAddGetAndDefault();
    void toolRegistryMissingReturnsToolNotFound();
    void frameRegistryAddAndGet();
    void frameRegistryMissingReturnsFrameNotFound();
};
