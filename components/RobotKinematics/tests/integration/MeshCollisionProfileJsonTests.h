#pragma once

#include <QObject>

class MeshCollisionProfileJsonTests : public QObject
{
    Q_OBJECT

private slots:
    void loaderRejectsUnknownTopLevelFieldsAndMissingScale();
    void loaderParsesBackendPreferenceAndMetadata();
    void loaderResolvesRelativeMeshPathsFromProfileFile();
    void loaderRejectsNonNumericMeshTransformsAndOptions();
    void validatorRejectsInvalidLinkAndBrokenSimplifiedMetadata();
    void loaderRejectsDuplicateMeshIdsAndInvalidDisabledPairs();
};
