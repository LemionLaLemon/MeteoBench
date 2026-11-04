#include "wlogTableModel.h"
#include "struc.h"
#include "wlogCommon.h"
#include "wlogV1.h"
#include <qabstractitemmodel.h>
#include <qcolor.h>
#include <qhashfunctions.h>
#include <qnamespace.h>
#include <qvariant.h>
#include "getRecord.h"
#include <QDateTime>
#include <cmath>
#include <QFileInfo>
#include <QColor>

WLogTableModel::WLogTableModel(const std::vector<wlogFileInfo> &fileIndex, const std::vector<MasterColumn> &masterColumns, QObject *parent) : QAbstractTableModel(parent), files(fileIndex), master(masterColumns){
    recount();
}

void WLogTableModel::recount(){
    uint64_t total = 0;

    for (const auto &file : files){
        total += file.recordCount;
    }

    totalRows = static_cast<int>(total);
}

void WLogTableModel::beginReload(){ beginResetModel(); }

void WLogTableModel::endReload(){
    recount();
    endResetModel();
}

int WLogTableModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()){
        return 0;
    }

    return totalRows;
}

int WLogTableModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()){
        return 0;
    }

    return static_cast<int>(master.size());
}

QVariant WLogTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Vertical) {
        int i = findFile(static_cast<uint64_t>(section), files);

        if (i < 0){
            return {};
        }
        const auto &file = files [i];
        
        switch (role){
            case Qt::BackgroundRole: {
                static const QColor colors[] = {
                    QColor("#ff4c4c"),
                    QColor("#5ce48c"),
                    QColor("#5f9aff"),
                    QColor("#ffd75f")
                };
                return colors[i % 4];
            }
            case Qt::UserRole:
                return QFileInfo(file.path).fileName();
            case Qt::DisplayRole:
                if (static_cast<uint64_t>(section) == file.firstRow){
                    return QFileInfo(file.path).fileName();
                }
                return QString("\""); // artifact from ancient MeteoBench
        }
        return {};
    }

    if (role != Qt::DisplayRole){
        return {};
    }

    if (section < 0 || section >= static_cast<int>(master.size())){
        return {};
    }

    return master[section].title;
}

QVariant WLogTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole){
        return {};
    }

    double v;

    if (!readCell(index.row(), index.column(), files, v)){
        return QString("-");
    }

    if (master[index.column()].format == wlog::displayFormat::datetime){
        return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(v)).toString("yyyy-MM-dd HH:mm:ss");
    }

    return std::isfinite(v) ?  QString::number(v, 'f', 2) : QString("-");
}