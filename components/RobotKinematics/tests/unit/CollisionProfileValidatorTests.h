#pragma once

#include <QObject>

class CollisionProfileValidatorTests : public QObject
{
    Q_OBJECT

private slots:
    void validProfilePasses();
    void duplicateGeometryIdsAreRejected();
    void missingLinkIdsAreRejected();
    void nonPositivePrimitiveDimensionsAreRejected();
    void invalidDisabledPairsAreRejected();
};
