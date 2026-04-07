#include "core/Paths.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

#include <filesystem>

namespace Paths {

const std::string& getExeDir() {
    static const std::string cached = []() {
#ifdef _WIN32
        char buf[MAX_PATH];
        DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
        if (len == 0 || len == MAX_PATH) return std::string(".");
        return std::filesystem::path(buf).parent_path().string();
#else
        char buf[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len == -1) return std::string(".");
        buf[len] = '\0';
        return std::filesystem::path(buf).parent_path().string();
#endif
    }();
    return cached;
}

std::string getAssetPath(const std::string& relative) {
    if (relative.empty()) return getExeDir() + "/assets";
    return getExeDir() + "/assets/" + relative;
}

} // namespace Paths
