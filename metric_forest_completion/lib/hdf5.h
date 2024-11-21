#pragma once

#include <array>
#include <expected>
#include <span>

#include "error.h"
#include "fileio.h"

// Minimal class for loading floating point data from HDF5 files. Only tested for the fashion_mnist dataset from https://github.com/erikbern/ann-benchmarks

class HDF5 {
  private:
    using FileSpan = std::span<uint8_t, std::dynamic_extent>;

    static constexpr std::array<uint8_t, 8> HDF5_SIGNATURE{137, 72, 68, 70, 13, 10, 26, 10};

    static constexpr std::array<uint8_t, 4> HDF5_B_TREE_NODE_SIGNATURE{'T', 'R', 'E', 'E'};
    static constexpr std::array<uint8_t, 4> HDF5_B_TREE_LEAF_SIGNATURE{'S', 'N', 'O', 'D'};
    static constexpr std::array<uint8_t, 4> HDF5_HEAP_SIGNATURE{'H', 'E', 'A', 'P'};

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /* Specification Structures                                                                                                   */
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    struct __attribute__((packed)) HDF5SuperBlockHeader {
        std::array<uint8_t, 8> signature;
        uint8_t version;
        uint8_t free_file_version;
        uint8_t root_group_symbol_table_version;
        uint8_t _reserved_0;
        uint8_t shared_header_message_version;
        uint8_t size_of_offsets;
        uint8_t size_of_lengths;
        uint8_t _reserved_1;
        uint16_t group_leaf_node_k;
        uint16_t group_internal_node_k;
        uint32_t file_consistency_flags;
    };

    template <typename Off>
    struct __attribute__((packed)) HDF5SuperblockOffsets {
        Off base_offset;
        Off free_file_address;
        Off end_of_file;
        Off driver_information_block;
    };

    template <typename Off>
    struct __attribute__((packed)) HDF5SymbolTableEntry {
        Off link_name_offset;
        Off object_header_address;
        uint32_t cache_type;
        uint32_t _reserved;
        std::array<uint8_t, 16> scratch_pad;
    };

    struct __attribute__((packed)) HDF5ObjectHeader {
        uint8_t version;
        uint8_t _reserved_0;
        uint16_t number_of_header_messages;
        uint32_t refrence_count;
        uint32_t size;
        uint32_t _reserved_1;
    };

    template <typename Off>
    struct __attribute__((packed)) HDF5BTreeNode {
        std::array<uint8_t, 4> signature;
        uint8_t type;
        uint8_t level;
        uint16_t entries;
        Off left_sibling;
        Off right_sibling;
    };

    struct __attribute__((packed)) HDF5BTreeLeaf {
        std::array<uint8_t, 4> signature;
        uint8_t version;
        uint8_t _reserved;
        uint16_t entries;
    };

    template <typename Off, typename Len>
    struct __attribute__((packed)) HDF5BLocalHeap {
        std::array<uint8_t, 4> signature;
        uint8_t version;
        std::array<uint8_t, 3> _reserved;
        Len size;
        Len free_list_offset;
        Off ptr;
    };

    struct __attribute__((packed)) HDF5BDataLayoutMessage {
        uint8_t version;
        uint8_t layout_class;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct SymbolTableEntry {
        off_t link_name_offset;
        off_t object_header_offset;
    };

    struct SuperBlock {
        uint8_t size_of_offsets;
        uint8_t size_of_lengths;

        SymbolTableEntry root_entry;
    };

    struct LocalHeap {
        size_t length;
        off_t ptr;
    };

    template <typename Off>
    static auto read_object_headers(FileSpan file, off_t location, auto callback) {
        auto* object_header = (HDF5ObjectHeader*)(file.data() + location);

        off_t current_message_location = location + sizeof(HDF5ObjectHeader);

        using Ret = decltype(callback(0, 0, file.data()))::value_type;

        for (size_t i = 0; i < object_header->number_of_header_messages; i++) {
            uint16_t message_type = *(uint16_t*)(file.data() + current_message_location);
            uint16_t message_size = *(uint16_t*)(file.data() + current_message_location + 2);
            if (message_type == 0x10) { // Object Header Continuation
                current_message_location = *(Off*)(file.data() + current_message_location + 8);
            } else {
                auto callback_res_or_error = callback(message_type, message_size, file.data() + current_message_location + 8);
                if (!callback_res_or_error.has_value()) {
                    std::expected<Ret, std::string> res = ERR(callback_res_or_error.error());
                    return res;
                }

                if (callback_res_or_error.value().has_value())
                    return std::expected<Ret, std::string>(callback_res_or_error.value().value());

                current_message_location += 8 + message_size;
            }
        }

        return std::expected<Ret, std::string>(Ret{});
    }

    static ErrorOr<SymbolTableEntry> load_symbol_table_entry(FileSpan file, off_t address) {
        auto* symbol_table_entry = (HDF5SymbolTableEntry<uint64_t>*)(file.data() + address);

        SymbolTableEntry res;

        res.link_name_offset = symbol_table_entry->link_name_offset;
        res.object_header_offset = symbol_table_entry->object_header_address;

        return res;
    }

    static ErrorOr<SuperBlock> load_superblock(FileSpan file) {
        if (file.size() < sizeof(HDF5SuperBlockHeader))
            return std::unexpected<std::string>{"File not big enough to be a valid HDF5 file"};

        // TODO: The superblock is not required to be at the start of the file
        auto* superblock_header = (HDF5SuperBlockHeader*)file.data();
        if (superblock_header->signature != HDF5_SIGNATURE)
            return ERR("File does not contain a valid HDF5 signature");

        if (superblock_header->version != 0)
            return ERR("HDF5 superblock version is not 0");

        if (superblock_header->size_of_offsets != 8)
            return ERR("HDF5 superblock size of offsets is not 8");
        if (superblock_header->size_of_lengths != 8)
            return ERR("HDF5 superblock size of lengths is not 8");

        if (file.size() < sizeof(HDF5SuperBlockHeader) + sizeof(HDF5SuperblockOffsets<uint64_t>) + sizeof(HDF5SymbolTableEntry<uint64_t>))
            return ERR("File not big enough to be a valid HDF5 file");

        auto* superblock_offsets = (HDF5SuperblockOffsets<uint64_t>*)(file.subspan(sizeof(HDF5SuperBlockHeader)).data());

        if (superblock_offsets->base_offset != 0)
            return ERR("HDF5 header has an invalid base offset\n");

        SuperBlock res;

        res.size_of_offsets = superblock_header->size_of_offsets;
        res.size_of_lengths = superblock_header->size_of_lengths;
        res.root_entry = TRY(load_symbol_table_entry(file, sizeof(HDF5SuperBlockHeader) + sizeof(HDF5SuperblockOffsets<uint64_t>)));

        return res;
    }

    static ErrorOr<LocalHeap> load_heap(FileSpan file, off_t address) {
        auto* heap_obj = (HDF5BLocalHeap<uint64_t, uint64_t>*)(file.data() + address);
        if (heap_obj->signature != HDF5_HEAP_SIGNATURE)
            return ERR("Invalid heap signature");

        LocalHeap res;

        res.length = heap_obj->size;
        res.ptr = heap_obj->ptr;

        return res;
    }

    static ErrorOr<void> walk_b_tree(FileSpan file, off_t address, LocalHeap heap, auto callback) {
        auto* tree = (HDF5BTreeNode<uint64_t>*)(file.data() + address);

        if (tree->signature != HDF5_B_TREE_NODE_SIGNATURE)
            return ERR("Invalid tree node signature");

        if (tree->type != 0)
            return ERR("Can't walk non group node trees");

        using Off = uint64_t;
        using Len = uint64_t;

        address += sizeof(HDF5BTreeNode<uint64_t>);

        for (size_t i = 0; i < tree->entries; i++) {
            auto key_ptr = *(Len*)(file.data() + address);
            auto child_ptr = *(Off*)(file.data() + address + sizeof(Len));

            if (tree->level == 0) {
                auto* leaf = (HDF5BTreeLeaf*)(file.data() + child_ptr);
                if (leaf->signature != HDF5_B_TREE_LEAF_SIGNATURE)
                    return ERR("Invalid tree leaf signature");

                child_ptr += sizeof(HDF5BTreeLeaf);

                for (size_t j = 0; j < leaf->entries; j++) {
                    TRY(callback(TRY(load_symbol_table_entry(file, child_ptr))));
                    child_ptr += sizeof(HDF5SymbolTableEntry<Off>);
                }
            } else {
                walk_b_tree(file, child_ptr, heap, callback);
            }

            address += sizeof(Len) + sizeof(Off);
        }
        return {};
    }

  public:
    static ErrorOr<void> print_data_sets(std::string filename) {
        auto file_buffer = TRY(load_file(filename));
        auto file = std::span<uint8_t, std::dynamic_extent>(file_buffer.buffer.get(), file_buffer.size);

        auto superblock = TRY(load_superblock(file));

        auto adresses_opt = TRY(read_object_headers<uint64_t>(
            file, superblock.root_entry.object_header_offset, [](uint16_t message_type, uint16_t message_size, uint8_t* ptr) -> ErrorOr<std::optional<std::tuple<uint64_t, uint64_t>>> {
                if (message_type != 0x11)
                    return {};
                uint64_t b_tree_address = *(uint64_t*)(ptr);
                uint64_t heap_address = *(uint64_t*)(ptr + sizeof(uint64_t));
                return std::tuple{b_tree_address, heap_address};
            }));
        if (!adresses_opt.has_value()) {
            return ERR("Super block object headers missing group information");
        }
        auto [b_tree_address, heap_object_address] = adresses_opt.value();

        auto heap = TRY(load_heap(file, heap_object_address));

        TRY(walk_b_tree(file, b_tree_address, heap, [&](SymbolTableEntry entry) -> ErrorOr<void> {
            std::print("KEY: {}\n", (char*)(file.data() + heap.ptr + entry.link_name_offset));
            return {};
        }));

        return {};
    }

    template <typename T>
    static ErrorOr<std::vector<T>> load_data_set(std::string filename, std::string name) {
        auto file_buffer = TRY(load_file(filename));
        auto file = std::span<uint8_t, std::dynamic_extent>(file_buffer.buffer.get(), file_buffer.size);

        auto superblock = TRY(load_superblock(file));

        auto adresses_opt = TRY(read_object_headers<uint64_t>(
            file, superblock.root_entry.object_header_offset, [](uint16_t message_type, uint16_t message_size, uint8_t* ptr) -> ErrorOr<std::optional<std::tuple<uint64_t, uint64_t>>> {
                if (message_type != 0x11)
                    return {};
                uint64_t b_tree_address = *(uint64_t*)(ptr);
                uint64_t heap_address = *(uint64_t*)(ptr + sizeof(uint64_t));
                return std::tuple{b_tree_address, heap_address};
            }));
        if (!adresses_opt.has_value()) {
            return ERR("Super block object headers missing group information");
        }
        auto [b_tree_address, heap_object_address] = adresses_opt.value();

        auto heap = TRY(load_heap(file, heap_object_address));

        std::optional<SymbolTableEntry> found_entry;

        TRY(walk_b_tree(file, b_tree_address, heap, [&](SymbolTableEntry entry) -> ErrorOr<void> {
            std::string key = (char*)(file.data() + heap.ptr + entry.link_name_offset);
            if (key == name) {
                found_entry = entry;
            }
            return {};
        }));

        if (!found_entry.has_value())
            return ERR("No dataset found by that name");

        std::optional<uint64_t> rows;
        std::optional<uint64_t> cols;

        std::optional<uint64_t> data_offset;
        std::optional<uint64_t> data_size;

        TRY(read_object_headers<uint64_t>(
            file, found_entry.value().object_header_offset, [&rows, &cols, &data_offset, &data_size](uint16_t message_type, uint16_t message_size, uint8_t* ptr) -> ErrorOr<std::optional<bool>> {
                if (message_type == 1) {
                    uint8_t version = *ptr;
                    uint8_t dimensionality = *(ptr + 1);
                    uint8_t flags = *(ptr + 2);

                    if (version != 1)
                        return ERR("Invalid version for data space message");
                    if (dimensionality != 2)
                        return ERR("Invalid dimensionality for data space message");
                    if (flags != 1)
                        return ERR("Invalid flags for data space message");

                    uint64_t dim_0_size = *(uint64_t*)(ptr + sizeof(uint64_t) * 0 + 8);
                    uint64_t dim_1_size = *(uint64_t*)(ptr + sizeof(uint64_t) * 1 + 8);

                    uint64_t dim_0_max = *(uint64_t*)(ptr + sizeof(uint64_t) * 2 + 8);
                    uint64_t dim_1_max = *(uint64_t*)(ptr + sizeof(uint64_t) * 3 + 8);

                    if (dim_0_size != dim_0_max)
                        return ERR("Dimension 0 has variable size");
                    if (dim_1_size != dim_1_max)
                        return ERR("Dimension 1 has variable size");

                    rows = dim_0_size;
                    cols = dim_1_size;

                    // std::print("     Simple Dataspace\n");
                    // std::print("        Version: {}\n", version);
                    // std::print("        Dimensionality: {}\n", dimensionality);
                    // std::print("        Flags: {}\n", flags);
                    // std::print("        Dim0:\n");
                    // std::print("            Size: {}\n", dim_0_size);
                    // std::print("            Max: {}\n", dim_0_max);
                    // std::print("        Dim1:\n");
                    // std::print("            Size: {}\n", dim_1_size);
                    // std::print("            Max: {}\n", dim_1_max);
                } else if (message_type == 3) {

                    uint8_t version = (*ptr) & 0b1111;
                    uint8_t data_class = (*ptr) >> 4;

                    uint32_t size = *(uint32_t*)(ptr + 4);

                    if (version != 1)
                        return ERR("Invalid version for data type message");
                    if (data_class != 1)
                        return ERR("Data is not floating point");

                    // TODO: Check that the floats are in IEEE format

                    // std::print("    Data Type\n");
                    // std::print("        Version: {}\n", version);
                    // std::print("        Class: {}\n", data_class);
                    // std::print("        Datatype size: {}\n", size);
                } else if (message_type == 8) {
                    if (*ptr != 3) {
                        return ERR("Dataset layout is not version 3");
                    }

                    auto* layout = (HDF5BDataLayoutMessage*)(ptr);
                    if (layout->layout_class != 1) {
                        return ERR("Dataset layout is not contiguous");
                    }

                    uint64_t offset = *(uint64_t*)(ptr + sizeof(HDF5BDataLayoutMessage));
                    uint64_t size = *(uint64_t*)(ptr + sizeof(HDF5BDataLayoutMessage) + sizeof(uint64_t));

                    data_offset = offset;
                    data_size = size;

                    // std::print("    Data Layout\n");
                    // std::print("        Version: {}\n", layout->version);
                    // std::print("        Layout class: {}\n", layout->layout_class);
                    // std::print("        Location: 0x{:x}\n", offset);
                    // std::print("        Size: 0x{:x}\n", size);
                }
                return {};
            }));

        if (!rows.has_value() || !cols.has_value() || !data_offset.has_value() || !data_size.has_value()) {
            return ERR("Missing data needed to load dataset");
        }

        if (sizeof(T) * *rows != *data_size) {
            return ERR("Given data extraction type is of the incorrect size. Correct size: " + std::to_string(*data_size / *rows));
        }

        std::vector<T> res;

        T* ptr = (T*)(file.data() + *data_offset);

        for (size_t i = 0; i < *rows; i++) {
            res.push_back(*ptr);
            ptr++;
        }

        return res;
    }
};