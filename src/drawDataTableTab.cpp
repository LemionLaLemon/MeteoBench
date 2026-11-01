#include "drawDataTableTab.h"
#include <qtabwidget.h>

void drawDataTableTab(QWidget *dataTableTab, WLogTableModel *model, QMenu *viewMenu, QMenu *recordsMenu, QToolBar *toolBar, QTabWidget *tabWidget, const std::vector <wlogFileInfo> &files, std::vector<QAction*> &actions){
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
    actions.push_back(autoFitAction);

    QObject::connect(autoFitAction, &QAction::triggered, [table](){
        table->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    });

    QAction *normalizeAction = viewMenu->addAction(QIcon("src/icons/NormalizeColumns.png"), "&Normalize Columns");
    actions.push_back(normalizeAction);

    QObject::connect(normalizeAction, &QAction::triggered, [table](){
        QHeaderView *header = table->horizontalHeader();

        for (int i = 0; i < header->count(); i++){
            header->resizeSection(i, 60);
        }
    });

    QAction *jumpToPreviousFileAction = recordsMenu->addAction(QIcon("src/icons/JumpToPreviousFile.png"), "Jump to &Previous File");
    actions.push_back(jumpToPreviousFileAction);
    QAction *jumpToNextFileAction = recordsMenu->addAction(QIcon("src/icons/JumpToNextFile.png"), "Jump to &Next File");
    actions.push_back(jumpToNextFileAction);

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
    actions.push_back(separator);
    toolBar->addAction(jumpToPreviousFileAction);
    toolBar->addAction(jumpToNextFileAction);
}