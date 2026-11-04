#pragma once
#include <cstdint>
#include <QFile>
#include "./struc.h"
#include <vector>

const uchar *getRecord(uint64_t globalRow, const std::vector<wlogFileInfo> &files);
int findFile(uint64_t globalRow, const std::vector<wlogFileInfo> &files);
void buildMaster(std::vector<wlogFileInfo> &files, std::vector<MasterColumn> &master);
bool readCell(uint64_t row, int masterCol, const std::vector<wlogFileInfo> &files, double &out);