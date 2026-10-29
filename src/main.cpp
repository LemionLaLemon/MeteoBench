#include <QApplication>
#include <QFont>
#include <QLabel>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>
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
#include <qlogging.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpalette.h>
#include <qpicture.h>
#include <qscroller.h>
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

#include "./wlogV1.h"
#include "./wlogTableModel.h"
#include "./struc.h"
#include "./fileHeader.h"

std::vector<wlogFileInfo> files;

QStringList selectedFiles = {
    "/home/lemion/Documents/Lemion Industries Meteobench/samplecontent/2026-07-31.wlog",
    "/home/lemion/Documents/Lemion Industries Meteobench/samplecontent/2026-08-01.wlog"
};

bool getRecord(uint64_t globalRow, const std::vector<wlogFileInfo> &files, wlogv1::record &output){
    for (const auto &info : files){
        if (globalRow < info.firstRow || globalRow - info.firstRow >= info.recordCount){
            continue;
        }
        uint64_t localRow = globalRow - info.firstRow;
        uint64_t offset = sizeof(wlogv1::header) + localRow * sizeof(wlogv1::record);

        QFile file (info.path);

        if (!file.open(QIODevice::ReadOnly)){
            return false;
        }
        if (offset > static_cast<uint64_t>(file.size()) || sizeof(output) > static_cast<uint64_t>(file.size()) - offset){
            return false;
        }
        if (!file.seek (static_cast<qint64>(offset))){
            return false;
        }
        if (file.read(reinterpret_cast<char*>(&output), sizeof(output)) != sizeof(output)){
            return false;
        }
        return true;
    }
    return false;
}

bool openWlog(const QString &path, wlogFileInfo &info){
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) { 
        return false; 
    }
    if (file.size() < static_cast<qint64>(sizeof(wlogv1::header))) { 
        return false; 
    }

    wlogv1::header header; //header struc type whatnot
    qint64 headerSize = sizeof(header);

    if (file.read(reinterpret_cast<char*>(&header), headerSize) != headerSize) { 
        return false; 
    }
    if (memcmp(header.magic, "WLOG", 4) != 0) { 
        return false; 
    }
    if (header.version != 1) {
        return false;
    }
    if (header.recordSize != sizeof(wlogv1::record)){
        return false;
    }

    uint64_t dataSize = file.size() - sizeof(wlogv1::header);

    if (dataSize % header.recordSize != 0) {
        return false;
    }

    info.path = path;
    info.version = header.version;
    info.recordSize = header.recordSize;
    info.recordCount = dataSize / header.recordSize;
    info.firstTimestamp = 0;

    if (info.recordCount > 0) {
        wlogv1::record first;

        qint64 firstRecordSize = sizeof(first);
        if (file.read(reinterpret_cast<char*>(&first), firstRecordSize) != firstRecordSize) {
            return false;
        }

        info.firstTimestamp = first.timestamp;
    }

    return true;
};

void buildFileIndex(const QStringList &selectedFiles, std::vector<wlogFileInfo> &files){
    files.clear();

    for (const QString &path : selectedFiles) {
        wlogFileInfo info;

        if (openWlog(path, info)) {
            files.push_back(info);
        }
    }

    std::sort(files.begin(), files.end(), [](const wlogFileInfo &a, const wlogFileInfo &b){
        return a.firstTimestamp < b.firstTimestamp;
    });

    uint64_t nextRow = 0;

    for (wlogFileInfo &file : files) {
        file.firstRow = nextRow;
        nextRow += file.recordCount;
    }
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

void drawDataTableTab(QWidget *dataTableTab, const std::vector<wlogFileInfo> &files, QMenu *viewMenu, QMenu *recordsMenu, QToolBar *toolBar, QTabWidget *tabWidget){
    QVBoxLayout *layout = new QVBoxLayout(dataTableTab);

    QTableView *table = new QTableView(dataTableTab);
    WLogTableModel *model = new WLogTableModel(files, table);

    table->setModel(model);

    FileHeader *fileHeader = new FileHeader(table);
    table->setVerticalHeader(fileHeader);
    table->horizontalHeader()->setResizeContentsPrecision(10);
    table->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    table->verticalHeader()->setFixedWidth(160);

    layout->addWidget(table);

    QAction *autoFitAction = viewMenu->addAction(QIcon("src/icons/AutofitColumns.png"), "Auto&Fit Columns");

    QObject::connect(autoFitAction, &QAction::triggered, [table](){
        table->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    });

    QAction *normalizeAction = viewMenu->addAction(QIcon("src/icons/NormalizeColumns.png"), "&Normalize Columns");

    QObject::connect(normalizeAction, &QAction::triggered, [table](){
        QHeaderView *header = table->horizontalHeader();

        for (int i = 0; i < header->count(); i++){
            header->resizeSection(i, 60);
        }
    });

    QAction *jumpToPreviousFileAction = recordsMenu->addAction(QIcon("src/icons/JumpToPreviousFile.png"), "Jump to &Previous File");
    QAction *jumpToNextFileAction = recordsMenu->addAction(QIcon("src/icons/JumpToNextFile.png"), "Jump to &Next File");

    QObject::connect(jumpToPreviousFileAction, &QAction::triggered, [table, model, &files](){
        int currentRow = table->rowAt(0);
        
        for (int i = static_cast<int>(files.size()) - 1; i >= 0; i--){
            if (files[i].firstRow < static_cast<uint64_t>(currentRow)){
                table->scrollTo(
                    model->index(static_cast<int>(files[i].firstRow), 0),
                    QAbstractItemView::PositionAtTop
                );
                break;
            }
        }
    });

    QObject::connect(jumpToNextFileAction, &QAction::triggered, [table, model, &files](){
        int currentRow = table->rowAt(0);
        
        for (size_t i = 0; i < files.size(); i++){
            if (files[i].firstRow > static_cast<uint64_t>(currentRow)){
                table->scrollTo(
                    model->index(static_cast<int>(files[i].firstRow), 0),
                    QAbstractItemView::PositionAtTop
                );
                break;
            }
        }
    });

    toolBar->addAction(autoFitAction);
    toolBar->addAction(normalizeAction);
    QAction *separator = toolBar->addSeparator();
    toolBar->addAction(jumpToPreviousFileAction);
    toolBar->addAction(jumpToNextFileAction);

    QObject::connect(tabWidget, &QTabWidget::currentChanged, [=](int index){
        autoFitAction->setVisible(index == 0);
        normalizeAction->setVisible(index == 0);
        separator->setVisible(index == 0);
        jumpToPreviousFileAction->setVisible(index == 0);
        jumpToNextFileAction->setVisible(index == 0);
    });
}

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

    QMenu *plotsMenu = menuBar->addMenu("&Plots");
    plotsMenu->setEnabled(false);

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

    drawDataTableTab(dataTableTab, files, viewMenu, recordsMenu, toolBar, tabWidget);

    window.show();

    return app.exec();
}
