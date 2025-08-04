// path.hpp
// Shimmer
// author: beefviper
// date: July 27, 2025

#pragma once

#include <vector>
#include <string>
#include <filesystem>

#include "registry.hpp"

namespace shimmer
{

class Path
{
public:
	explicit Path();

	bool contains(const std::filesystem::path& testPath) const;
	void add(const std::filesystem::path& newPath);
	void remove(const std::filesystem::path& targetPath);

private:
	std::vector<std::filesystem::path> paths;
	Registry registry{ "Environment" };

	std::vector<std::filesystem::path> splitEnvPaths(const std::string& pathStr);
	std::string joinEnvPaths(const std::vector<std::filesystem::path>& pathList);
	void broadcastChange();
};

} // namespace shimmer
