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

#include "./wlogV1.h"
#include "./wlogTableModel.h"
#include "./struc.h"
#include "./fileHeader.h"
#include "getRecord.h"

std::vector<wlogFileInfo> files;

QStringList selectedFiles = {
    "/home/lemion/Documents/Lemion Industries Meteobench/samplecontent/2026-07-31.wlog",
    "/home/lemion/Documents/Lemion Industries Meteobench/samplecontent/2026-08-01.wlog"
};

bool openWlog(const QString &path, wlogFileInfo &info){
    info.file = std::make_unique<QFile>(path);
    info.dataOffset = sizeof(wlogv1::header);
    QFile &file = *info.file;

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

    info.mapped = file.map(0, file.size());
    if (!info.mapped){
        return false;
    }

    return true;
};

void buildFileIndex(const QStringList &selectedFiles, std::vector<wlogFileInfo> &files){
    files.clear();

    for (const QString &path : selectedFiles) {
        wlogFileInfo info;

        if (openWlog(path, info)) {
            files.push_back(std::move(info));
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

void drawDataTableTab(QWidget *dataTableTab, WLogTableModel *model, QMenu *viewMenu, QMenu *recordsMenu, QToolBar *toolBar, QTabWidget *tabWidget){
    QVBoxLayout *layout = new QVBoxLayout(dataTableTab);

    QTableView *table = new QTableView(dataTableTab);

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

    QObject::connect(jumpToPreviousFileAction, &QAction::triggered, [table, model](){
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

    QObject::connect(jumpToNextFileAction, &QAction::triggered, [table, model](){
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
class PlotTableView : public QTableView {
    public:
        using QTableView::QTableView;
        using QTableView::sizeHintForColumn;
};
void drawPlotsTab(QWidget *plotsTab, WLogTableModel *model, QMenu *viewMenu, QToolBar *toolBar, QTabWidget *tabWidget){
    QVBoxLayout *layout = new QVBoxLayout(plotsTab);

    QSplitter *horizontal = new QSplitter(Qt::Horizontal);
    QSplitter *tables = new QSplitter(Qt::Vertical);
    QSplitter *graphs = new QSplitter(Qt::Vertical);

    horizontal->addWidget(tables);
    horizontal->addWidget(graphs);

    std::vector<PlotTableView*> plotTables;

    for (int column = 1; column < model->columnCount(); column++){ 
        PlotTableView *table = new PlotTableView();
        table->setModel(model);
        table->verticalHeader()->hide();

        for (int c = 1; c < model->columnCount(); c++){
            table->setColumnHidden(c, c != column);
        }
        QCustomPlot *graph = new QCustomPlot();
        // TODO: calculate visible records and downsample the graph accordingly

        QVector<double> times;
        QVector<double> values;

        for (uint64_t row = 0; row < 1000; row++){
            const uchar *rec = getRecord(row, files);

            if (!rec){
                break;
            }
            times.append(wlog::readValue(rec, wlogv1::columns[0]));
            values.append(wlog::readValue(rec, wlogv1::columns[column]));
        }

        graph->addGraph();
        QSharedPointer<QCPAxisTickerDateTime> ticker(new QCPAxisTickerDateTime);
        ticker->setDateTimeFormat("MM-dd HH:mm:ss");
        graph->xAxis->setTicker(ticker);
        graph->graph(0)->setAdaptiveSampling(true);
        graph->graph(0)->setData(times, values);
        graph->rescaleAxes();
        graph->replot();

        QCPItemTracer *tracer = new QCPItemTracer(graph);
        tracer->setGraph(graph->graph(0));
        tracer->setInterpolating(false);
        tracer->setStyle(QCPItemTracer::tsCircle);
        tracer->setSize(7);

        QCPItemStraightLine *cursor = new QCPItemStraightLine(graph);
        cursor->setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
        cursor->point1->setCoords(0, 0);
        cursor->point2->setCoords(0, 1);
        cursor->setVisible(false);
        tracer->setVisible(false);

        QObject::connect(graph, &QCustomPlot::mouseMove, [graph, tracer, cursor](QMouseEvent *event){
            if (!graph->axisRect()->rect().contains(event->pos())){
                cursor->setVisible(false);
                tracer->setVisible(false);
                QToolTip::hideText();
                graph->replot(QCustomPlot::rpQueuedReplot);
                return;
            }

            double x = graph->xAxis->pixelToCoord(event->pos().x());

            if (graph->graph(0)->data()->isEmpty() || !std::isfinite(graph->yAxis->range().lower) || !std::isfinite(graph->yAxis->range().upper)){
                return;
            }
            tracer->setGraphKey(x);
            tracer->updatePosition();

            double timestamp = tracer->position->key();
            double value = tracer->position->value();

            if (!std::isfinite(value)){
                cursor->setVisible(false);
                tracer->setVisible(false);
                QToolTip::hideText();
                graph->replot(QCustomPlot::rpQueuedReplot);
                return;
            }

            cursor->point1->setCoords(timestamp, 0);
            cursor->point2->setCoords(timestamp, 1);

            cursor->setVisible(true);
            tracer->setVisible(true);

            QString time = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(timestamp)).toString("yyyy-MM-dd HH:mm:ss");
            QToolTip::showText(event->globalPosition().toPoint(), QString("%1:\n %2").arg(time).arg(value, 0, 'f', 2), graph);
            graph->replot(QCustomPlot::rpQueuedReplot);
        });

        tables->addWidget(table);
        graphs->addWidget(graph);
        table->setMinimumHeight(60);
        graph->setMinimumHeight(60);
        plotTables.push_back(table);
    }
    horizontal->setSizes({250, 750});
    QList<int> equal(model->columnCount() - 1, 1);
    tables->setSizes(equal);
    graphs->setSizes(equal);

    layout->addWidget(horizontal);

    auto dataColumn = [](QTableView *t) -> int {
        for (int c = 1; c < t->model()->columnCount(); c++){
            if (!t->isColumnHidden(c)){
                return c;
            }
        }
        return -1;
    };

    // table scroll sync
    for (QTableView *source : plotTables){
        QObject::connect(source->verticalScrollBar(), &QScrollBar::valueChanged, [plotTables, source](int value){
            for (QTableView *target : plotTables){
                if (target != source){
                target->verticalScrollBar()->setValue(value);
                }
            }
        });
        QObject::connect(source->horizontalHeader(), &QHeaderView::sectionResized, [plotTables, source, dataColumn](int column, int, int newWidth){
            if (source->isColumnHidden(column)){
                return;
            }
            const bool isTime = (column == 0);
            for (QTableView *target : plotTables){
                if (target == source){
                    continue;
                }
                int targetColumn = isTime ? 0 : dataColumn(target);
                if (targetColumn < 0){
                    continue;
                }
                if (target->columnWidth(targetColumn) != newWidth){
                    target->setColumnWidth(targetColumn, newWidth);
                }
            }
        });
    }

    // table graph vsync
    QObject::connect(tables, &QSplitter::splitterMoved, [tables, graphs](int, int){
        graphs->setSizes(tables->sizes());
    });
    QObject::connect(graphs, &QSplitter::splitterMoved, [tables, graphs](int, int){
        tables->setSizes(graphs->sizes());
    });

    // actions
    auto applyWidths = [plotTables, dataColumn](int timeWidth, int dataWidth){
        for (QTableView *t : plotTables){
            t->setColumnWidth(0, timeWidth);
            int dc = dataColumn(t);
            if (dc >= 0){
                t->setColumnWidth(dc, dataWidth);
            }
        }
    };

    QAction *autoFitAction = viewMenu->addAction(QIcon("src/icons/AutofitColumns.png"), "Auto&Fit Columns");
    QAction *normalizeAction = viewMenu->addAction(QIcon("src/icons/NormalizeColumns.png"), "&Normalize Columns");
    QAction *resetAction = viewMenu->addAction(QIcon("src/icons/ResetLayout.png"), "&Reset Layout");

    QObject::connect(autoFitAction, &QAction::triggered, [plotTables, dataColumn, applyWidths](){
        int timeWidth = 0;
        int dataWidth = 0;

        for (PlotTableView *t : plotTables){
            QHeaderView *header = t->horizontalHeader();

            timeWidth = std::max({timeWidth, t->sizeHintForColumn(0), header->sectionSizeHint(0)});
            int dc = dataColumn(t);
            if (dc >= 0){
                dataWidth = std::max({dataWidth, t->sizeHintForColumn(dc), header->sectionSizeHint(dc)});
            }
        }
        applyWidths(timeWidth, dataWidth);
    });

    QObject::connect(normalizeAction, &QAction::triggered, [applyWidths](){
        applyWidths(60, 60);
    });

    QObject::connect(resetAction, &QAction::triggered, [plotTables, applyWidths, horizontal, tables, graphs](){
        int w = plotTables.front()->horizontalHeader()->defaultSectionSize();
        applyWidths(w, w);
        horizontal->setSizes({250, 750});
        tables->setSizes({1, 1, 1});
        graphs->setSizes({1, 1, 1});
    });

    toolBar->addAction(autoFitAction);
    toolBar->addAction(normalizeAction);
    toolBar->addAction(resetAction);
    QAction *separator = toolBar->addSeparator();

    QObject::connect(tabWidget, &QTabWidget::currentChanged, [=](int index){
        autoFitAction->setVisible(index == 1);
        normalizeAction->setVisible(index == 1);
        resetAction->setVisible(index == 1);
        separator->setVisible(index == 1);
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

    WLogTableModel *model = new WLogTableModel(files, &window);
    drawDataTableTab(dataTableTab, model, viewMenu, recordsMenu, toolBar, tabWidget);
    drawPlotsTab(plotsTab, model, viewMenu, toolBar, tabWidget);

    window.show();

    return app.exec();
}
