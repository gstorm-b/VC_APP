#pragma once

#include <QObject>

class StlMeshLoaderTests : public QObject
{
    Q_OBJECT

private slots:
    void loadsAsciiStlAndNormalizesMillimetersToMeters();
    void loadsBinaryStlAndPreservesMeterScale();
    void rejectsDegenerateTriangles();
    void filtersDegenerateTrianglesWhenSkipModeIsRequested();
    void rejectsAllDegenerateAsciiStlWhenSkipModeLeavesNoTriangles();
    void rejectsInvalidPayloadAndNonPositiveScale();
};
