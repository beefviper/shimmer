// shimmer.cpp
// Shimmer
// author: beefviper
// date: July 27, 2025

#include "shimmer.hpp"

#include <iostream>

#include "mode.hpp"

namespace shimmer
{

static constexpr const char* REG_INSTALLED_PATH = "InstalledPath";
static constexpr const char* SHIMMER_VERSION = "0.1.0";

static ShimMode parseMode(const std::string& modeStr)
{
	if (modeStr == "detached")
	{
		return ShimMode::Detached;
	}
	return ShimMode::Wait;
}

static std::string joinArgs(int argc, char* argv[]) {
	std::string joined;
	for (int i = 1; i < argc; ++i) {
		if (i > 1) joined += " ";
		joined += "\"";
		joined += argv[i];
		joined += "\"";
	}
	return joined;
}

Shimmer::Shimmer(int argc, char* argv[])
{
	char exePathRaw[MAX_PATH];
	GetModuleFileNameA(NULL, exePathRaw, MAX_PATH);
	std::filesystem::path exePath(exePathRaw);

	currentExeName = exePath.stem().string();
	currentExeDir = exePath.parent_path();

	if (_stricmp(currentExeName.c_str(), "shimmer") == 0 && argc < 2)
	{
		printHelp();
		return;
	}

	std::string command = argc > 1 ? argv[1] : "";
	if (command != "--install" && command != "--uninstall" && command != "--init")
	{
		ini = std::make_unique<Ini>();
	}
	else if (command == "--init")
	{
		ini = std::make_unique<Ini>(registry.read("InstalledPath"));
	}

	if (command == "--install")
	{
		install();
	}
	else if (command == "--uninstall")
	{
		uninstall();
	}
	else if (command == "--init")
	{
		init();
	}
	else if (command == "--create" && argc > 3)
	{
		std::string name = argv[2];
		std::string target = argv[3];
		ShimMode mode = ShimMode::Wait;
		if (argc > 4) {
			mode = parseMode(argv[4]);
		}
		create(name, target, mode);
	}
	else if (command == "--update" && argc > 2)
	{
		std::string target = argv[2];
		ShimMode mode = ShimMode::Wait;
		if (argc > 3) {
			mode = parseMode(argv[3]);
		}
		update(target, mode);
	}
	else if (command == "--remove")
	{
		remove();
	}
	else if (command == "--list")
	{
		list();
	}
	else if (command == "--rebuild")
	{
		rebuild();
	}
	else if (command == "--version")
	{
		version();
	}
	else
	{
		auto shims = ini->getShims();
		auto it = std::find_if(shims.begin(), shims.end(),
			[&](const Shim& shim) { return shim.alias == currentExeName; });

		if (it == shims.end())
		{
			MessageBoxA(NULL, ("Shim not found: " + it->alias).c_str(), "Error", MB_OK | MB_ICONERROR);
			return;
		}

		std::string args = joinArgs(argc, argv);
		std::string commandLine = "\"" + it->program + "\" " + args;

		STARTUPINFOA startupInfo = { sizeof(startupInfo)};
		PROCESS_INFORMATION processInfo = {};

		if (!CreateProcessA(NULL, &commandLine[0], NULL, NULL, FALSE, 0 , NULL, NULL, &startupInfo, &processInfo))
		{
			MessageBoxA(NULL, ("Failed to launch: " + it->program).c_str(), "Launch Error", MB_OK | MB_ICONERROR);
			return;
		}

		if (it->mode == ShimMode::Detached)
		{
			CloseHandle(processInfo.hProcess);
			CloseHandle(processInfo.hThread);
			return;
		}

		WaitForSingleObject(processInfo.hProcess, INFINITE);
		DWORD exitCode;
		GetExitCodeProcess(processInfo.hProcess, &exitCode);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
		return;
	}
}

void Shimmer::install()
{
	if (registry.read(REG_INSTALLED_PATH).empty())
	{
		registry.write(REG_INSTALLED_PATH, currentExeDir.string());
		paths.add(currentExeDir);
		std::cout << "Path :" << currentExeDir.string() << std::endl;
		std::cout << "Shimmer installed successfully." << std::endl;
	}
	else
	{
		if (paths.contains(currentExeDir))
		{
			std::cout << "Path:" << currentExeDir.string() << std::endl;
			std::cout << "Shimmer is already installed." << std::endl;
		}
		else
		{
			std::cout << "Path :" << currentExeDir.string() << std::endl;
			std::cout << "Shimmer is already in the above path." << std::endl;
			std::cout << "Please run 'shimmer --uninstall' to remove it first." << std::endl;
		}
	}
}

void Shimmer::uninstall()
{
	if (registry.read(REG_INSTALLED_PATH) == "")
	{
		std::cout << "Shimmer is not installed." << std::endl;
	}
	else
	{
		registry.clear(REG_INSTALLED_PATH);
		paths.remove(currentExeDir);
		std::cout << "Shimmer uninstalled successfully." << std::endl;
	}
}

void Shimmer::init()
{
	auto iniPath = ini->getPath();
	if (std::filesystem::exists(iniPath))
	{
		std::cout << "shimmer.ini already exists at " << iniPath << std::endl;
	}
	else
	{
		ini->writeDefault();
		std::cout << "shimmer.ini created at " << iniPath << std::endl;
	}
}

void Shimmer::create(const std::string& name, const std::string& target, ShimMode mode)
{
	ini->add({ name, target, mode });
}

void Shimmer::update(const std::string& target, ShimMode mode)
{
	ini->add({ currentExeName, target, mode });
}

void Shimmer::remove()
{
	ini->remove(currentExeName);
}

void Shimmer::list()
{
	ini->list();
}

void Shimmer::rebuild()
{
	ini->rebuild();
}

void Shimmer::version()
{
	std::cout << "Shimmer version: " << SHIMMER_VERSION << std::endl;
}

void Shimmer::printHelp() {
	std::cout << R"(
shimmer: Single EXE-based portable app shim system

Usage:
  shimmer.exe                   Show this help
  shimmer.exe --install         Add current directory to user PATH
  shimmer.exe --uninstall       Remove current directory from PATH
  shimmer.exe --init            Create a default shimmer.ini file
  shimmer.exe --list            List registered shims
  shimmer.exe --update <target> Register current exe name to <target>
  shimmer.exe --remove          Remove current shim entry
  shimmer.exe --create <name> <target> [wait|detached]
  shimmer.exe --rebuild         Recreate .exe stubs for all INI entries
  shimmer.exe --version         Print version number
)";
}

} // namespace shimmer
