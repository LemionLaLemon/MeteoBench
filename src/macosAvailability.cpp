// Cross-compiling for macOS with a Linux clang has no Darwin compiler-rt, which
// normally provides __isPlatformVersionAtLeast (emitted for @available checks in
// Qt's Cocoa plugin). This is the same implementation compiler-rt uses, on top of
// libSystem's _availability_version_check (macOS 10.15+).
#include <cstdint>

extern "C" {
struct dyld_build_version_t {
    uint32_t platform;
    uint32_t version;
};

bool _availability_version_check(uint32_t count, dyld_build_version_t versions[]);

__attribute__((visibility("hidden")))
int32_t __isPlatformVersionAtLeast(uint32_t platform, uint32_t major, uint32_t minor, uint32_t subminor)
{
    dyld_build_version_t v = {platform, (major << 16) | ((minor & 0xff) << 8) | (subminor & 0xff)};
    return _availability_version_check(1, &v);
}
}
