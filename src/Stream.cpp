#include "Stream.hpp"

#include <array>

void ignore_bytes(SDL_IOStream* t_stream, std::streamsize t_bytes_to_ignore)
{
    for (std::streamsize i = 0; i < t_bytes_to_ignore; ++i)
    {
        read_u8(t_stream, 0);
    }
}

auto read_u8(SDL_IOStream* t_stream, std::streamsize t_bytes_to_ignore) -> std::uint_fast8_t
{
    ignore_bytes(t_stream, t_bytes_to_ignore);
    std::uint8_t value{0};
    SDL_ReadU8(t_stream, &value);
    return value;
}

auto read_u16(SDL_IOStream* t_stream, std::streamsize t_bytes_to_ignore) -> std::uint_fast16_t
{
    ignore_bytes(t_stream, t_bytes_to_ignore);
    std::array<std::uint_fast8_t, 2> values{};
    for (auto& value : values)
    {
        value = read_u8(t_stream);
    }
    return (static_cast<std::uint_fast16_t>(values.at(1)) << 8) | values.at(0);
}

auto read_u32(SDL_IOStream* t_stream, std::streamsize t_bytes_to_ignore) -> std::uint_fast32_t
{
    ignore_bytes(t_stream, t_bytes_to_ignore);
    std::array<std::uint_fast16_t, 2> values{};
    for (auto& value : values)
    {
        value = read_u16(t_stream);
    }
    return (static_cast<std::uint_fast32_t>(values.at(1)) << 16) | values.at(0);
}

auto read_u64(SDL_IOStream* t_stream, std::streamsize t_bytes_to_ignore) -> std::uint_fast64_t
{
    ignore_bytes(t_stream, t_bytes_to_ignore);
    std::array<std::uint_fast32_t, 2> values{};
    for (auto& value : values)
    {
        value = read_u32(t_stream);
    }
    return (static_cast<std::uint_fast64_t>(values.at(1)) << 32) | values.at(0);
}

auto read_string(SDL_IOStream* t_stream, std::streamsize t_bytes_to_ignore) -> std::string
{
    ignore_bytes(t_stream, t_bytes_to_ignore);
    const auto size = read_u32(t_stream);
    std::string string(size, 0);
    SDL_ReadIO(t_stream, string.data(), size);
    return string;
}

auto read_double(SDL_IOStream* t_stream, std::streamsize t_bytes_to_ignore) -> double
{
    ignore_bytes(t_stream, t_bytes_to_ignore);
    std::array<std::uint_fast16_t, 2> values{};
    for (auto& value : values)
    {
        value = read_u16(t_stream);
    }
    return static_cast<double>(values.at(0)) / static_cast<double>(values.at(1));
}