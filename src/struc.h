#pragma once
#include <cstdint>
#include <QString>

struct wlogFileInfo {
    QString path;
    uint16_t version;
    uint16_t recordSize;
    uint64_t recordCount;
    uint64_t firstTimestamp;
    uint64_t firstRow;
};
