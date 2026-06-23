#pragma once

#include <QObject>

class CustomPresetTests : public QObject
{
    Q_OBJECT

private slots:
    void builderCreatesCustomSixDofConfigThatSolvesIk();
    void jsonLoaderRejectsUnknownTopLevelField();
    void jsonLoaderRejectsNonCanonicalUnits();
};
