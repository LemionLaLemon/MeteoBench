#pragma once
#include <QVBoxLayout>
#include <QTableView>
#include <QFile>
#include "fileHeader.h"
#include "wlogTableModel.h"
#include "wlogV1.h"
#include "wlogCommon.h"
#include <QToolBar>
#include <qmenu.h>

void drawDataTableTab(QWidget *dataTableTab, WLogTableModel *model, QMenu *viewMenu, QMenu *recordsMenu, QToolBar *toolBar, QTabWidget *tabWidget, const std::vector <wlogFileInfo> &files, std::vector<QAction*> &actions);