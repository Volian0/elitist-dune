#pragma once

#include "Stream.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>
#include <vector>

class NoteInfo
{
public:
    enum class Type : std::uint_fast8_t
    {
        ON,
        OFF,
        ALL_OFF,
        ENUM_SIZE [[maybe_unused]]
    };

    NoteInfo(SDL_IOStream* t_stream);

    [[nodiscard]] constexpr auto get_tick_offset() const noexcept
    {
        return m_tick_offset;
    }

    [[nodiscard]] constexpr auto get_type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] constexpr auto get_key() const noexcept
    {
        return m_key;
    }

    [[nodiscard]] constexpr auto get_velocity() const noexcept
    {
        return m_velocity;
    }

private:
    std::uint_fast32_t m_tick_offset;
    Type m_type;
    std::uint_fast8_t m_key;
    std::uint_fast8_t m_velocity;
};

class TileInfo
{
public:
    enum class Type : unsigned char
    {
        SINGLE,
        LONG,
        DOUBLE,
        EMPTY
    };

    TileInfo(SDL_IOStream* t_stream);

    [[nodiscard]] constexpr auto get_type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] constexpr auto get_unit_length() const noexcept
    {
        return m_unit_length;
    }

    [[nodiscard]] constexpr auto get_events() const noexcept -> const auto&
    {
        return m_events.at(0);
    }

    [[nodiscard]] constexpr auto get_double_tile_events() const noexcept -> const auto&
    {
        return m_events;
    }

private:
    std::uint_fast32_t m_unit_length;
    Type m_type;
    std::array<std::vector<NoteInfo>, 2> m_events;
};

class SongInfo
{
public:
    SongInfo(SDL_IOStream* t_stream);
    SongInfo(const char* t_path);

    [[nodiscard]] constexpr auto get_note_ticks_per_single_tile() const noexcept
    {
        return m_note_ticks_per_single_tile;
    }

    [[nodiscard]] constexpr auto get_length_units_per_single_tile() const noexcept
    {
        return m_length_units_per_single_tile;
    }

    [[nodiscard]] constexpr auto get_starting_tempo() const noexcept
    {
        return m_starting_tempo;
    }

    [[nodiscard]] constexpr auto get_tiles() const noexcept -> const auto&
    {
        return m_tiles;
    }

private:
    std::uint_fast16_t m_note_ticks_per_single_tile;
    std::uint_fast16_t m_length_units_per_single_tile;
    double m_starting_tempo;
    double m_acceleration;
    std::vector<TileInfo> m_tiles;
};