// shimmer.hpp
// Shimmer
// author: beefviper
// date: July 27, 2025

#pragma once

#include <memory>

#include "registry.hpp"
#include "ini.hpp"
#include "path.hpp"

namespace shim
{

shim::ShimMode parseMode(const std::string& modeStr);
std::string joinArgs(int argc, char* argv[]);

class Shimmer
{
public:
	Shimmer();

	void install();
	void uninstall();
	void init() const;
	void create(const std::string& name, const std::string& target, ShimMode mode) const;
	void update(const std::string& target, ShimMode mode) const;
	void remove(std::string target) const;
	void list() const;
	void rebuild() const;
	void version() const;
	void printHelp() const;

//private:
	Registry registry{ "Software\\Shimmer" };
	Path paths;
	std::unique_ptr <Ini> ini{};

	std::string currentExeName{};
	std::filesystem::path currentExeDir{};
};

} // namespace shim
