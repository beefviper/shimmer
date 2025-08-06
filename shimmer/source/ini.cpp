// ini.cpp
// Shimmer
// author: beefviper
// date: July 27, 2025

#include "ini.hpp"

#include <iostream>
#include <fstream>

namespace shim
{

static std::string trim(const std::string str)
{
	size_t start = str.find_first_not_of(" \t\n\r");
	size_t end = str.find_last_not_of(" \t\n\r");
	return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

static ShimMode praseMode(const std::string& modeStr)
{
	if (modeStr == "Detached")
	{
		return ShimMode::Detached;
	}

	return ShimMode::Wait;
}

static std::string modeToString(ShimMode mode)
{
	return mode == ShimMode::Detached ? "Detached" : "Wait";
}

static constexpr const char* REG_INSTALLPATH_VALUE = "InstalledPath";

Ini::Ini() :
	Ini(registry.read(REG_INSTALLPATH_VALUE))
{
}

Ini::Ini(const std::filesystem::path& basePath)
{
	std::filesystem::path regInstalledPath = registry.read(REG_INSTALLPATH_VALUE);
	if (regInstalledPath.empty() && basePath.empty())
	{
		std::cerr << "Error: InstalledPath not found in registry. Shimmer must be installed first." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	else if (!regInstalledPath.empty() && basePath.empty())
	{
		iniPath = regInstalledPath / "shimmer.ini";

		std::ifstream iniFile(iniPath);
		if (!iniFile)
		{
			std::cerr << "Error: Unable to open INI file at " << iniPath << std::endl;
			std::exit(EXIT_FAILURE);
		}

		std::string line;
		while (std::getline(iniFile, line))
		{
			line = trim(line);
			if (line.empty() || line[0] == '[' || line[0] == '#')
			{
				continue;
			}

			auto eq = line.find('=');
			if (eq == std::string::npos)
			{
				std::cerr << "Error: Invalid line in INI file: " << line << std::endl;
				std::cerr << "\tUnable to find '=' in line." << std::endl;
				exit(EXIT_FAILURE);
			}

			std::string alias = trim(line.substr(0, eq));
			std::string rest = trim(line.substr(eq + 1));

			if (rest.size() < 3 || rest[0] != '"')
			{
				std::cerr << "Error: Invalid line in INI file: " << line << std::endl;
				std::cerr << "\tUnable to find opening quote in line." << std::endl;
				exit(EXIT_FAILURE);
			}

			auto quoteEnd = rest.find('"', 1);
			if (quoteEnd == std::string::npos)
			{
				std::cerr << "Error: Invalid line in INI file: " << line << std::endl;
				std::cerr << "\tUnable to find closing quote in line." << std::endl;
				exit(EXIT_FAILURE);
			}

			std::string program = rest.substr(1, quoteEnd - 1);
			std::string modeStr = trim(rest.substr(quoteEnd + 1));
			if (!modeStr.empty() && modeStr[0] == '|')
			{
				modeStr = trim(modeStr.substr(1));
			}

			if (!alias.empty() && !program.empty())
			{
				shims.emplace_back(alias, program, praseMode(modeStr));
			}
			else
			{
				std::cerr << "Error: Invalid line in INI file: " << line << std::endl;
				std::cerr << "\tAlias or program is empty." << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		iniFile.close();
	}
	else
	{
		iniPath = regInstalledPath / "shimmer.ini";
	}
}

Ini::~Ini()
{
	if (!shims.empty())
	{
		std::ofstream file(iniPath);

		if (!file)
		{
			std::cerr << "Error: Unable to open INI file for writing at " << iniPath << std::endl;
			std::exit(EXIT_FAILURE);
		}

		file << "[shimmer]\n";
		for (const Shim& shim : shims)
		{
			file << shim.alias << " = \"" << shim.program << "\" | " << modeToString(shim.mode) << "\n";
		}
	}
}

std::vector<Shim> Ini::getShims() const
{
	return shims;
}

std::filesystem::path Ini::getPath() const
{
	return iniPath;
}
	

bool Ini::add(const Shim& shim)
{
	auto it = std::find_if(shims.begin(), shims.end(),
		[&](const Shim& s)
		{
			return s.alias == shim.alias;
		}
	);

	if (it != shims.end())
	{
		return false;
	}

	char exePathRaw[MAX_PATH];
	GetModuleFileNameA(NULL, exePathRaw, MAX_PATH);
	std::filesystem::path exePath(exePathRaw);

	std::filesystem::path newShim = iniPath.parent_path() / (shim.alias + ".exe");
	try {
		std::filesystem::copy_file(exePath, newShim, std::filesystem::copy_options::overwrite_existing);
	}
	catch (const std::filesystem::filesystem_error& e) {
		MessageBoxA(NULL, e.what(), "Copy Failed", MB_OK | MB_ICONERROR);
		return 1;
	}

	shims.push_back(shim);
	return true;
}

bool Ini::remove(const std::string& alias)
{
	std::filesystem::path shimExePath = iniPath.parent_path() / (alias + ".exe");

	char exePathRaw[MAX_PATH];
	GetModuleFileNameA(NULL, exePathRaw, MAX_PATH);
	std::filesystem::path currentExePath(exePathRaw);

	if (_stricmp(shimExePath.filename().string().c_str(), "shimmer.exe") == 0 &&
		std::filesystem::equivalent(shimExePath, currentExePath))
	{
		MessageBoxA(NULL, "Cannot remove shimmer itself.", "Remove Failed", MB_OK | MB_ICONERROR);
		return false;
	}

	if (std::filesystem::equivalent(shimExePath, currentExePath))
	{
		std::string cmd =
			"cmd.exe /C \""
			"ping -n 3 127.0.0.1 > nul && "
			"del /F /Q \"" + shimExePath.string() + "\"\"";

		STARTUPINFOA si = { sizeof(si) };
		PROCESS_INFORMATION pi;
		if (!CreateProcessA(NULL, cmd.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
			std::string msg = "Failed to schedule deletion of: " + shimExePath.string();
			MessageBoxA(NULL, msg.c_str(), "Delete Failed", MB_OK | MB_ICONERROR);
		}
		else {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
	else
	{
		std::error_code ec;
		std::filesystem::remove(shimExePath, ec);
		if (ec) {
			std::string msg = "Failed to delete shim file: " + shimExePath.string() + "\n" + ec.message();
			MessageBoxA(NULL, msg.c_str(), "Delete Failed", MB_OK | MB_ICONERROR);
		}
	}

	auto it = std::remove_if(shims.begin(), shims.end(),
		[&](const Shim& s)
		{
			return s.alias == alias;
		}
	);

	if (it == shims.end())
	{
		return false;
	}

	shims.erase(it, shims.end());
	return true;
}

void Ini::list() const
{
	if (shims.empty())
	{
		std::cout << "No shims registered." << std::endl;
	}
	else
	{
		for (const auto& shim : shims)
		{
			std::cout << shim.alias << " = " << shim.program << " | " << (shim.mode == ShimMode::Detached ? "Detached" : "Wait") << std::endl;
		}
	}
}

void Ini::rebuild() const
{

}

void Ini::writeDefault() const
{
	std::ofstream file(iniPath);

	if (!file)
	{
		std::cerr << "Error: Unable to create default INI file at " << iniPath << std::endl;
		std::exit(EXIT_FAILURE);
	}

	file << "[shimmer]\n";
	file << "# git = \"C:\\Program Files\\Git\\bin\\git.exe\" | wait\n";
	file << "# dolphin = \"c:\\progs\\emu\\dolphin\\dolphin.exe\" | detached\n";
}

} // namespace shim
