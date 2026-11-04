#include "./getRecord.h"
#include "struc.h"
#include <QFile>
#include <algorithm>
#include <cstring>

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

void buildMaster(std::vector<wlogFileInfo> &files, std::vector<MasterColumn> &master){
    master.clear();

    for (const auto &f : files){
        for (size_t c = 0; c < f.format->columnCount; c++){
            const wlog::column &col = f.format->columns[c];
            bool known = false;

            for (const auto &m : master){
                if (strcmp(m.id, col.id) == 0){
                    known = true;
                    break;
                }
            }

            if (!known){
                master.push_back({col.id, col.title, col.unit, col.format});
            }
        }
    }

    for (auto &f : files){
        f.columnMap.assign(master.size(), -1);

        for (size_t m = 0; m < master.size(); m++){
            for (size_t c = 0; c < f.format->columnCount; c++){
                if (strcmp(master[m].id, f.format->columns[c].id) == 0){
                    f.columnMap[m] = static_cast<int>(c);
                    break;
                }
            }
        }
    }
}

bool readCell(uint64_t row, int masterCol, const std::vector<wlogFileInfo> &files, double &out){
    int i = findFile(row, files);

    if (i < 0){
        return false;
    }
    const wlogFileInfo &f = files[i];

    if (masterCol < 0 || static_cast<size_t>(masterCol) >= f.columnMap.size() || f.columnMap[masterCol] < 0){
        return false;
    }

    const uchar *rec = f.mapped + f.dataOffset + (row - f.firstRow) * f.recordSize;
    out = wlog::readValue(rec, f.format->columns[f.columnMap[masterCol]]);
    return true;
}