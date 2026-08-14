#include "app_ui.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <shellapi.h>

#include <string>
#include <vector>

namespace {

[[nodiscard]] int utf8Main(int argc, char** argv) {
    return runApp(argc, argv);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    int argc = 0;
    LPWSTR* wideArgv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (wideArgv == nullptr) {
        char* empty = nullptr;
        return utf8Main(0, &empty);
    }

    std::vector<std::string> storage;
    storage.reserve(static_cast<size_t>(argc));
    std::vector<char*> argv;
    argv.reserve(static_cast<size_t>(argc));

    for (int i = 0; i < argc; ++i) {
        const int needed = WideCharToMultiByte(CP_UTF8, 0, wideArgv[i], -1, nullptr, 0, nullptr, nullptr);
        std::string utf8(static_cast<size_t>(needed > 0 ? needed - 1 : 0), '\0');
        if (needed > 1) {
            WideCharToMultiByte(CP_UTF8, 0, wideArgv[i], -1, utf8.data(), needed, nullptr, nullptr);
        }
        storage.push_back(std::move(utf8));
    }
    LocalFree(wideArgv);

    for (auto& s : storage) {
        argv.push_back(s.data());
    }

    return utf8Main(static_cast<int>(argv.size()), argv.data());
}
