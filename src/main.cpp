#include <QApplication>
#include <QFont>
#include <QLabel>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>
#include <cmath>
#include <qabstractitemmodel.h>
#include <qaction.h>
#include <qapplication.h>

#include <QPushButton>
#include <QTabBar>
#include <QTabWidget>
#include <QLabel>
#include <qboxlayout.h>
#include <qbrush.h>
#include <qcoreapplication.h>
#include <qheaderview.h>
#include <qlogging.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpalette.h>
#include <qpicture.h>
#include <qscrollbar.h>
#include <qscroller.h>
#include <qsplitter.h>
#include <qstyle.h>
#include <qtabbar.h>
#include <qtableview.h>
#include <qtablewidget.h>
#include <qtabwidget.h>
#include <QMenuBar>
#include <QMenu>
#include <QMainWindow>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QString>
#include <QHeaderView>
#include <QStyleHints>
#include <QGuiApplication>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <qtoolbar.h>
#include <QIcon>
#include <QMessageBox>

#include <QFile>
#include <QDebug>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <qurl.h>
#include <vector>
#include <QShortcut>
#include <QAbstractTableModel>
#include <QSplitter>
#include <QSignalBlocker>
#include <QScrollBar>
#include <QSignalBlocker>
#include "./qcustomplot.h"
#include <QProgressDialog>
#include <QFileInfo>
#include <functional>

#include "./wlogV1.h"
#include "./wlogTableModel.h"
#include "./struc.h"
#include "./fileHeader.h"
#include "getRecord.h"
#include "wlogCommon.h"
#include "stats.h"
#include "drawPlotsTab.h"
#include "drawDataTableTab.h"
#include "formats.h"

std::vector<wlogFileInfo> files;

WLogTableModel::WLogTableModel(const std::vector<wlogFileInfo> &fileIndex, QObject *parent) : QAbstractTableModel(parent), files(fileIndex){
    recount();
}

QStringList selectedFiles = {
    "/home/lemion/Documents/Lemion Industries Meteobench/samplecontent/2026-07-31.wlog",
    "/home/lemion/Documents/Lemion Industries Meteobench/samplecontent/2026-08-01.wlog"
};

bool openWlog(const QString &path, wlogFileInfo &info){
    info.file = std::make_unique<QFile>(path);
    QFile &file = *info.file;

    if (!file.open(QIODevice::ReadOnly)) { 
        return false; 
    }
    if (file.size() < static_cast<qint64>(sizeof(wlog::headerPrefix))) { 
        return false; 
    }

    wlog::headerPrefix header; //header struc type whatnot

    if (file.read(reinterpret_cast<char*>(&header), sizeof(header)), sizeof(header) != sizeof(header)) { 
        return false; 
    }
    if (memcmp(header.magic, "WLOG", 4) != 0) { 
        return false; 
    }

    const wlog::LogFormat *fmt = findFormat(header.version);

    if (!fmt || header.recordSize != fmt->recordSize){
        return false;
    }
    if (file.size() < static_cast<qint64>(fmt->headerSize)){
        return false;
    }

    info.mapped = file.map(0, file.size());;
    if (!info.mapped){
        return false;
    }

    uint64_t dataSize = file.size() - fmt->headerSize;

    info.path = path;
    info.format = fmt;
    info.version = fmt->version;
    info.recordSize = fmt->recordSize;
    info.dataOffset = fmt->headerSize;
    info.recordCount = dataSize / fmt->recordSize;
    info.firstTimestamp = 0;

    if (info.recordCount > 0){
        info.firstTimestamp = static_cast<uint64_t>(wlog::readValue(info.mapped + info.dataOffset, fmt->columns[0]));
    }

    return true;
};

bool trimToValid(wlogFileInfo &info){
    const double minTime = 946684800.0; //2000 1 1
    const double maxTime = 4102444800.0; //2100 1 1
    double prev = 0;

    for (uint64_t i = 0; i < info.recordCount; i++){
        const uchar *rec = info.mapped + info.dataOffset + i * info.recordSize;
        double t = wlog::readValue(rec, info.format->columns[0]);
        bool shapeOk = !info.format->validRecord || info.format->validRecord(rec);

        if (!shapeOk || !(t >= minTime && t <= maxTime) || t < prev){
            info.recordCount = i;
            return true;
        }
        prev = t;
    }
    return false;
}

QStringList buildFileIndex(const QStringList &selectedFiles, std::vector<wlogFileInfo> &files, const std::function<bool(int, const QString &)> &onProgress = nullptr){
    QStringList failed;
    int done = 0;
    files.clear();

    for (const QString &path : selectedFiles) {
        if (onProgress && !onProgress(done, path)){
            return failed;
        }
        wlogFileInfo info;

        if (openWlog(path, info)) {
            info.trimmed = trimToValid(info);

            if (info.recordCount == 0){
                failed << path;
            }
            else {
                computeStats(info);
                files.push_back(std::move(info));
            }
        }
        else {
            failed << path;
        }

        done++;
    }

    std::sort(files.begin(), files.end(), [](const wlogFileInfo &a, const wlogFileInfo &b){
        return a.firstTimestamp < b.firstTimestamp;
    });

    uint64_t nextRow = 0;

    for (wlogFileInfo &file : files) {
        file.firstRow = nextRow;
        nextRow += file.recordCount;
    }
    return failed;
}

void About(QWidget *window){
    QMessageBox::about(
        window,
        "About MeteoBench",
        "MeteoBench v1.0.0\n"
        "Meteorological Data Analysis Software\n"
        "Copyright © 2026 Lemion Industries\n\n"
        "Meow mraaoaw mew mrrrp :3"
    );
};

int main(int argc, char *argv[])
{
    buildFileIndex(selectedFiles, files);
    // App setup
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    QPalette palette = app.palette();

    palette.setColor(QPalette::Window, QColor("#dfe8f6"));
    palette.setColor(QPalette::WindowText, Qt::black);
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::AlternateBase, QColor("#edf3fc"));
    palette.setColor(QPalette::Text, Qt::black);
    palette.setColor(QPalette::Button, QColor("#dfe8f6"));
    palette.setColor(QPalette::ButtonText, Qt::black);
    palette.setColor(QPalette::Highlight, QColor("#316ac5"));
    palette.setColor(QPalette::HighlightedText, Qt::white);

    app.setPalette(palette);
    app.setStyle("Windows"); // or fusion

    QFile stylesheet("src/style.qss");
    if (stylesheet.open(QIODevice::ReadOnly)){
        app.setStyleSheet(QString::fromUtf8(stylesheet.readAll()));
    }

    // Window setup
    QMainWindow window;
    window.setWindowTitle("MeteoBench");
    window.resize(800, 600);

    // Toolbar setup
    QToolBar *toolBar = new QToolBar(&window);
    toolBar->setMovable(false);
    toolBar->setFloatable(false);

    window.addToolBar(toolBar);

    // Actions
    QAction *openAction = new QAction(QIcon("src/icons/Open.png"), "&Open", &window);
    openAction->setShortcut(QKeySequence::Open);
    openAction->setToolTip("Open WLOG files (Ctrl + O)");

    QAction *validateAction = new QAction(QIcon("src/icons/Validate.png"), "&Validate", &window);
    validateAction->setToolTip("Validate opened WLOG files\nChecks for corrupt file regions");
    validateAction->setEnabled(false);

    QAction *aboutAction = new QAction("&About MeteoBench", &window);
    QObject::connect(aboutAction, &QAction::triggered, [&window](){About(&window);});

    // Menubar Setup
    QMenuBar *menuBar = window.menuBar();
    QMenu *fileMenu = menuBar->addMenu("&File");
    fileMenu->addAction(openAction);

    QMenu *editMenu = menuBar->addMenu("&Edit");
    editMenu->setEnabled(false);

    QMenu *viewMenu = menuBar->addMenu("&View");

    QMenu *recordsMenu = menuBar->addMenu("&Records");
    recordsMenu->addAction(validateAction);

    QMenu *plotsMenu = new StayOpenMenu("&Plots", &window);
    menuBar->addMenu(plotsMenu);

    QMenu *analysisMenu = menuBar->addMenu("&Analysis");
    analysisMenu->setEnabled(false);

    QMenu *helpMenu = menuBar->addMenu("&Help");
    helpMenu->addAction(aboutAction);

    // Tabs setup
    QTabWidget *tabWidget = new QTabWidget(&window);
    tabWidget->tabBar()->setFocusPolicy(Qt::NoFocus);
    tabWidget->tabBar()->setCursor(Qt::PointingHandCursor);

    QWidget *dataTableTab = new QWidget();

    QWidget *plotsTab = new QWidget();

    QWidget *analysisTab = new QWidget();

    tabWidget->addTab(dataTableTab, "Data Table");
    tabWidget->addTab(plotsTab, "Plots");
    tabWidget->addTab(analysisTab, "Analysis");

    new QShortcut(QKeySequence("Ctrl+1"), &window, [tabWidget](){ tabWidget->setCurrentIndex(0);});
    new QShortcut(QKeySequence("Ctrl+2"), &window, [tabWidget](){ tabWidget->setCurrentIndex(1);});
    new QShortcut(QKeySequence("Ctrl+3"), &window, [tabWidget](){ tabWidget->setCurrentIndex(2);});

    window.setCentralWidget(tabWidget);

    // Append actions to toolbar
    toolBar->addAction(openAction);
    toolBar->addAction(validateAction);
    toolBar->addSeparator();

    WLogTableModel *model = new WLogTableModel(files, &window);

    std::vector<QAction*> dataActions;
    std::vector<QAction*> plotActions;

    auto syncToolBar = [&](int index){
        for (QAction *a : dataActions){
            a->setVisible(index == 0);
        }
        for (QAction *a : plotActions){
            a->setVisible(index == 1);
        }
        plotsMenu->setEnabled(index == 1);
    };
    QObject::connect(tabWidget, &QTabWidget::currentChanged, syncToolBar);

    QObject::connect(openAction, &QAction::triggered, [&](){
        QStringList paths = QFileDialog::getOpenFileNames(&window, "Open WLOG Files", QString(), "WLOG files (*.wlog);;All files (*)");
        if (paths.isEmpty()){
            return;
        }

        QProgressDialog progress("", "Cancel", 0, static_cast<int>(paths.size()), &window);
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(300);

        auto labelFor = [](const QString &name){
            return QString("<table><tr><td><img src='src/icons/Clock.png'</td><td style='padding-left: 8px;'>Pre-analyzing files...<br>%1</td></tr></table>").arg(name);
        };

        std::vector<wlogFileInfo> fresh;
        QStringList failed = buildFileIndex(paths, fresh, [&](int done, const QString &path){
            progress.setLabelText(labelFor(QFileInfo(path).fileName()));
            progress.setValue(done);
            return !progress.wasCanceled();
        });

        if (progress.wasCanceled()){
            return;
        }

        progress.setValue(static_cast<int>(paths.size()));

        if (fresh.empty()){
            QMessageBox::warning(&window, "Nothing opened", "None of the selected files are valid WLOG files:\n\n" + failed.join("\n"));
            return;
        }

        model->beginReload();
        files= std::move(fresh);
        model->endReload();

        qDeleteAll(plotActions);
        plotActions.clear();
        qDeleteAll(plotsMenu->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly));
        plotsMenu->clear();

        const QObjectList kids = plotsTab->children();
        qDeleteAll(kids);

        drawPlotsTab(plotsTab, model, viewMenu, toolBar, tabWidget, plotsMenu, files, plotActions);
        syncToolBar(tabWidget->currentIndex());

        QStringList trimmed;
        for (const auto &f : files){
            if (f.trimmed){
                trimmed << QFileInfo(f.path).fileName() + " (" + QString::number(f.recordCount) + " valid records kept)";
            }
        }

        if (!failed.isEmpty() || !trimmed.isEmpty()){
            QString msg;
            if (!failed.isEmpty()){
                msg += "Skipped (not valid WLOG files):\n" + failed.join("\n") + "\n\n";
            }
            if (!trimmed.isEmpty()){
                msg += "Damaged files, loaded up to the first bad record:\n" + trimmed.join("\n");
            }
            QMessageBox::warning(&window, "Some files had problems", msg);
        }
    });

    drawDataTableTab(dataTableTab, model, viewMenu, recordsMenu, toolBar, tabWidget, files, dataActions);
    drawPlotsTab(plotsTab, model, viewMenu, toolBar, tabWidget, plotsMenu, files, plotActions);
    syncToolBar(tabWidget->currentIndex());

    window.show();

    return app.exec();
}
