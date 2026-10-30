#pragma once
#include <cstdint>
#include <QFile>
#include "./struc.h"

const uchar *getRecord(uint64_t globalRow, const std::vector<wlogFileInfo> &files);
int findFile(uint64_t globalRow, const std::vector<wlogFileInfo> &files);