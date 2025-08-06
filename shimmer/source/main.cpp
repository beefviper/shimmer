// main.cpp
// Shimmer
// author: beefviper
// date: July 27, 2025

#include <iostream>

#include "shimmer.hpp"

int main(int argc, char* argv[])
{
	shim::Shimmer shimmer;

	if (_stricmp(shimmer.currentExeName.c_str(), "shimmer") == 0 && argc < 2)
	{
		shimmer.printHelp();
		return 0;
	}

	std::string command = argc > 1 ? argv[1] : "";
	if (command != "--install" && command != "--uninstall" && command != "--init")
	{
		shimmer.ini = std::make_unique<shim::Ini>();
	}
	else if (command == "--init")
	{
		shimmer.ini = std::make_unique<shim::Ini>(shimmer.registry.read("InstalledPath"));
	}

	if (command == "--install")
	{
		shimmer.install();
	}
	else if (command == "--uninstall")
	{
		shimmer.uninstall();
	}
	else if (command == "--init")
	{
		shimmer.init();
	}
	else if (command == "--create" && argc > 3)
	{
		std::string name = argv[2];
		std::string target = argv[3];
		shim::ShimMode mode = shim::ShimMode::Wait;
		if (argc > 4) {
			mode = shim::parseMode(argv[4]);
		}
		shimmer.create(name, target, mode);
	}
	else if (command == "--update" && argc > 2)
	{
		std::string target = argv[2];
		shim::ShimMode mode = shim::ShimMode::Wait;
		if (argc > 3) {
			mode = shim::parseMode(argv[3]);
		}
		shimmer.update(target, mode);
	}
	else if (command == "--remove")
	{
		std::string target = shimmer.currentExeName;

		if (argc > 2)
		{
			target = argv[2];
		}

		if (target == "shimmer")
		{
			MessageBoxA(NULL, "Cannot remove the shimmer executable itself.", "Error", MB_OK | MB_ICONERROR);
			return 0;
		}

		shimmer.remove(target);
	}
	else if (command == "--list")
	{
		shimmer.list();
	}
	else if (command == "--rebuild")
	{
		shimmer.rebuild();
	}
	else if (command == "--version")
	{
		shimmer.version();
	}
	else
	{
		auto shims = shimmer.ini->getShims();
		auto it = std::find_if(shims.begin(), shims.end(),
			[&](const shim::Shim& shim) { return shim.alias == shimmer.currentExeName; });

		if (it == shims.end())
		{
			MessageBoxA(NULL, ("Shim not found: " + it->alias).c_str(), "Error", MB_OK | MB_ICONERROR);
			return 0;
		}

		std::string args = shim::joinArgs(argc, argv);
		std::string commandLine = "\"" + it->program + "\" " + args;

		STARTUPINFOA startupInfo = { sizeof(startupInfo) };
		PROCESS_INFORMATION processInfo = {};

		if (!CreateProcessA(NULL, &commandLine[0], NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo))
		{
			MessageBoxA(NULL, ("Failed to launch: " + it->program).c_str(), "Launch Error", MB_OK | MB_ICONERROR);
			return 0;
		}

		if (it->mode == shim::ShimMode::Detached)
		{
			CloseHandle(processInfo.hProcess);
			CloseHandle(processInfo.hThread);
			return 0;
		}

		WaitForSingleObject(processInfo.hProcess, INFINITE);
		DWORD exitCode;
		GetExitCodeProcess(processInfo.hProcess, &exitCode);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
		return 0;
	}
}
