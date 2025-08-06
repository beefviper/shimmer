// shimmer.cpp
// Shimmer
// author: beefviper
// date: July 27, 2025

#include "shimmer.hpp"

#include <iostream>
#include <string>

#include "mode.hpp"

namespace shim
{

static constexpr const char* REG_INSTALLED_PATH = "InstalledPath";
static constexpr const char* SHIMMER_VERSION = "0.1.0";

shim::ShimMode parseMode(const std::string& modeStr)
{
	if (modeStr == "detached")
	{
		return shim::ShimMode::Detached;
	}
	return shim::ShimMode::Wait;
}

std::string joinArgs(int argc, char* argv[])
{
	std::string joined;
	for (int i = 1; i < argc; ++i)
	{
		if (i > 1) joined += " ";
		joined += "\"";
		joined += argv[i];
		joined += "\"";
	}
	return joined;
}

Shimmer::Shimmer()
{
	char exePathRaw[MAX_PATH];
	GetModuleFileNameA(NULL, exePathRaw, MAX_PATH);
	std::filesystem::path exePath(exePathRaw);

	currentExeName = exePath.stem().string();
	currentExeDir = exePath.parent_path();

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

void Shimmer::init() const
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

void Shimmer::create(const std::string& name, const std::string& target, ShimMode mode) const
{
	ini->add({ name, target, mode });
}

void Shimmer::update(const std::string& target, ShimMode mode) const
{
	ini->add({ currentExeName, target, mode });
}

void Shimmer::remove(std::string target) const
{
	ini->remove(target);
}

void Shimmer::list() const
{
	ini->list();
}

void Shimmer::rebuild() const
{
	ini->rebuild();
}

void Shimmer::version() const
{
	std::cout << "Shimmer version: " << SHIMMER_VERSION << std::endl;
}

void Shimmer::printHelp() const
{
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

} // namespace shim
