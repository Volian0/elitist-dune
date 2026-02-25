#pragma once

#include "SongInfo.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

enum class Column : unsigned char
{
    NONE = 0b0000,
    FAR_LEFT = 0b1000,
    CENTER_LEFT = 0b0100,
    CENTER_RIGHT = 0b0010,
    FAR_RIGHT = 0b0001,
    DT_LEFT = 0b1010,
    DT_RIGHT = 0b0101 //,
    // ALL = 0b1111
};

inline float column_to_position(Column t_column)
{
    if (t_column == Column::FAR_LEFT)
    {
        return 0.0F;
    }
    if (t_column == Column::CENTER_LEFT)
    {
        return 1.0F;
    }
    if (t_column == Column::CENTER_RIGHT)
    {
        return 2.0F;
    }
    return 3.0F;
}

inline Column position_to_column(float t_position)
{
    if (t_position < 1.0F)
    {
        return Column::FAR_LEFT;
    }
    if (t_position < 2.0F)
    {
        return Column::CENTER_LEFT;
    }
    if (t_position < 3.0F)
    {
        return Column::CENTER_RIGHT;
    }
    return Column::FAR_RIGHT;
}

struct Tile
{
    unsigned id;
    float position;
    Column column;
    Column cleared_column{Column::NONE};
    float position_clicked; // for long tile
    std::chrono::steady_clock::time_point time_clicked_left;
    std::chrono::steady_clock::time_point time_clicked_right;
    bool fully_cleared_long{false};
    unsigned char finger_id{255};
};