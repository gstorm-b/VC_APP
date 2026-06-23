#pragma once

#include <QObject>

class CollisionProfileJsonTests : public QObject
{
    Q_OBJECT

private slots:
    void loaderRejectsInvalidSchemaAndUnits();
    void virtualProfileJsonMatchesCppFallbackAndRepresentativeStates();
    void nachiProfileJsonMatchesCppFallbackAndRepresentativeStates();
};
