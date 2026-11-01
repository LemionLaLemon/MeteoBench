#include "drawPlotsTab.h"
#include "qcustomplot.h"
#include <qnamespace.h>

QCPItemStraightLine *addHLine(QCustomPlot *graph, double y, const QPen &pen, const char *group){
    QCPItemStraightLine *line = new QCPItemStraightLine(graph);
    line->setPen(pen);
    line->point1->setCoords(0, y);
    line->point2->setCoords(1, y);
    line->setProperty("group", group);

    return line;
}

void addStatLines(QCustomPlot *graph, int column, const std::vector<wlogFileInfo> &files){
    columnStats all = combineStats(files, column);

    if (all.count == 0){
        return;
    }

    addHLine(graph, all.min, QPen(Qt::darkRed, 1, Qt::DotLine), "minmax");
    addHLine(graph, all.max, QPen(Qt::darkRed, 1, Qt::DotLine), "minmax");
    addHLine(graph, all.sum / all.count, QPen(Qt::darkGreen, 1, Qt::DashLine), "avgAll");

    for (const auto &f : files){
        const columnStats &s = f.stats[column];

        if (s.count == 0){
            continue;
        }
        double avg = s.sum / s.count;

        QCPItemLine *segment = new QCPItemLine(graph);
        segment->setPen(QPen(Qt::darkMagenta, 1.5));
        segment->start->setCoords(f.stats[0].min, avg);
        segment->end->setCoords(f.stats[0].max, avg);
        segment->setProperty("group", "avgFile");
    }
}

void addFileMarkers(QCustomPlot *graph, const std::vector<wlogFileInfo> &files){
    for (const auto &f : files){
        double start = static_cast<double>(f.firstTimestamp);

        QCPItemStraightLine *line = new QCPItemStraightLine(graph);
        line->setPen(QPen(Qt::gray, 1));
        line->point1->setCoords(start, 0);
        line->point2->setCoords(start, 1);

        QCPItemText *label = new QCPItemText(graph);
        label->setText(QFileInfo(f.path).fileName());
        label->setPositionAlignment(Qt::AlignTop | Qt::AlignLeft);
        label->position->setTypeX(QCPItemPosition::ptPlotCoords);
        label->position->setTypeY(QCPItemPosition::ptAxisRectRatio);
        label->position->setCoords(start, 0);
        label->setPadding(QMargins(3, 2, 3, 2));
        label->setProperty("group", "label");
    }
}

void drawPlotsTab(QWidget *plotsTab, WLogTableModel *model, QMenu *viewMenu, QToolBar *toolBar, QTabWidget *tabWidget, QMenu *plotsMenu, const std::vector <wlogFileInfo> &files, std::vector<QAction*> &actions){
    QVBoxLayout *layout = new QVBoxLayout(plotsTab);

    QSplitter *horizontal = new QSplitter(Qt::Horizontal);
    QSplitter *tables = new QSplitter(Qt::Vertical);
    QSplitter *graphs = new QSplitter(Qt::Vertical);

    horizontal->addWidget(tables);
    horizontal->addWidget(graphs);

    std::vector<PlotTableView*> plotTables;
    std::vector<QCustomPlot*> graphList;
    std::vector<PlotEntry> plots;

    const uint64_t rows = static_cast<uint64_t>(model->rowCount());

    QVector<double> times;
    auto *selection = new QItemSelectionModel(model, plotsTab);

    times.reserve(static_cast<qsizetype>(rows));


    for (uint64_t row = 0; row < rows; row++){
        const uchar *rec = getRecord(row, files);

        if (!rec){
            break;
        }
        times.append(wlog::readValue(rec, wlogv1::columns[0]));
    }

    for (int column = 1; column < model->columnCount(); column++){ 
        QCustomPlot *graph = new QCustomPlot();

        PlotTableView *table = new PlotTableView();
        table->setModel(model);
        table->verticalHeader()->hide();

        QItemSelectionModel *old = table->selectionModel();
        table->setSelectionModel(selection);
        delete old;
        table->setSelectionBehavior(QAbstractItemView::SelectRows);

        QAction *toggle = plotsMenu ->addAction(wlogv1::columns[column].title);
        toggle->setCheckable(true);
        toggle->setChecked(true);

        QObject::connect(toggle, &QAction::toggled, [table, graph](bool on){
            table->setVisible(on);
            graph->setVisible(on);
        });

        for (int c = 1; c < model->columnCount(); c++){
            table->setColumnHidden(c, c != column);
        }
        
        QVector<double> values;
        values.reserve(static_cast<qsizetype>(rows));
        // TODO: calculate visible records and downsample the graph accordingly

        for (uint64_t row = 0; row < rows; row++){
            const uchar *rec = getRecord(row, files);

            if (!rec){
                break;
            }
            values.append(wlog::readValue(rec, wlogv1::columns[column]));
        }

        graph->addGraph();
        QSharedPointer<QCPAxisTickerDateTime> ticker(new QCPAxisTickerDateTime);
        ticker->setDateTimeFormat("MM-dd HH:mm:ss");
        graph->xAxis->setTicker(ticker);
        graph->graph(0)->setAdaptiveSampling(true);
        graph->graph(0)->setData(times, values, true);

        if (!times.isEmpty()){
            graph->xAxis->setRange(times.first(), times.last());
        }

        graph->rescaleAxes();

        addStatLines(graph, column, files);
        addFileMarkers(graph, files);

        graph->replot();

        QCPItemTracer *tracer = new QCPItemTracer(graph);
        tracer->setGraph(graph->graph(0));
        tracer->setInterpolating(false);
        tracer->setStyle(QCPItemTracer::tsCircle);
        tracer->setSize(7);
        graph->axisRect()->setAutoMargins(QCP::msNone);
        graph->axisRect()->setMargins(QMargins(60, 8, 12, 28));

        graph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
        graph->axisRect()->setRangeDrag(Qt::Horizontal);
        graph->axisRect()->setRangeZoom(Qt::Horizontal);

        QCPItemStraightLine *cursor = new QCPItemStraightLine(graph);
        cursor->setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
        cursor->point1->setCoords(0, 0);
        cursor->point2->setCoords(0, 1);
        cursor->setVisible(false);
        tracer->setVisible(false);

        QCPItemStraightLine *selectedRecordCursor = new QCPItemStraightLine(graph);
        selectedRecordCursor->setPen(QPen(QColor("#316ac5"), 1, Qt::DashDotDotLine));
        selectedRecordCursor->point1->setCoords(0, 0);
        selectedRecordCursor->point2->setCoords(0,1);
        selectedRecordCursor->setVisible(true);
        selectedRecordCursor->setProperty("group", "selectedRecordCursor");

        tables->addWidget(table);
        graphs->addWidget(graph);
        table->setMinimumHeight(60);
        graph->setMinimumHeight(60);
        plotTables.push_back(table);
        graphList.push_back(graph);
        plots.push_back({graph, tracer, cursor, selectedRecordCursor, column});
    }
    horizontal->setSizes({250, 750});
    QList<int> equal(model->columnCount() - 1, 1);
    tables->setSizes(equal);
    graphs->setSizes(equal);

    // initial selection
    for (PlotTableView *t : plotTables){
        t->selectRow(0);
        t->scrollTo(t->model()->index(0, 0), QAbstractItemView::PositionAtCenter);
    }

    layout->addWidget(horizontal);

    auto dataColumn = [](QTableView *t) -> int {
        for (int c = 1; c < t->model()->columnCount(); c++){
            if (!t->isColumnHidden(c)){
                return c;
            }
        }
        return -1;
    };

    auto showAt = [plots](double x){
        QString time;
        QString lines;

        for (const PlotEntry &p : plots){
            if (p.graph->isHidden() || p.graph->graph(0)->data()->isEmpty()){
                continue;
            }
            p.tracer->setGraphKey(x);
            p.tracer->updatePosition();

            double ts = p.tracer->position->key();
            double value = p.tracer->position->value();

            p.cursor->point1->setCoords(ts, 0);
            p.cursor->point2->setCoords(ts, 1);
            p.cursor->setVisible(true);
            p.tracer->setVisible(std::isfinite(value));

            if (time.isEmpty()){
                time = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(ts)).toString("yyyy-MM-dd HH:mm:ss");
            }
           lines += QString("\n%1: %2").arg(wlogv1::columns[p.column].title).arg(std::isfinite(value) ? QString::number(value, 'f', 2) : QString("NaN"));

            p.graph->replot(QCustomPlot::rpQueuedReplot);
        }
        return time + lines;
    };

    QLabel *tip = new QLabel(plotsTab);
    tip->setAttribute(Qt::WA_TransparentForMouseEvents);
    tip->setStyleSheet("background-color: rgba(0, 0, 0, 220); color: white; padding: 3px 5px; ");
    tip->hide();

    auto placeTip = [tip, plotsTab](QCustomPlot *from, QPoint pos, const QString &text){
        if (text.isEmpty()){
            return;
        }
        tip->setText(text);
        tip->adjustSize();

        QPoint mapped = from->mapTo(plotsTab, pos);
        QPoint p = mapped + QPoint(14, 14);

        if (p.x() + tip->width() > plotsTab->width()){
            p.setX(mapped.x() - 14 - tip->width());
        }
        if (p.y() + tip->height() > plotsTab->height()){
            p.setY(mapped.y() - 14 - tip->height());
        }
        tip->move(p);
        tip->raise();
        tip->show();
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

    // zoom + pan sync
    for (QCustomPlot *source : graphList){
        QObject::connect(source->xAxis, qOverload<const QCPRange &>(&QCPAxis::rangeChanged), [graphList, source](const QCPRange &range){
            for (QCustomPlot *target : graphList){
                if (target != source){
                    target->xAxis->setRange(range);
                    target->replot(QCustomPlot::rpQueuedReplot);
                }
            }
        });
    }

    // cursor sync AND
    // graph click table select
    for (const PlotEntry &source : plots){
        auto pressPos = std::make_shared<QPoint>();

        QObject::connect(source.graph, &QCustomPlot::mousePress, [pressPos](QMouseEvent *event){
            *pressPos = event->pos();
        });
        QObject::connect(source.graph, &QCustomPlot::mouseRelease, [=](QMouseEvent *event){
            if (event->button() != Qt::LeftButton || times.isEmpty()){
                return;
            }
            if ((event->pos() - *pressPos).manhattanLength() > 4){
                return;
            }
            if (!source.graph->axisRect()->rect().contains(event->pos())){
                return;
            }

            double x = source.graph->xAxis->pixelToCoord(event->pos().x());

            auto it = std::lower_bound(times.begin(), times.end(), x);

            if (it == times.end()){
                --it;
            }
            else if (it != times.begin() && (x - *(it - 1)) < (*it - x)){
                --it;
            }
            int row = static_cast<int>(it - times.begin());

            QModelIndex index = model->index(row, 0);
            selection->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            placeTip(source.graph, event->pos(), showAt(*it));
            plotTables.front()->scrollTo(index, QAbstractItemView::PositionAtCenter);
        });
        QObject::connect(source.graph, &QCustomPlot::mouseMove, [plots, source, showAt, placeTip](QMouseEvent *event){
            if (!source.graph->axisRect()->rect().contains(event->pos())){
                for (const PlotEntry &p : plots){
                    p.cursor->setVisible(false);
                    p.tracer->setVisible(false);
                    p.graph->replot(QCustomPlot::rpQueuedReplot);
                }
                QToolTip::hideText();
                return;
            }
            QString text = showAt(source.graph->xAxis->pixelToCoord(event->pos().x()));
            placeTip(source.graph, event->pos(), text);
        });
    }

    if (!times.isEmpty()){
        showAt(times.first());
    }

    // row changed cursor something
    QObject::connect(selection, &QItemSelectionModel::currentRowChanged, [=](const QModelIndex &current, const QModelIndex &){
        if (!current.isValid() || current.row() >= times.size()){
            return;
        }
        double t = times[current.row()];

        for (const PlotEntry &p : plots){
            p.selectedRecordCursor->point1->setCoords(t, 0);
            p.selectedRecordCursor->point2->setCoords(t, 1);
            p.selectedRecordCursor->setVisible(true);
            p.graph->replot(QCustomPlot::rpQueuedReplot);
        }
    });

    // plots analysis toggles
    plotsMenu->addSeparator();
    auto *overlays = new StayOpenMenu("Graph Overlays", plotsMenu);
    plotsMenu->addMenu(overlays);

    auto addGroupToggle = [&](const QString &title, const char *group){
        QAction *action = overlays->addAction(title);
        action->setCheckable(true);
        action->setChecked(true);

        QObject::connect(action, &QAction::toggled, [graphList, group](bool on){
            for (QCustomPlot *g : graphList){
                for (int i = 0; i < g->itemCount(); i++){
                    QCPAbstractItem *item = g->item(i);

                    if (item->property("group").toString() == group){
                        item->setVisible(on);
                    }
                }
                g->replot(QCustomPlot::rpQueuedReplot);
            }
        });
    };
    
    addGroupToggle("Show Min/Max", "minmax");
    addGroupToggle("Show Average (All)", "avgAll");
    addGroupToggle("Show Average (By File)", "avgFile");
    addGroupToggle("Show File Labels", "label");
    addGroupToggle("Show Selected Record", "selectedRecordCursor");

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
    actions.push_back(autoFitAction);
    QAction *normalizeAction = viewMenu->addAction(QIcon("src/icons/NormalizeColumns.png"), "&Normalize Columns");
    actions.push_back(normalizeAction);
    QAction *resetAction = viewMenu->addAction(QIcon("src/icons/ResetLayout.png"), "&Reset Layout");
    actions.push_back(resetAction);

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
    actions.push_back(separator);
}