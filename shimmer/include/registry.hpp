// registry.hpp
// Shimmer
// author: beefviper
// date: July 27, 2025

#pragma once

#include <string>

#include <Windows.h>

namespace shim
{

class Registry
{
public:
	Registry(const std::string& subKey, REGSAM access = KEY_READ | KEY_WRITE);
	~Registry();

	Registry(const Registry&) = delete;
	Registry& operator=(const Registry&) = delete;

	Registry(Registry&& other) noexcept;
	Registry& operator=(Registry&& other) noexcept;

	std::string read(const std::string& valueName) const;
	void write(const std::string& valueName, const std::string& value) const;
	void clear(const std::string& valueName) const;

private:
	HKEY hKey{ nullptr };
};

} // namespace shim
