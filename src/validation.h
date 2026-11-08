#pragma once
#include <vector>
#include <functional>
#include <cstdint>
#include "wlogCommon.h"
#include "struc.h"

struct activeCheck{
    const wlog::LogFormat *format;
    const wlog::validationCheck *check;
    int column;
    double param;
};

struct validationIssue{
    uint64_t row;
    const wlog::validationCheck *check;
    int column;
};

struct fileValidation{
    const wlogFileInfo *file;
    std::vector<validationIssue> issues;
    bool discardedTail = false; //true if file resync na kaput (v1 records)
    uint64_t discardFromRow = 0;
};

using validationProgress = std::function<bool(size_t fileIndex, uint64_t row)>;

std::vector<fileValidation> validateFiles(const std::vector<wlogFileInfo> &files, const std::vector<activeCheck> &checks, const validationProgress &progress = {});