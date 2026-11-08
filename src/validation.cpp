#include "validation.h"
#include "struc.h"

std::vector<fileValidation> validateFiles(const std::vector<wlogFileInfo> &files, const std::vector<activeCheck> &checks, const validationProgress &progress){
    std::vector<fileValidation> results;
    results.reserve(files.size());

    for (size_t fi = 0; fi < files.size(); fi++){
        const wlogFileInfo &file = files[fi];
        const wlog::LogFormat &fmt = *file.format;

        std::vector<const activeCheck*> mine;
        for (const activeCheck &a : checks){
            if (a.format == &fmt){
                mine.push_back(&a);
            }
        }

        fileValidation out;
        out.file = &file;

        for (uint64_t row = 0; row < file.recordCount; row++){
            if (progress && (row % 4096) == 0 && !progress(fi, row)){
                results.push_back(std::move(out));
                return results; //someone decided to cancel validation :<
            }

            const uint8_t *rec = file.mapped + file.dataOffset + row * file.recordSize;
            bool failed = false;

            for (const activeCheck *a : mine){
                const wlog::column *col = a->column >= 0 ? &fmt.columns[a->column] : nullptr;
                if (!a->check->run(rec, col, a->param)){
                    out.issues.push_back({row, a->check, a->column});
                    failed = true;
                }
            }

            if (failed && !fmt.resyncable){
                out.discardedTail = true;
                out.discardFromRow = row;
            }
        }
        results.push_back(std::move(out));
    }
    return results;
}