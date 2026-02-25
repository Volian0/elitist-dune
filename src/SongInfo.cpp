#include "SongInfo.hpp"

#include "Stream.hpp"
#include "Utility.hpp"

#include <fstream>

NoteInfo::NoteInfo(SDL_IOStream* t_stream)
    : m_tick_offset{read_u32(t_stream)}, m_type{static_cast<Type>(read_u8(t_stream))},
      m_key{m_type != Type::ALL_OFF ? read_u8(t_stream) : std::uint_fast8_t{0}},
      m_velocity{m_type == Type::ON ? read_u8(t_stream) : std::uint_fast8_t{0}}
{
}

TileInfo::TileInfo(SDL_IOStream* t_stream)
    : m_unit_length{read_u32(t_stream)}, m_type{static_cast<Type>(read_u8(t_stream))}
{
    if (m_type != Type::EMPTY)
    {
        m_events.at(0) = read_array<NoteInfo>(t_stream);
        if (m_type == Type::DOUBLE)
        {
            m_events.at(1) = read_array<NoteInfo>(t_stream);
        }
    }
}

SongInfo::SongInfo(SDL_IOStream* t_stream)
    : m_note_ticks_per_single_tile{read_u16(t_stream, 2)}, m_length_units_per_single_tile{read_u16(t_stream)},
      m_starting_tempo{read_double(t_stream)}, m_acceleration{read_double(t_stream, 1)},
      m_tiles{read_array<TileInfo>(t_stream)}
{
}

SongInfo::SongInfo(const char* t_path)
    : SongInfo{[&]() {
          SDL_IOStream* file{SDL_IOFromFile(get_res_path(t_path).data(), "rb")};
          return SongInfo{file};
      }()}
{
}