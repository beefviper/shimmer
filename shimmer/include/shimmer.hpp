// shimmer.hpp
// Shimmer
// author: beefviper
// date: July 27, 2025

#pragma once

#include <memory>

#include "registry.hpp"
#include "ini.hpp"
#include "path.hpp"

namespace shimmer
{

class Shimmer
{
public:
	Shimmer(int argc, char* argv[]);

private:
	Registry registry{ "Software\\Shimmer" };
	Path paths;
	std::unique_ptr <Ini> ini{};

	std::string currentExeName{};
	std::filesystem::path currentExeDir{};

	void install();
	void uninstall();
	void init();
	void create(const std::string& name, const std::string& target, ShimMode mode);
	void update(const std::string& target, ShimMode mode);
	void remove();
	void list();
	void rebuild();
	void version();
	void printHelp();
};

} // namespace shimmer
