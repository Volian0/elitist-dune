#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <istream>
#include <string>
#include <vector>

static_assert(sizeof(std::uint8_t) == 1);
static_assert(sizeof(std::uint16_t) == 2);
static_assert(sizeof(std::uint32_t) == 4);
static_assert(sizeof(std::uint64_t) == 8);

void ignore_bytes(SDL_IOStream* t_stream, std::streamsize t_bytes_to_ignore = 0);
auto read_u8(SDL_IOStream* t_stream, std::streamsize t_bytes_to_ignore = 0) -> std::uint_fast8_t;
[[nodiscard]] auto read_u16(SDL_IOStream* t_stream, std::streamsize t_bytes_to_ignore = 0) -> std::uint_fast16_t;
[[nodiscard]] auto read_u32(SDL_IOStream* t_stream, std::streamsize t_bytes_to_ignore = 0) -> std::uint_fast32_t;
[[nodiscard]] auto read_u64(SDL_IOStream* t_stream, std::streamsize t_bytes_to_ignore = 0) -> std::uint_fast64_t;
[[nodiscard]] auto read_string(SDL_IOStream* t_stream, std::streamsize t_bytes_to_ignore = 0) -> std::string;
[[nodiscard]] auto read_double(SDL_IOStream* t_stream, std::streamsize t_bytes_to_ignore = 0) -> double;

template <typename T>
[[nodiscard]] auto read_array(SDL_IOStream* t_stream, std::streamsize t_bytes_to_ignore = 0) -> std::vector<T>
{
    ignore_bytes(t_stream, t_bytes_to_ignore);
    const auto size = read_u32(t_stream);
    std::vector<T> vector;
    for (std::uint_fast32_t i = 0; i < size; ++i)
    {
        vector.emplace_back(t_stream);
    }
    return vector;
}