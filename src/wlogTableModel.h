#pragma once

#include <QAbstractTableModel>
#include <qabstractitemmodel.h>
#include <qnamespace.h>
#include <qvariant.h>
#include <vector>

#include "./struc.h"

class WLogTableModel : public QAbstractTableModel{
    public:
        explicit WLogTableModel(
            const std::vector<wlogFileInfo> &files,
            QObject *parent = nullptr
        );
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant headerData(
            int section,
            Qt::Orientation orientation,
            int role = Qt::DisplayRole
        ) const override;
        QVariant data(
            const QModelIndex &index,
            int role = Qt::DisplayRole
        ) const override;
    private:
        const std::vector<wlogFileInfo> &files;
};
