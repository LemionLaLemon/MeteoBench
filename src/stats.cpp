#include "stats.h"

void computeStats(wlogFileInfo &info){
    info.stats.assign(wlogv1::columnCount, columnStats{});

    for (uint64_t row = 0; row < info.recordCount; row++){
        const uchar *rec = info.mapped + info.dataOffset + row * info.recordSize;

        for (size_t c = 0; c < wlogv1::columnCount; c++){
            double v = wlog::readValue(rec, wlogv1::columns[c]);

            if (!std::isfinite(v)){
                continue;
            }
            columnStats &s = info.stats[c];
            s.min = std::min(s.min, v);
            s.max = std::max(s.max, v);
            s.sum += v;
            s.count++;
        }
    }
}

columnStats combineStats(const std::vector<wlogFileInfo> &files, int column){
    columnStats total;

    for (const auto &f : files){
        const columnStats &s = f.stats[column];

        if (s.count == 0){
            continue;
        }
        total.min = std::min(total.min, s.min);
        total.max = std::max(total.max, s.max);
        total.sum += s.sum;
        total.count += s.count;
    }
    return total;
}