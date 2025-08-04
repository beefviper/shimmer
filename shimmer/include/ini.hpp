// ini.hpp
// Shimmer
// author: beefviper
// date: July 27, 2025

#pragma once

#include <vector>
#include <string>
#include <filesystem>

#include "mode.hpp"
#include "registry.hpp"

namespace shimmer
{

struct Shim
{
	std::string alias;
	std::string program;
	ShimMode mode{ ShimMode::Wait };
};

class Ini
{
public:
	Ini();
	Ini(const std::filesystem::path& path);
	~Ini();

	std::vector<Shim> getShims() const;
	std::filesystem::path getPath() const;

	bool add(const Shim& shim);
	bool remove(const std::string& alias);
	void list() const;
	void rebuild() const;
	void writeDefault() const;

private:
	Registry registry{ "Software\\Shimmer" };
	std::filesystem::path iniPath;
	std::vector<Shim> shims;
};

} // namespace shimmer
