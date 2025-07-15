#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <filesystem>
#include <algorithm>
#include <vector>

// === Version ===
constexpr int SHIMMER_VERSION = 8;

// === Forward declarations ===
std::map<std::string, std::string> loadIni(const std::filesystem::path& path);
void saveIni(const std::filesystem::path& path, const std::map<std::string, std::string>& shims);
void writeDefaultIni(const std::filesystem::path& path);
std::string getUserPathEnv();
bool ensurePathContains(const std::string& shimDir);
bool removeFromUserPath(const std::string& shimDir);
std::string joinArgs(int argc, char* argv[]);
void printHelp();
void rebuildShims(const std::filesystem::path& shimExe, const std::filesystem::path& dir, const std::map<std::string, std::string>& shims);
bool parseTarget(const std::string& entry, std::string& exe, bool& detached);
std::string getShimmerInstallPath();
void setShimmerInstallPath(const std::string& path);
void clearShimmerInstallPath();

int main(int argc, char* argv[]) {
    char exePathRaw[MAX_PATH];
    GetModuleFileNameA(NULL, exePathRaw, MAX_PATH);
    std::filesystem::path exePath(exePathRaw);
    std::string shimName = exePath.stem().string();
    std::filesystem::path shimDir = exePath.parent_path();
    std::filesystem::path iniPath = shimDir / "shims.ini";

    if (_stricmp(shimName.c_str(), "shimmer") == 0 && argc < 2) {
        printHelp();
        return 0;
    }

    if (argc == 2) {
        std::string cmd = argv[1];
        if (cmd == "--install") {
            std::string previous = getShimmerInstallPath();
            std::string current = shimDir.string();

            if (!previous.empty() && previous != current) {
                std::cout << "shimmer is already installed from:\n  " << previous << "\n";
                std::cout << "Please uninstall it before installing again.\n";
                return 1;
            }

            if (ensurePathContains(current)) {
                setShimmerInstallPath(current);
                std::cout << "Shim directory added to user PATH.\n";
            }
            else {
                std::cout << "Shim directory already in PATH.\n";
            }
            return 0;
        }
        else if (cmd == "--uninstall") {
            std::cout << (removeFromUserPath(shimDir.string())
                ? "Shim directory removed from user PATH.\n"
                : "Shim directory was not in PATH.\n");
            clearShimmerInstallPath();
            return 0;
        }
        else if (cmd == "--init") {
            if (std::filesystem::exists(iniPath))
                std::cout << "shims.ini already exists.\n";
            else {
                writeDefaultIni(iniPath);
                std::cout << "Created default shims.ini.\n";
            }
            return 0;
        }
        else if (cmd == "--list") {
            auto shims = loadIni(iniPath);
            for (const auto& [k, v] : shims)
                std::cout << k << " -> " << v << "\n";
            return 0;
        }
        else if (cmd == "--remove") {
            auto shims = loadIni(iniPath);
            if (shims.erase(shimName)) {
                saveIni(iniPath, shims);
                std::cout << "Removed shim: " << shimName << "\n";
            }
            else {
                std::cout << "No shim named \"" << shimName << "\" was found.\n";
            }
            return 0;
        }
        else if (cmd == "--rebuild") {
            auto shims = loadIni(iniPath);
            rebuildShims(exePath, shimDir, shims);
            return 0;
        }
        else if (cmd == "--help") {
            printHelp();
            return 0;
        }
        else if (cmd == "--version") {
            std::cout << "shimmer version " << SHIMMER_VERSION << "\n";
            return 0;
        }
    }

    if (argc == 3 && std::string(argv[1]) == "--add") {
        auto shims = loadIni(iniPath);
        shims[shimName] = argv[2];
        saveIni(iniPath, shims);
        std::cout << "Updated shim \"" << shimName << "\" -> " << argv[2] << "\n";
        return 0;
    }

    if ((argc == 4 || argc == 5) && std::string(argv[1]) == "--create") {
        std::string newName = argv[2];
        std::string target = argv[3];
        std::string mode = (argc == 5) ? argv[4] : "wait";
        std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);

        if (mode != "wait" && mode != "detached") {
            std::cerr << "Invalid mode: " << mode << "\n";
            return 1;
        }

        std::filesystem::path newShim = shimDir / (newName + ".exe");

        try {
            std::filesystem::copy_file(exePath, newShim, std::filesystem::copy_options::overwrite_existing);
        }
        catch (const std::filesystem::filesystem_error& e) {
            MessageBoxA(NULL, e.what(), "Copy Failed", MB_OK | MB_ICONERROR);
            return 1;
        }

        auto shims = loadIni(iniPath);
        shims[newName] = target + " | " + mode;
        saveIni(iniPath, shims);
        std::cout << "Created shim \"" << newName << "\" -> " << target << " (" << mode << ")\n";
        return 0;
    }

    auto shims = loadIni(iniPath);
    auto it = shims.find(shimName);
    if (it == shims.end()) {
        MessageBoxA(NULL, ("Shim not found: " + shimName).c_str(), "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    std::string targetExec;
    bool isDetached = false;

    if (!parseTarget(it->second, targetExec, isDetached)) {
        MessageBoxA(NULL, "Malformed target entry.", "INI Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    std::string args = joinArgs(argc, argv);
    std::string commandLine = "\"" + targetExec + "\" " + args;

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if (!CreateProcessA(NULL, &commandLine[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        MessageBoxA(NULL, ("Failed to launch: " + targetExec).c_str(), "Launch Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (isDetached) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return 0;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exitCode;
}

// === Helper function definitions ===

std::map<std::string, std::string> loadIni(const std::filesystem::path& path) {
    std::ifstream file(path);
    std::map<std::string, std::string> shims;
    std::string line;
    bool inSection = false;

    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;
        if (line[0] == '[') {
            inSection = (line == "[shims]");
            continue;
        }
        if (!inSection) continue;
        auto eq = line.find('=');
        if (eq != std::string::npos) {
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            key.erase(std::remove_if(key.begin(), key.end(), isspace), key.end());
            val.erase(0, val.find_first_not_of(" \t"));
            shims[key] = val;
        }
    }

    return shims;
}

void saveIni(const std::filesystem::path& path, const std::map<std::string, std::string>& shims) {
    std::ofstream file(path);
    file << "[shims]\n";
    for (const auto& [key, val] : shims)
        file << key << " = " << val << "\n";
}

void writeDefaultIni(const std::filesystem::path& path) {
    std::ofstream file(path);
    file << R"(; shimmer: shim launcher configuration file
; Format: name = full_path_to_exe | mode
; mode is optional: wait (default) or detached

[shims]
; notepad = C:\Windows\System32\notepad.exe | detached
; git = C:\Programs\Git\bin\git.exe | wait
)";
}

bool parseTarget(const std::string& entry, std::string& exe, bool& detached) {
    auto pipe = entry.find('|');
    if (pipe == std::string::npos) {
        exe = entry;
        detached = false;
        return true;
    }
    exe = entry.substr(0, pipe);
    std::string flag = entry.substr(pipe + 1);
    exe.erase(exe.find_last_not_of(" \t") + 1);
    flag.erase(0, flag.find_first_not_of(" \t"));
    std::transform(flag.begin(), flag.end(), flag.begin(), ::tolower);
    detached = (flag == "detached");
    return true;
}

std::string getUserPathEnv() {
    char* buffer = nullptr;
    size_t len = 0;
    _dupenv_s(&buffer, &len, "PATH");
    std::string path(buffer ? buffer : "");
    free(buffer);
    return path;
}

bool ensurePathContains(const std::string& shimDir) {
    std::string currentPath = getUserPathEnv();
    if (currentPath.find(shimDir) != std::string::npos)
        return false;

    std::string newPath = currentPath;
    if (!newPath.empty() && newPath.back() != ';')
        newPath += ";";
    newPath += shimDir;

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "PATH", 0, REG_EXPAND_SZ,
            reinterpret_cast<const BYTE*>(newPath.c_str()), static_cast<DWORD>(newPath.size() + 1));
        RegCloseKey(hKey);
        SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
            reinterpret_cast<LPARAM>("Environment"), SMTO_ABORTIFHUNG, 5000, nullptr);
        return true;
    }
    return false;
}

bool removeFromUserPath(const std::string& pathToRemove) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
        return false;

    char buffer[8192]{};
    DWORD bufferSize = sizeof(buffer);
    DWORD type = 0;

    if (RegQueryValueExA(hKey, "PATH", nullptr, &type, reinterpret_cast<BYTE*>(buffer), &bufferSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }

    std::string currentPath(buffer, bufferSize - 1);
    std::string normalized = pathToRemove;
    std::replace(currentPath.begin(), currentPath.end(), '\\', '/');
    std::replace(normalized.begin(), normalized.end(), '\\', '/');

    std::vector<std::string> parts;
    size_t pos = 0;
    while ((pos = currentPath.find(';')) != std::string::npos) {
        parts.push_back(currentPath.substr(0, pos));
        currentPath.erase(0, pos + 1);
    }
    if (!currentPath.empty()) parts.push_back(currentPath);

    bool found = false;
    std::string newPath;
    for (const auto& part : parts) {
        std::string norm = part;
        std::replace(norm.begin(), norm.end(), '\\', '/');
        if (norm == normalized)
            found = true;
        else {
            if (!newPath.empty()) newPath += ";";
            newPath += part;
        }
    }

    if (!found) {
        RegCloseKey(hKey);
        return false;
    }

    RegSetValueExA(hKey, "PATH", 0, REG_EXPAND_SZ,
        reinterpret_cast<const BYTE*>(newPath.c_str()),
        static_cast<DWORD>(newPath.size() + 1));

    RegCloseKey(hKey);
    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
        reinterpret_cast<LPARAM>("Environment"), SMTO_ABORTIFHUNG, 5000, nullptr);
    return true;
}

void rebuildShims(const std::filesystem::path& shimExe, const std::filesystem::path& dir, const std::map<std::string, std::string>& shims) {
    for (const auto& [key, _] : shims) {
        std::filesystem::path newPath = dir / (key + ".exe");

        if (std::filesystem::exists(newPath) && std::filesystem::exists(shimExe)) {
            try {
                if (std::filesystem::equivalent(newPath, shimExe)) continue;
            }
            catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Error checking equivalence: " << e.what() << "\n";
            }
        }

        try {
            std::filesystem::copy_file(shimExe, newPath, std::filesystem::copy_options::overwrite_existing);
            std::cout << "Created: " << newPath.filename().string() << "\n";
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Failed to create " << newPath << ": " << e.what() << "\n";
        }
    }
}

std::string joinArgs(int argc, char* argv[]) {
    std::string joined;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) joined += " ";
        joined += "\"";
        joined += argv[i];
        joined += "\"";
    }
    return joined;
}

void printHelp() {
    std::cout << R"(
shimmer: Single EXE-based portable app shim system

Usage:
  shimmer.exe                   Show this help
  shimmer.exe --install        Add current directory to user PATH
  shimmer.exe --uninstall      Remove current directory from PATH
  shimmer.exe --init           Create a default shims.ini file
  shimmer.exe --list           List registered shims
  shimmer.exe --add <target>   Register current exe name to <target>
  shimmer.exe --remove         Remove current shim entry
  shimmer.exe --create <name> <target> [wait|detached]
  shimmer.exe --rebuild        Recreate .exe stubs for all INI entries
  shimmer.exe --version        Print version number
)";
}

std::string getShimmerInstallPath() {
    HKEY hKey;
    char buffer[MAX_PATH]{};
    DWORD size = sizeof(buffer);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\shimmer", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "InstallPath", nullptr, nullptr, (LPBYTE)buffer, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(buffer, size - 1);
        }
        RegCloseKey(hKey);
    }
    return "";
}

void setShimmerInstallPath(const std::string& path) {
    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\shimmer", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "InstallPath", 0, REG_SZ, (const BYTE*)path.c_str(), (DWORD)path.size() + 1);
        RegCloseKey(hKey);
    }
}

void clearShimmerInstallPath() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\shimmer", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "InstallPath", 0, REG_SZ, (const BYTE*)"", 1);
        RegCloseKey(hKey);
    }
}
