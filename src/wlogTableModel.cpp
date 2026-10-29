#include "wlogTableModel.h"
#include "struc.h"
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
    if (orientation == Qt::Vertical && role == Qt::BackgroundRole) {
        for (size_t i=0; i< files.size(); i++){
            const auto &file = files[i];

            if (static_cast<uint64_t>(section) >= file.firstRow && static_cast<uint64_t>(section) - file.firstRow < file.recordCount) {
                static const QColor colors[] = {
                    QColor("#ff4c4c"),
                    QColor("#5ce48c"),
                    QColor("#5f9aff"),
                    QColor("#ffd75f")
                };
                return colors[i % 4];
            }
        }
    }

    if (orientation == Qt::Vertical && role == Qt::UserRole){
        for (const auto &file : files){
            uint64_t row = static_cast<uint64_t>(section);

            if (row >= file .firstRow && row - file.firstRow < file.recordCount){
                return QFileInfo(file.path).fileName();
            }
        }
        return {};
    }
    
    if (role != Qt::DisplayRole){
        return {};
    }

    if (orientation == Qt::Vertical){
        for (const auto &file : files){
            if (static_cast<uint64_t>(section) >= file.firstRow && static_cast<uint64_t>(section) - file.firstRow < file.recordCount) {
                if (static_cast<uint64_t>(section) == file.firstRow){
                    return QFileInfo(file.path).fileName();
                }
                return QString("\""); // artifact from ancient MeteoBench
            }
        }
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