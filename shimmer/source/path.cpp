// path.cpp
// Shimmer
// author: beefviper
// date: July 27, 2025

#include "path.hpp"

#include <sstream>
#include <iostream>
#include <algorithm>

#include <Windows.h>

namespace shimmer
{

static constexpr const char* REG_PATH_VALUE = "PATH";

Path::Path()
{
	std::string pathStr = registry.read(REG_PATH_VALUE);
	paths = splitEnvPaths(pathStr);
}

bool Path::contains(const std::filesystem::path& testPath) const
{
	for (const auto& path : paths)
	{
		if (std::filesystem::exists(path) && std::filesystem::exists(testPath) &&
			std::filesystem::equivalent(path, testPath))
		{
			return true;
		}
	}
	return false;
}

void Path::add(const std::filesystem::path& newPath)
{
	if (contains(newPath))
	{
		return;
	}

	paths.push_back(newPath);
	std::string joined = joinEnvPaths(paths);
	registry.write(REG_PATH_VALUE, joined);
	broadcastChange();
}

void Path::remove(const std::filesystem::path& targetPath)
{
	auto it = std::remove_if(paths.begin(), paths.end(),
		[&](const std::filesystem::path& path)
		{
			return std::filesystem::exists(path) && std::filesystem::exists(targetPath) &&
				std::filesystem::equivalent(path, targetPath);
		});

	if (it != paths.end())
	{
		paths.erase(it, paths.end());
		std::string joined = joinEnvPaths(paths);
		registry.write(REG_PATH_VALUE, joined);
		broadcastChange();
	}
}

std::vector<std::filesystem::path> Path::splitEnvPaths(const std::string& pathStr)
{
	std::vector<std::filesystem::path> result;

	std::stringstream ss(pathStr);
	std::string item;

	while (std::getline(ss, item, ';'))
	{
		if (!item.empty())
		{
			result.emplace_back(item);
		}
	}

	return result;
}

std::string Path::joinEnvPaths(const std::vector<std::filesystem::path>& pathList)
{
	std::ostringstream oss;

	for (size_t i = 0; i < pathList.size(); ++i)
	{
		oss << pathList[i].string();
		if (i < pathList.size() - 1)
		{
			oss << ';';
		}
	}

	return oss.str();
}

void Path::broadcastChange()
{
/*
	SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
		reinterpret_cast<LPARAM>("Environment"),
		SMTO_ABORTIFHUNG, 5000, nullptr);
*/
}

} // namespace shimmer
