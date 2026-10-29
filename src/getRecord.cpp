#pragma once
#include "./getRecord.h"
#include <QFile>

inline bool getRecord(uint64_t globalRow, const std::vector<wlogFileInfo> &files, wlogv1::record &output){
    for (const auto &info : files){
        if (globalRow < info.firstRow || globalRow - info.firstRow >= info.recordCount){
            continue;
        }
        uint64_t localRow = globalRow - info.firstRow;
        uint64_t offset = sizeof(wlogv1::header) + localRow * sizeof(wlogv1::record);

        QFile file (info.path);

        if (!file.open(QIODevice::ReadOnly)){
            return false;
        }
        if (offset > static_cast<uint64_t>(file.size()) || sizeof(output) > static_cast<uint64_t>(file.size()) - offset){
            return false;
        }
        if (!file.seek (static_cast<qint64>(offset))){
            return false;
        }
        if (file.read(reinterpret_cast<char*>(&output), sizeof(output)) != sizeof(output)){
            return false;
        }
        return true;
    }
    return false;
}