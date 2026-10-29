#include "wlogTableModel.h"
#include "struc.h"
#include "wlogV1.h"
#include <qabstractitemmodel.h>
#include <qhashfunctions.h>
#include <qnamespace.h>
#include <qvariant.h>
#include "getRecord.h"
#include <QDateTime>
#include <cmath>

WLogTableModel::WLogTableModel(const std::vector<wlogFileInfo> &fileIndex, QObject *parent) : QAbstractTableModel(parent), files(fileIndex) {}

int WLogTableModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()){
        return 0;
    }

    uint64_t total = 0;

    for (const auto &file : files){
        total += file.recordCount;
    }

    return static_cast<int>(total);
}

int WLogTableModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()){
        return 0;
    }

    return 4; // TODO: count the columns
}

QVariant WLogTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole){
        return {};
    }

    if (orientation == Qt::Vertical){
        return section + 1;
    }

    if (section < 0 || section >= wlogv1::columnCount){
        return {};
    }

    return wlogv1::columns[section].title;
}

QVariant WLogTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole){
        return {};
    }

    wlogv1::record record{};

    if (!getRecord(index.row(), files, record)){
        return {};
    }

    switch (index.column()){
        case 0:
            return QDateTime::fromSecsSinceEpoch(record.timestamp).toString("yyyy-MM-dd HH:mm:ss");
        case 1:
            return std::isnan(record.temperature) ? QString("NaN") : QString::number(record.temperature, 'f', 2);
        case 2:
            return std::isnan(record.humidity) ? QString("NaN") : QString::number(record.humidity, 'f', 2);
        case 3:
            return std::isnan(record.pressure) ? QString("NaN") : QString::number(record.pressure, 'f', 2);
    }
    return {};
}