#include "./getRecord.h"
#include "struc.h"
#include <QFile>
#include <algorithm>

const uchar *getRecord(uint64_t globalRow, const std::vector<wlogFileInfo> &files){
    int i = findFile(globalRow, files);

    if (i < 0){
        return nullptr;
    }

    const auto &info = files[i];
    uint64_t localRow = globalRow - info.firstRow;
    return info.mapped + info.dataOffset + localRow * info.recordSize;
}

int findFile(uint64_t globalRow, const std::vector<wlogFileInfo> &files){
    auto it = std::upper_bound(files.begin(), files.end(), globalRow, [](uint64_t row, const wlogFileInfo &f){
        return row < f.firstRow;
    });

    if (it == files.begin()){
        return -1;
    }
    --it;

    if (globalRow - it->firstRow >= it->recordCount){
        return -1;
    }
    return static_cast<int>(it - files.begin());
}