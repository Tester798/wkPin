#pragma once

#include <vector>
#include <sstream>
#include <Windows.h>


HANDLE moduleHandle = GetModuleHandle(NULL);
uintptr_t moduleBase = (uintptr_t)moduleHandle;


struct module_info
{
    std::string module_name;
    uint64_t base_address;
};


struct patch_info
{
    std::string module_name;
    uint64_t rva;
    unsigned char original_byte;
    unsigned char patched_byte;
};


uint64_t parse_hex(const std::string &str)
{
    char *end;
    uint64_t result = std::strtoull(str.c_str(), &end, 16);

    if (*end != '\0')
        throw std::runtime_error(("Not a hex number: \"" + str + "\"").c_str());

    return result;
}


std::vector<patch_info> read_1337_text(std::string text)
{
	std::istringstream stream(text);
	std::vector<patch_info> patch_infos;
	std::string current_module;
	while (!stream.eof())
	{
		std::string line;
		std::getline(stream, line);

		if (line.empty())
			continue;

		if (line.find('>', 0) == 0)
		{
			current_module = line.substr(1);
		}
		else
		{
			if (current_module.empty())
				throw std::runtime_error("Invalid 1337 file. Expected module name.");

			size_t colon_pos = line.find(':');
			size_t arrow_pos = line.find("->");

			if (colon_pos == -1 || arrow_pos == -1 || arrow_pos <= colon_pos)
				throw std::runtime_error("Invalid 1337 file.");

			uint64_t rva = parse_hex(line.substr(0, colon_pos));
			uint64_t original_byte = parse_hex(line.substr(colon_pos + 1, arrow_pos - colon_pos - 1));
			uint64_t patched_byte = parse_hex(line.substr(arrow_pos + 2));

			if (original_byte > 255 || patched_byte > 255)
				throw std::runtime_error("Invalid 1337 file.");

			patch_infos.push_back({
				current_module,
				rva,
				static_cast<unsigned char>(original_byte),
				static_cast<unsigned char>(patched_byte)
				});
		}
	}
	return patch_infos;
}


unsigned char read_byte(uint64_t address)
{
	moduleHandle = GetCurrentProcess();
	unsigned char buffer;
    SIZE_T bytes_read;
	if (!ReadProcessMemory(moduleHandle, reinterpret_cast<void*>(address), &buffer, sizeof(buffer), &bytes_read) || bytes_read != sizeof(buffer))
		throw std::runtime_error("Error ReadProcessMemory.");
    return buffer;
}


void write_byte(uint64_t address, unsigned char value)
{
	moduleHandle = GetCurrentProcess();
	SIZE_T bytes_written;
	if (!WriteProcessMemory(moduleHandle, reinterpret_cast<void*>(address), &value, sizeof(value), &bytes_written) || bytes_written != sizeof(value))
		throw std::runtime_error("Error WriteProcessMemory.");
}


bool verify_patches(const std::vector<patch_info> &patch_infos, bool revert)
{
    for (auto&& patch_info : patch_infos)
    {
        unsigned char value = read_byte(moduleBase + patch_info.rva);
        if (value != (revert ? patch_info.patched_byte : patch_info.original_byte))
            return false;
    }
    return true;
}


void apply_patches(const std::vector<patch_info> &patch_infos, bool revert)
{
	for (auto&& patch_info : patch_infos)
    {
        unsigned char value = revert ? patch_info.original_byte : patch_info.patched_byte;
        write_byte(moduleBase + patch_info.rva, value);
    }
}