#ifndef WINDOWS_HELPER_H
#define WINDOWS_HELPER_H

#include <QTime>
#include <QLabel>
#include <QTextEdit>
#include <QCalendarWidget>
#include <QFrame>
#include <QTreeView>
#include <QFileSystemModel>
#include <QBoxLayout>
#include <QSettings>
#include <QDockWidget>
#include <QDebug>
#include <QResizeEvent>
#include <QAction>
#include <QWidgetAction>
#include <QComboBox>
#include <QInputDialog>
#include <QRubberBand>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QScreen>
#include <QStyle>
#include <QMessageBox>
#include <QMenu>
#include <QToolButton>
#include <QToolBar>
#include <QPointer>
#include <QMap>
#include <QElapsedTimer>
#include <QRandomGenerator>

// #include "DockAreaTabBar.h"
// #include "DockAreaTitleBar.h"
// #include "DockAreaWidget.h"
// #include "DockComponentsFactory.h"
// #include "DockManager.h"
// #include "DockSplitter.h"
// #include "FloatingDockContainer.h"
// #include "ImageViewer.h"
// #include "MyDockAreaTitleBar.h"
// #include "StatusDialog.h"

#include "DockWidget.h"
// #include "ads_globals.h"

/**
 * Returns a random number from 0 to highest - 1
 */
static int randomNumberBounded(int highest)
{
    return QRandomGenerator::global()->bounded(highest);
}


/**
 * Function returns a features string with closable (c), movable (m) and floatable (f)
 * features. i.e. The following string is for a not closable but movable and floatable
 * widget: c- m+ f+
 */
static QString featuresString(ads::CDockWidget* DockWidget)
{
    auto f = DockWidget->features();
    return QString("c%1 m%2 f%3")
        .arg(f.testFlag(ads::CDockWidget::DockWidgetClosable) ? "+" : "-")
        .arg(f.testFlag(ads::CDockWidget::DockWidgetMovable) ? "+" : "-")
        .arg(f.testFlag(ads::CDockWidget::DockWidgetFloatable) ? "+" : "-");
}


/**
 * Appends the string returned by featuresString() to the window title of
 * the given DockWidget
 */
static void appendFeaturStringToWindowTitle(ads::CDockWidget* DockWidget)
{
    DockWidget->setWindowTitle(DockWidget->windowTitle()
                               +  QString(" (%1)").arg(featuresString(DockWidget)));
}

/**
 * Helper function to create an SVG icon
 */
static QIcon svgIcon(const QString& File, int intent = 92)
{
    // This is a workaround, because in item views SVG icons are not
    // properly scaled and look blurry or pixelate
    QIcon SvgIcon(File);
    SvgIcon.addPixmap(SvgIcon.pixmap(intent));
    return SvgIcon;
}


#endif // WINDOWS_HELPER_H
