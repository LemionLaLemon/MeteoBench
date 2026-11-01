#pragma once
#include <cstdint>
#include <QString>
#include <memory>
#include <QFile>


struct columnStats {
    double min = INFINITY;
    double max = -INFINITY;
    double sum = 0;
    uint64_t count = 0;
};

struct wlogFileInfo {
    QString path;
    uint16_t version;
    uint16_t recordSize;
    uint64_t recordCount;
    uint64_t firstTimestamp;
    uint64_t firstRow;
    std::unique_ptr<QFile> file;
    const uchar *mapped = nullptr;
    uint64_t dataOffset = 0;
    std::vector<columnStats> stats;
    bool trimmed = false;
};