// registry.cpp
// Shimmer
// author: beefviper
// date: July 27, 2025

#include "registry.hpp"

#include <iostream>

namespace shim
{

Registry::Registry(const std::string& subKey, REGSAM access)
{
	if (RegCreateKeyExA(HKEY_CURRENT_USER, subKey.c_str(), 0, nullptr, 0, access, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
	{
		std::cout << "Failed to create or open registry key: " << subKey << std::endl;
		hKey = nullptr;
	}
}

Registry::~Registry()
{
	if (hKey)
	{
		RegCloseKey(hKey);
	}
}


Registry::Registry(Registry&& other) noexcept
	: hKey(other.hKey)
{
	other.hKey = nullptr;
}

Registry& Registry::operator=(Registry&& other) noexcept {
	if (this != &other) {
		if (hKey) RegCloseKey(hKey);
		hKey = other.hKey;
		other.hKey = nullptr;
	}

	return *this;
}

std::string Registry::read(const std::string& valueName) const
{
	if (!hKey)
	{
		return {};
	}

	std::string result{};

	DWORD size{};
	DWORD type{};

	LONG queryResult = RegQueryValueExA(hKey, valueName.c_str(), nullptr, &type, nullptr, &size);
	if (queryResult != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ))
	{
		std::cerr << "Failed to read registry value or value is not a string: " << valueName << std::endl;
		return {};
	}

	std::string buffer(size, '\0');

	queryResult = RegQueryValueExA(hKey, valueName.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(buffer.data()), &size);
	if (queryResult == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ))
	{
		if (size > 0 && buffer[size - 1] == '\0')
		{
			--size;
		}
		result = std::string(buffer.data(), size);
	}
	else
	{
		std::cerr << "Failed to read registry value or value is not a string: " << valueName << std::endl;
	}

	return result;
}

void Registry::write(const std::string& valueName, const std::string& value) const
{
	if (!hKey)
	{
		return;
	}

	const BYTE* dataPtr = reinterpret_cast<const BYTE*>(value.c_str());
	DWORD dataSize = static_cast<DWORD>(value.size() + 1); // +1 for null terminator

	if (RegSetValueExA(hKey, valueName.c_str(), 0, REG_SZ, dataPtr, dataSize) != ERROR_SUCCESS)
	{
		std::cerr << "Failed to write registry value: " << valueName << std::endl;
	}
}

void Registry::clear(const std::string& valueName) const
{
	write(valueName, "");
}

} // namespace shim
