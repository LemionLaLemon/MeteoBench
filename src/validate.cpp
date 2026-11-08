#include "validate.h"
#include "struc.h"
#include "validation.h"
#include "wlogCommon.h"
#include <QtCore>
#include <algorithm>
#include <qabstractitemview.h>
#include <qboxlayout.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qdialogbuttonbox.h>
#include <qevent.h>
#include <qfileinfo.h>
#include <qformlayout.h>
#include <qgroupbox.h>
#include <qheaderview.h>
#include <qlabel.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qoverload.h>
#include <qprogressdialog.h>
#include <qpushbutton.h>
#include <qscrollarea.h>
#include <qscrollbar.h>
#include <qspinbox.h>
#include <qtablewidget.h>
#include <qwidget.h>
#include <QMessageBox>
#include <set>
#include <map>

struct checkWidget{
    const wlog::LogFormat *format;
    const wlog::validationCheck *check;
    int column;
    QCheckBox *box;
    QDoubleSpinBox *spin;
};

static void addCheckRow(QFormLayout *form, std::vector<checkWidget> &out, const wlog::LogFormat *fmt, const wlog::validationCheck &chk, int column){
    auto *box = new QCheckBox(chk.title);
    box->setToolTip(chk.description);

    QDoubleSpinBox *spin = nullptr;
    if (chk.param == wlog::checkParam::number){
        spin = new QDoubleSpinBox();
        spin->setRange(-1e9, 1e9);

        if (chk.defaultParam && column >= 0){
            double def = chk.defaultParam(&fmt->columns[column]);
            if (!std::isnan(def)){
                spin->setValue(def);
            }
        }

        spin->setEnabled(true);
        QObject::connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), box, [box](){
            box->setChecked(true);
        });
        form->addRow(box, spin);
    }
    else {
        form->addRow(box);
    }
    out.push_back({fmt, &chk, column, box, spin});
}

static QGroupBox *buildFormatGroup(const wlog::LogFormat *fmt, std::vector<checkWidget> &out){
    QGroupBox *group = new QGroupBox(QString("V%1 record options").arg(fmt->version));
    auto *layout = new QVBoxLayout(group);

    for (size_t col = 0; col < fmt->columnCount; col++){
        if (fmt->columns[col].format != wlog::displayFormat::number) {
            continue;
        }

        auto *sub = new QGroupBox(QString("%1 (%2)").arg(QString(fmt->columns[col].title), QString(fmt->columns[col].unit)));
        auto *form = new QFormLayout(sub);
        
        for (size_t i = 0; i < wlog::numberCheckCount; i++){
            addCheckRow(form, out, fmt, wlog::numberChecks[i], static_cast<int>(col));
        }
        layout->addWidget(sub);
    }

    std::set<std::string> seen;
    std::map<std::string, QFormLayout*> groupForms;

    for (size_t i = 0; i < fmt->checkCount; i++){
        const wlog::validationCheck &check = fmt->checks[i];
        
        QFormLayout *&form = groupForms[check.group];
        if (!form){
            auto *sub = new QGroupBox(check.group);
            form = new QFormLayout(sub);
            layout->addWidget(sub);
        }
        addCheckRow(form, out, fmt, check, -1);
    }
    return group;
}

static std::vector<activeCheck> collect(const std::vector<checkWidget> &widgets){
    std::vector<activeCheck> active;
    for (const auto &w : widgets){
        if (w.box->isChecked()){
            active.push_back({w.format, w.check, w.column, w.spin ? w.spin->value() : 0.0});
        }
    }
    return active;
}

static bool deleteRecords(const wlogFileInfo &file, const std::set<uint64_t> &rowsToDelete, QString &error){
    QFile in(file.path);
    if (!in.open(QIODevice::ReadOnly)){
        error = in.errorString();
        return false;
    }

    QFile::remove(file.path + ".bak"); //delete previous backups
    QFile::copy(file.path, file.path + ".bak"); //back up the file

    QSaveFile out(file.path);
    if (!out.open(QIODevice::WriteOnly)){
        error = out.errorString();
        return false;
    }

    out.write(in.read(file.dataOffset));

    const qint64 size = file.recordSize;
    uint64_t row = 0;
    while (!in.atEnd()){
        QByteArray block = in.read(size * 4096);
        qint64 pos = 0;
        for (; pos + size <= block.size(); pos += size, row++){
            if (!rowsToDelete.count(row)){
                out.write(block.constData() + pos, size);
            }
        }
        if (pos < block.size()){
            out.write(block.constData() + pos, block.size() - pos);
        }
    }
    if (!out.commit()){
        error = out.errorString();
        return false;
    }
    return true;
}

void drawValidationWindow(QWidget *parent, const std::vector<wlogFileInfo> &files, const std::function<void()> &onFilesChanged){
    QDialog dialog(parent);
    dialog.setWindowTitle("Validation");
    dialog.resize(800, 600);

    QVBoxLayout *mainLayoutOfLies = new QVBoxLayout(&dialog);

    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayoutOfLies->addLayout(mainLayout);

    QVBoxLayout *validationSelection = new QVBoxLayout();
    mainLayout->addLayout(validationSelection, 6); //4:6 split with affectedRows

    //validation options selection
    QScrollArea *selectionScrollArea = new QScrollArea();
    selectionScrollArea->setWidgetResizable(true);
    validationSelection->addWidget(selectionScrollArea);

    QWidget *selectionScrollAreaContent = new QWidget();
    QVBoxLayout *selectionContentLayout = new QVBoxLayout(selectionScrollAreaContent); 
    selectionScrollArea->setWidget(selectionScrollAreaContent);
    
    std::vector<const wlog::LogFormat*> openFormats;
    for (const auto &file : files){
        if (std::find(openFormats.begin(), openFormats.end(), file.format) == openFormats.end()){
            openFormats.push_back(file.format);
        }
    }

    struct valueColumn{ 
        const char *id; 
        QString header; 
    };
    std::vector<valueColumn> affectedValueColumns;

    for (const wlog::LogFormat *format : openFormats){
        for (size_t c = 0; c < format->columnCount; c++) {
            const wlog::column &column = format->columns[c];
            if (column.format != wlog::displayFormat::number){
                continue;
            }
            bool known = false;
            for (const valueColumn &v : affectedValueColumns){
                if (strcmp(v.id, column.id) == 0){
                    known = true;
                    break;
                }
            }
            if (!known){
                affectedValueColumns.push_back({column.id, QString("%1 (%2)").arg(QString(column.title), QString(column.unit))});
            }
        }
    }

    selectionContentLayout->setSpacing(25);

    std::vector<checkWidget> selectionWidgets;
    for (const wlog::LogFormat *format : openFormats){
        selectionContentLayout->addWidget(buildFormatGroup(format, selectionWidgets));
    }
    selectionContentLayout->addStretch();

    //affected rows
    QVBoxLayout *affectedRows = new QVBoxLayout();
    mainLayout->addLayout(affectedRows, 7);

    QTableWidget *affectedTable = new QTableWidget(0, 4 + static_cast<int>(affectedValueColumns.size()));
    affectedTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    QStringList affectedHeaders = {"", "File", "Time", "Issue"};
    affectedTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    QHeaderView *affectedHeader = affectedTable->horizontalHeader();
    affectedHeader->setSectionResizeMode(0, QHeaderView::Fixed);
    affectedHeader->setMinimumSectionSize(10);
    affectedTable->setColumnWidth(0, affectedTable->style()->pixelMetric(QStyle::PM_IndicatorWidth) + 8);
    affectedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    affectedTable->verticalHeader()->hide();
    affectedTable->horizontalHeader()->setStretchLastSection(true);
    
    QCheckBox *selectAllBox = new QCheckBox(affectedHeader);

    auto placeSelectAll = [=](){
        QSize  boxSize = selectAllBox->sizeHint();
        selectAllBox->move(affectedHeader->sectionViewportPosition(0) + (affectedHeader->sectionSize(0) - boxSize.width()) / 2, (affectedHeader->height() - boxSize.height()) / 2);
    };
    placeSelectAll();
    QObject::connect(affectedHeader, &QHeaderView::sectionResized, selectAllBox, placeSelectAll);
    QObject::connect(affectedHeader, &QHeaderView::geometriesChanged, selectAllBox, placeSelectAll);
    QObject::connect(affectedTable->horizontalScrollBar(), &QScrollBar::valueChanged, selectAllBox, placeSelectAll);

    QObject::connect(selectAllBox, &QCheckBox::clicked, affectedTable, [=](bool on){
        QSignalBlocker blocker(affectedTable);
        for (int r = 0; r < affectedTable->rowCount(); r++){
            affectedTable->item(r, 0)->setCheckState(on ?Qt::Checked : Qt::Unchecked);
        }
    });

    QObject::connect(affectedTable, &QTableWidget::itemChanged, selectAllBox, [=](QTableWidgetItem *item){
        if (item->column() != 0){
            return;
        }
        bool all = affectedTable->rowCount() > 0;
        for (int r = 0; r < affectedTable->rowCount() && all; r++){
            all = affectedTable->item(r, 0)->checkState() == Qt::Checked;
        }
        QSignalBlocker blocker(selectAllBox);
        selectAllBox->setChecked(all);
    });

    for (const valueColumn &v : affectedValueColumns){
        affectedHeaders << v.header;
    }
    affectedTable->setHorizontalHeaderLabels(affectedHeaders);
    
    affectedRows->addWidget(affectedTable);

    QPushButton *deleteSelected = new QPushButton("Back Up and Delete Selected Records");
    deleteSelected->setStyleSheet("QPushButton {border: 1px solid red;} QPushButton:hover {background-color: rgba(255, 0, 0, 40);} QPushButton:pressed { background-color: rgba(255, 0, 0, 90);}");
    affectedRows->addWidget(deleteSelected);

    //dialog buttons
    QDialogButtonBox *buttons = new QDialogButtonBox();

    QPushButton *validateButton = buttons->addButton("Validate", QDialogButtonBox::ActionRole);
    buttons->addButton(QDialogButtonBox::Close);

    QObject::connect(validateButton, &QPushButton::clicked, &dialog, [&](){
        const std::vector<activeCheck> activeChecks = collect(selectionWidgets);
        uint64_t totalRecords = 0;
        std::vector<uint64_t> fileStart;
        for (const auto &file : files){
            fileStart.push_back(totalRecords);
            totalRecords += file.recordCount;
        }

        std::atomic<uint64_t> recordsDone{0};
        std::atomic<size_t> currentFile{0};
        std::atomic<bool> cancelRequested{false};

        QProgressDialog progress("", "Cancel", 0, 1000, &dialog);
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(300);
        progress.setAutoClose(false);
        progress.setAutoReset(false);
        QObject::connect(&progress, &QProgressDialog::canceled, [&](){
            cancelRequested = true;
        });

        std::future<std::vector<fileValidation>> job = std::async(std::launch::async, [&](){
            return validateFiles(files, activeChecks, [&](size_t fileIndex, uint64_t row){
                currentFile = fileIndex;
                recordsDone = fileStart[fileIndex] + row;
                return !cancelRequested.load();
            });
        });

        QEventLoop waitLoop;
        QTimer poll;
        QObject::connect(&poll, &QTimer::timeout, [&](){
            if (!cancelRequested.load()){
                progress.setLabelText(QString("<table><tr><td><img src='src/icons/Clock.png'</td><td style='padding-left: 8px;'>Validating files...<br>%1</td></tr></table>").arg(QFileInfo(files[currentFile.load()].path).fileName()));
                progress.setValue(static_cast<int>(recordsDone.load() * 1000 / std::max<uint64_t>(totalRecords, 1)));
            }
            if (job.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready){
                waitLoop.quit();
            }
        });
        poll.start(50);
        waitLoop.exec();
        poll.stop();

        const std::vector<fileValidation> fileResults = job.get();
        progress.reset();
        progress.hide();

        size_t totalIssues = 0;
        for (const fileValidation &fileResult : fileResults){
            totalIssues += fileResult.issues.size();
        }

        QSignalBlocker fillBocker(affectedTable);
        affectedTable->setUpdatesEnabled(false);
        for (int s = 1; s < affectedTable->columnCount(); s++){
            affectedHeader->setSectionResizeMode(s, QHeaderView::Interactive);
        }
        affectedTable->setRowCount(static_cast<int>(totalIssues));
        int tableRow = 0;

        for (const fileValidation &fileResult : fileResults){
            const wlogFileInfo &file = *fileResult.file;
            const wlog::LogFormat &format = *file.format;
            const QString fileName = QFileInfo(file.path).fileName();

            std::vector<int> valueSource(affectedValueColumns.size(), -1);
            for (size_t v = 0; v < affectedValueColumns.size(); v++){
                for (size_t c = 0; c < format.columnCount; c++){
                    if (strcmp(format.columns[c].id, affectedValueColumns[v].id) == 0){
                        valueSource[v] = static_cast<int>(c);
                    }
                }
            }

            int timeColumn = -1;
            for (size_t c = 0; c < format.columnCount; c++){
                if (format.columns[c].format == wlog::displayFormat::datetime){
                    timeColumn = static_cast<int>(c);
                    break;
                }
            }

            for (const validationIssue &issue : fileResult.issues){
                const uint8_t *rec = file.mapped + file.dataOffset + issue.row * file.recordSize;

                auto *checkItem = new QTableWidgetItem();
                checkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                checkItem->setCheckState(Qt::Checked);
                affectedTable->setItem(tableRow, 0, checkItem);

                const size_t fileIndex = &file - files.data();
                checkItem->setData(Qt::UserRole, static_cast<qulonglong>(fileIndex));
                checkItem->setData(Qt::UserRole + 1, static_cast<qulonglong>(issue.row));

                affectedTable->setItem(tableRow, 1, new QTableWidgetItem(fileName));

                QString timeText;
                if (timeColumn >= 0){
                    double t = wlog::readValue(rec, format.columns[timeColumn]);
                    timeText = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(t)).toString(QString(format.columns[timeColumn].unit));
                }
                affectedTable->setItem(tableRow, 2, new QTableWidgetItem(timeText));

                QString issueText = QString(issue.check->title);
                if (issue.column >= 0){
                    issueText = QString("%1: %2").arg(QString(format.columns[issue.column].title), issueText);
                }
                affectedTable->setItem(tableRow, 3, new QTableWidgetItem(issueText));

                for (size_t v = 0; v < affectedValueColumns.size(); v++){
                    QString valueText;
                    if (valueSource[v] >= 0){
                        valueText = QString::number(wlog::readValue(rec, format.columns[valueSource[v]]), 'f', 2);
                    }
                    affectedTable->setItem(tableRow, 4 + static_cast<int>(v), new QTableWidgetItem(valueText));
                }
                tableRow++;
            }
        }

        for (int s = 1; s < affectedTable->columnCount(); s++){
            affectedHeader->setSectionResizeMode(s, QHeaderView::ResizeToContents);
        }
        affectedTable->setUpdatesEnabled(true);
        selectAllBox->setChecked(affectedTable->rowCount() > 0);

        for (const fileValidation &fileResult : fileResults){
            if (fileResult.stoppedEarly){
                QMessageBox::information(&dialog, "Too many issues", "Stopped after 10,000,000 issues. Delete these records, then validate again to continue.");
                break;
            }
        }
    });

    QObject::connect(deleteSelected, &QPushButton::clicked, &dialog, [&](){
        std::map<size_t, std::set<uint64_t>> toDelete;
        for (int r = 0; r < affectedTable->rowCount(); r++){
            QTableWidgetItem *item = affectedTable->item(r, 0);
            if (item && item->checkState() == Qt::Checked){
                toDelete[item->data(Qt::UserRole).toULongLong()].insert(item->data(Qt::UserRole + 1).toULongLong());
            }
        }
        if (toDelete.empty()){
            return;
        }
        size_t total = 0;
        for (const auto &entry : toDelete){
            total += entry.second.size();
        }
        if (QMessageBox::warning(&dialog, "Delete records", QStringLiteral("Are you sure you want to permanently delete %1 record(s) from %2 file(s)?\n\nA backup of each file will be kept as a .wlog.bak").arg(total).arg(toDelete.size()), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes){
            return;
        }
        QStringList errors;
        for (const auto &entry : toDelete){
            QString error;
            if (!deleteRecords(files[entry.first], entry.second, error)){
                errors << QFileInfo(files[entry.first].path).fileName() + ": " + error;
            }
        }
        onFilesChanged();
        affectedTable->setRowCount(0);
        selectAllBox->setChecked(false);
        if (!errors.isEmpty()){
            QMessageBox::warning(&dialog, "Some files could not be rewritten", errors.join("\n"));
        }
    });

    mainLayout->addStretch();
    mainLayoutOfLies->addWidget(buttons);

    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.exec();
}
