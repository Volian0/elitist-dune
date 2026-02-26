#pragma once

#include "SongInfo.hpp"
#include "Soundfont.hpp"
#include "Tile.hpp"

#include <glm/glm.hpp>

#include <chrono>
#include <deque>
#include <iostream>
#include <optional>
#include <random>
#include <vector>

struct Level
{
    Level(const char* t_file, Soundfont& t_soundfont)
        : song_info{t_file}, tempo(song_info.get_starting_tempo()), distribution(0, 11)
    {
        starting_tps = tempo;
        spawn_tiles();
        std::scoped_lock lock{t_soundfont.mutex};
        t_soundfont.all_notes_off();
        for (const auto& tile_info : song_info.get_tiles())
        {
            if (tile_info.get_type() != TileInfo::Type::EMPTY)
            {
                ++total_tiles;
            }
        }
        tile_id_tempo1 = total_tiles / 3;
        tile_id_tempo2 = total_tiles * 2 / 3;
    }

    //TODO: IN CONSTRUCTOR
    std::string song_name{"Hungarian Rhapsody No. 2"};
    std::string song_composer{"Song Composer"};
    unsigned long best_score{2026};
    int speed_modifier{0};
    unsigned total_tiles{0};

    unsigned tile_id_tempo1;
    unsigned tile_id_tempo2;

    std::chrono::steady_clock::time_point time_song_started;

    void queue_note_events(const std::chrono::steady_clock::time_point& t_time,
                           const std::vector<NoteInfo>& t_note_events, Soundfont& t_soundfont)
    {
        std::scoped_lock lock{t_soundfont.mutex};
        for (const auto& note_event : t_note_events)
        {
            t_soundfont.queued_note_events.emplace_back(QueuedNoteEvent{
                t_time + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                             std::chrono::duration<float>(float(note_event.get_tick_offset()) /
                                                          (float(song_info.get_note_ticks_per_single_tile()) * tempo))),
                note_event});
        }
    }

    float background_id = 0.0F;
    float starting_tps;

    void touch_down(const glm::vec2& t_mouse_position, unsigned char t_finger_id,
                    const std::chrono::steady_clock::time_point& t_time, Soundfont& t_soundfont)
    {
        Tile* active_tile = get_active_tile();
        if (!active_tile || game_over_column)
        {
            return;
        }
        auto& tile = *active_tile;
        const auto& tile_info = song_info.get_tiles()[tile.id];
        const glm::vec2 hit_position{glm::vec2{t_mouse_position.x, 1.0F - t_mouse_position.y} * 4.0F};
        const Column hit_column = position_to_column(hit_position.x);
        // ok hit
        const bool first_hit{tile.cleared_column == Column::NONE};
        if ((unsigned(hit_column) & unsigned(tile.column)) && !(unsigned(hit_column) & unsigned(tile.cleared_column)))
        {
            if (first_hit)
            {
                if (tile.id == 0)
                {
                    if (current_lap == 0)
                    {
                        time_song_started = t_time;
                    }
                    ++current_lap;
                }
                else if (cleared_tiles == tile_id_tempo1)
                {
                    background_id = 1.0F;
                }
                else if (cleared_tiles == tile_id_tempo2)
                {
                    background_id = 2.0F;
                }
                if (current_lap > 1 && (tile.id == 0 || cleared_tiles % total_tiles == tile_id_tempo1 ||
                                        cleared_tiles % total_tiles == tile_id_tempo2))
                {
                    tempo = new_bpm_pt2(tempo * 30.0F, 0.5F, 0.5F, current_lap >= 4) / 30.0F;
                }
                ++cleared_tiles;
            }
            protected_column = hit_column;
            score += 1;
            if (tile_info.get_type() != TileInfo::Type::LONG)
            {
                score_display = score;
                time_last_score_update = t_time;
            }
            if (tile_info.get_type() == TileInfo::Type::DOUBLE) // handle double tiles
            {
                if (first_hit) // first hit
                {
                    // tile_time_clicked = t_time;
                    queue_note_events(t_time, tile_info.get_double_tile_events()[0], t_soundfont);
                }
                else // second hit
                {
                    // float seconds_since_last_hit = (tp_new - tile.time_clicked).count();
                    std::chrono::steady_clock::time_point time_second =
                        tile_time_clicked +
                        std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float>(
                            0.5F * float(tile_info.get_unit_length()) /
                            (float(song_info.get_length_units_per_single_tile()) * tempo)));
                    queue_note_events(t_time > time_second ? t_time : time_second,
                                      tile_info.get_double_tile_events()[1], t_soundfont);
                }
                ((unsigned(hit_column) & 0b1100) ? tile.time_clicked_left : tile.time_clicked_right) = t_time;
            }
            else
            {
                tile.time_clicked_left = t_time;
                if (tile_info.get_type() == TileInfo::Type::LONG)
                {
                    tile.finger_id = t_finger_id;
                    tile.position_clicked = std::max(position + hit_position.y + 0.5F, tile.position + 0.5F);
                    if (tile.position_clicked >=
                        tile.position +
                            float(tile_info.get_unit_length()) / float(song_info.get_length_units_per_single_tile()))
                    {
                        // fully cleared
                        score += 1;
                        score_display = score;
                        time_last_score_update = t_time;
                        tile.fully_cleared_long = true;
                        tile.finger_id = 255;
                        tile.time_clicked_left = t_time;
                    }
                }
                queue_note_events(t_time, tile_info.get_events(), t_soundfont);
            }
            tile.cleared_column = Column(unsigned(tile.cleared_column) | unsigned(hit_column));
            idle = false;
            tile_time_clicked = t_time;
            if (tile.column == tile.cleared_column)
            {
                tile_full_time_clicked = t_time;
            }
        }
        else if (protected_column != hit_column && !idle) // game over
        {
            game_over_column = hit_column;
            game_over(t_time, t_soundfont);
        }
    }

    void touch_up(unsigned char t_finger_id, const std::chrono::steady_clock::time_point& t_time)
    {
        for (auto& tile : tiles)
        {
            if (tile.finger_id == t_finger_id)
            {
                score_display = score;
                time_last_score_update = t_time;
                tile.finger_id = 255;
            }
        }
    }

    std::chrono::steady_clock::time_point time_last_score_update;

    void game_over(const std::chrono::steady_clock::time_point& t_time, Soundfont& t_soundfont)
    {
        score_display = score;
        // time_last_score_update = t_time;
        time_game_over = t_time;
        position_game_over = position;
        std::scoped_lock lock{t_soundfont.mutex};
        t_soundfont.queued_note_events.clear();
        t_soundfont.all_notes_off();
        t_soundfont.note_on(53, 100);
        t_soundfont.note_on(56, 100);
        t_soundfont.note_on(60, 100);
    }

    void update(float t_delta_time, const std::chrono::steady_clock::time_point& t_time, Soundfont& t_soundfont)
    {
        if (game_over_column || idle)
        {
            if (game_over_column)
            {
                if (game_over_column == Column::NONE) // scroll to first tile
                {
                    position = glm::mix(
                        position_game_over, tiles.at(0).position,
                        glm::min(glm::sqrt(std::chrono::duration<float>(t_time - time_game_over).count()), 1.0F));
                }
                else // scroll to active tile
                {
                    Tile* tile = get_active_tile();
                    if (tile && position > tile->position)
                    {
                        position = glm::mix(
                            position_game_over, tile->position,
                            glm::min(glm::sqrt(std::chrono::duration<float>(t_time - time_game_over).count()), 1.0F));
                    }
                }
                if (std::chrono::duration<float>(t_time - time_game_over).count() >= 2.0F)
                {
                    if (revives_used >= 3)
                    {
                        reset(t_soundfont);
                    }
                    else
                    {
                        revive(t_soundfont);
                    }
                }
            }
            return;
        }
        // move tiles
        position += tempo * t_delta_time;

        // spawn new tiles
        spawn_tiles();

        // fully clear long tile
        for (auto& tile : tiles)
        {
            const auto& tile_info = song_info.get_tiles()[tile.id];
            if (tile_info.get_type() == TileInfo::Type::LONG)
            {
                if (tile.finger_id != 255)
                {
                    tile.position_clicked += tempo * t_delta_time;
                    if (tile.position_clicked >=
                        tile.position +
                            float(tile_info.get_unit_length()) / float(song_info.get_length_units_per_single_tile()))
                    {
                        // fully cleared
                        score += 1;
                        score_display = score;
                        time_last_score_update = t_time;
                        tile.fully_cleared_long = true;
                        tile.finger_id = 255;
                        tile.time_clicked_left = t_time;
                    }
                }
            }
        }

        // test for game over for uncleared tiles
        while (!tiles.empty()) // TODO: CORRECT THIS
        {
            auto& tile = tiles[0];
            const auto& tile_info = song_info.get_tiles()[tile.id];
            float length = float(tile_info.get_unit_length()) / float(song_info.get_length_units_per_single_tile());
            float length2 = length;
            if (tile_info.get_type() == TileInfo::Type::LONG)
            {
                length2 = 1.0F;
            }
            if (tile.finger_id != 255 || (tile.position) - position + length2 >= -1.0F)
            {
                break;
            }
            if (tile.cleared_column != tile.column)
            {
                game_over_column = Column::NONE;
                game_over(t_time, t_soundfont);
                return;
            }
            if (tile.finger_id != 255 || (tile.position) - position + length >= -1.0F)
            {
                break;
            }
            tiles.pop_front();
        }
    }

    Tile* get_active_tile()
    {
        for (auto& tile : tiles)
        {
            // if tile is uncleared
            if (tile.cleared_column != tile.column &&
                song_info.get_tiles()[tile.id].get_type() != TileInfo::Type::EMPTY)
            {
                return &tile;
            }
        }
        return nullptr;
    }

    void spawn_tiles()
    {
        while (spawned_tile_length <= position + 4.0F)
        {
            const TileInfo& tile_info = song_info.get_tiles()[spawned_tiles % song_info.get_tiles().size()];
            auto& tile = tiles.emplace_back();
            tile.id = spawned_tiles % song_info.get_tiles().size();
            tile.position = spawned_tile_length;
            tile.column = get_next_column(tile_info, distribution(rng));
            ++spawned_tiles;
            spawned_tile_length +=
                float(tile_info.get_unit_length()) / float(song_info.get_length_units_per_single_tile());
        }
    }

    Column get_next_column(const TileInfo& t_tile_info, unsigned short t_random_number)
    {
        if (t_tile_info.get_type() == TileInfo::Type::EMPTY)
        {
            if (t_tile_info.get_unit_length() >= song_info.get_length_units_per_single_tile())
            {
                previous_column = Column::NONE;
            }
        }
        else if (t_tile_info.get_type() == TileInfo::Type::DOUBLE)
        {
            if (previous_column == Column::NONE) // random 2
            {
                previous_column = t_random_number % 2 ? Column::DT_RIGHT : Column::DT_LEFT;
            }
            else if (unsigned(previous_column) & unsigned(Column::FAR_LEFT) ||
                     unsigned(previous_column) & unsigned(Column::CENTER_RIGHT)) // put it on the right
            {
                previous_column = Column::DT_RIGHT;
            }
            else // put it on the left
            {
                previous_column = Column::DT_LEFT;
            }
        }
        else // single and long tile
        {
            if (previous_column == Column::NONE) // random 4
            {
                if (t_random_number % 4 == 0)
                {
                    previous_column = Column::FAR_LEFT;
                }
                else if (t_random_number % 4 == 1)
                {
                    previous_column = Column::CENTER_LEFT;
                }
                else if (t_random_number % 4 == 2)
                {
                    previous_column = Column::CENTER_RIGHT;
                }
                else
                {
                    previous_column = Column::FAR_RIGHT;
                }
            }
            else if (previous_column == Column::FAR_LEFT) // random 3
            {
                if (t_random_number % 3 == 0)
                {
                    previous_column = Column::CENTER_LEFT;
                }
                else if (t_random_number % 3 == 1)
                {
                    previous_column = Column::CENTER_RIGHT;
                }
                else
                {
                    previous_column = Column::FAR_RIGHT;
                }
            }
            else if (previous_column == Column::CENTER_LEFT) // random 3
            {
                if (t_random_number % 3 == 0)
                {
                    previous_column = Column::FAR_LEFT;
                }
                else if (t_random_number % 3 == 1)
                {
                    previous_column = Column::CENTER_RIGHT;
                }
                else
                {
                    previous_column = Column::FAR_RIGHT;
                }
            }
            else if (previous_column == Column::CENTER_RIGHT) // random 3
            {
                if (t_random_number % 3 == 0)
                {
                    previous_column = Column::FAR_LEFT;
                }
                else if (t_random_number % 3 == 1)
                {
                    previous_column = Column::CENTER_LEFT;
                }
                else
                {
                    previous_column = Column::FAR_RIGHT;
                }
            }
            else if (previous_column == Column::FAR_RIGHT) // random 3
            {
                if (t_random_number % 3 == 0)
                {
                    previous_column = Column::FAR_LEFT;
                }
                else if (t_random_number % 3 == 1)
                {
                    previous_column = Column::CENTER_LEFT;
                }
                else
                {
                    previous_column = Column::CENTER_RIGHT;
                }
            }
            else if (previous_column == Column::DT_LEFT) // random 2
            {
                previous_column = t_random_number % 2 ? Column::CENTER_LEFT : Column::FAR_RIGHT;
            }
            else if (previous_column == Column::DT_RIGHT) // random 2
            {
                previous_column = t_random_number % 2 ? Column::FAR_LEFT : Column::CENTER_RIGHT;
            }
        }
        return previous_column;
    }

    SongInfo song_info;
    float tempo{0.0};
    std::optional<Column> game_over_column;
    Column previous_column{Column::NONE};
    Column protected_column{Column::NONE};
    float position{-1.0F};
    std::deque<Tile> tiles;
    bool idle{true};
    unsigned long spawned_tiles{0};
    float spawned_tile_length{0.0F};
    unsigned long cleared_tiles{0};
    unsigned long score{0};
    unsigned long score_display{0};
    unsigned current_lap{0};
    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<unsigned short> distribution;
    std::chrono::steady_clock::time_point time_game_over;
    float position_game_over;
    unsigned char revives_used{0};

    void revive(Soundfont& t_soundfont)
    {
        for (auto& tile : tiles)
        {
            tile.finger_id = 255;
        }
        ++revives_used;
        game_over_column = std::nullopt;
        idle = true;
        Tile* active_tile = get_active_tile();
        if (active_tile == nullptr)
        {
            std::abort();
        }
        position = active_tile->position - 1.0F;
        spawn_tiles();
        std::scoped_lock lock{t_soundfont.mutex};
        t_soundfont.all_notes_off();
    }

    std::chrono::steady_clock::time_point tile_time_clicked{};      // for double tile
    std::chrono::steady_clock::time_point tile_full_time_clicked{}; // for effects

    float new_bpm_pt2(unsigned t_current_bpm, float t_current_beats, float t_current_part_base_beats,
                      bool t_reached_fourth_lap)
    {
        // TODO: is this the correct order?
        const volatile float t_unknown = t_reached_fourth_lap ? 130.0F : 100.0F;

        const volatile float v1 = std::max(0.0F, t_current_beats);
        const volatile float v2 = t_current_bpm;
        const volatile float n = v2 / v1;
        const volatile float v3 = (n - t_unknown);
        const volatile float v4 = 0.001F * v3;
        const volatile float a = 1.3F - v4;
        const volatile float r = n / 60.0F;
        const volatile float v5 = a < 1.04F ? 1.04F : a;
        const volatile float v6 = r * v5;
        const volatile float v7 = 60.0F * v6;
        const volatile float v8 = v7 * t_current_part_base_beats;
        return v8;
    }

    void reset(Soundfont& t_soundfont)
    {
        tempo = song_info.get_starting_tempo();
        game_over_column = std::nullopt;
        previous_column = protected_column = Column::NONE;
        position = -1.0F;
        tiles.clear();
        idle = true;
        spawned_tiles = 0;
        spawned_tile_length = 0.0F;
        cleared_tiles = 0;
        score = 0;
        score_display = 0;
        current_lap = 0;
        revives_used = 0;
        background_id = 0.0F;
        speed_modifier = 0;
        spawn_tiles();
        std::scoped_lock lock{t_soundfont.mutex};
        t_soundfont.all_notes_off();
        // tempo = 10.0F; //TODO: TEMPORARY, DELETE LATER
    }
};