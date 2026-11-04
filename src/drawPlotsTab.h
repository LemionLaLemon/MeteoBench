#pragma once
#include <QWidget>
#include "struc.h"
#include "wlogTableModel.h"
#include <QToolBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QSplitter>
#include <vector>
#include "qcustomplot.h"
#include "stats.h"
#include "getRecord.h"
#include <memory>
#include <algorithm>
#include "formats.h"

class PlotTableView : public QTableView {
    public:
        using QTableView::QTableView;
        using QTableView::sizeHintForColumn;
};
struct PlotEntry {
    QCustomPlot *graph;
    QCPItemTracer *tracer;
    QCPItemStraightLine *cursor;
    QCPItemStraightLine *selectedRecordCursor;
    int column;
};
class StayOpenMenu : public QMenu{
    public:
        using QMenu::QMenu;
    protected:
        void mouseReleaseEvent(QMouseEvent *event) override{
            QAction *action = activeAction();

            if (action && action->isCheckable() && action->isEnabled()){
                action->trigger();
                return;
            }
            QMenu::mouseReleaseEvent(event);
        }
};

void drawPlotsTab(QWidget *plotsTab, WLogTableModel *model, QMenu *viewMenu, QToolBar *toolBar, QTabWidget *tabWidget, QMenu *plotsMenu, const std::vector <wlogFileInfo> &files, std::vector<QAction*> &actions);