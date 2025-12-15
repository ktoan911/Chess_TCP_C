// common/utils.hpp
#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdint>
#include <cstddef>
#include <vector>

// Chuyển từ giá trị 2 byte order sang big-endian
inline std::vector<uint8_t> to_big_endian_16(uint16_t value) {
    std::vector<uint8_t> bytes(2);
    bytes[0] = (value >> 8) & 0xFF;
    bytes[1] = value & 0xFF;
    return bytes;
}

// Chuyển từ giá trị 4 byte order sang big-endian
inline std::vector<uint8_t> to_big_endian_32(uint32_t value) {
    std::vector<uint8_t> bytes(4);
    bytes[0] = (value >> 24) & 0xFF;
    bytes[1] = (value >> 16) & 0xFF;
    bytes[2] = (value >> 8) & 0xFF;
    bytes[3] = value & 0xFF;
    return bytes;
}

// Chuyển từ big-endian sang host byte order
inline uint16_t from_big_endian_16(const std::vector<uint8_t>& bytes, size_t start = 0) {
    return (static_cast<uint16_t>(bytes[start]) << 8) |
           static_cast<uint16_t>(bytes[start + 1]);
}

// Chuyển từ big-endian sang host byte order
inline uint32_t from_big_endian_32(const std::vector<uint8_t>& bytes, size_t start = 0) {
    return (static_cast<uint32_t>(bytes[start]) << 24) |
           (static_cast<uint32_t>(bytes[start + 1]) << 16) |
           (static_cast<uint32_t>(bytes[start + 2]) << 8) |
           static_cast<uint32_t>(bytes[start + 3]);
}

#endif // UTILS_HPP