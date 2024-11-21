#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "error.h"

// Helper function to load a file fomr disk into a buffer

struct FileBuffer {
    uintmax_t size;
    std::shared_ptr<uint8_t[]> buffer;
};
inline ErrorOr<FileBuffer> load_file(std::string file_path) {
    std::filesystem::path path(file_path);

    if (!std::filesystem::exists(path))
        return ERR("File '" + file_path + "' does not exist");

    auto file_size = std::filesystem::file_size(path);
    std::shared_ptr<uint8_t[]> buffer(new uint8_t[file_size]);

    std::ifstream in(file_path);
    if (!in)
        return ERR("Could not open file '" + file_path + "'");

    in.read(reinterpret_cast<char*>(buffer.get()), file_size);

    return FileBuffer{file_size, buffer};
}