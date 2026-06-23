#pragma once

#include <QObject>

class UrdfAdapterTests : public QObject
{
    Q_OBJECT

private slots:
    void exportIncludesLinksJointsAndMetadataWarning();
    void importExportedSerialChainAndPreserveFk();
};
