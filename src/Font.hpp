#pragma once

#include "Utility.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <array>

struct Glyph
{
    glm::i64vec2 size{0,0};
    glm::i64vec2 bearing{0,0};
    long long advance{0};
    glm::uvec2 atlas_position{0,0};
    glm::uvec2 atlas_size{0,0};
    void from_json(const nlohmann::json& t_json)
    {
        size = {t_json.at("size_x"), t_json.at("size_y")};
        bearing = {t_json.at("bearing_x"), t_json.at("bearing_y")};
        advance = t_json.at("advance");
        atlas_position = {t_json.at("atlas_position_x"), t_json.at("atlas_position_y")};
        atlas_size = {t_json.at("atlas_size_x"), t_json.at("atlas_size_y")};
    }
};

struct Font
{
    unsigned padding;
    unsigned atlas_size;
    std::array<Glyph, 688> glyphs;

    Font(const char* t_path)
    {
        const auto buffer = file_to_buffer<char>(t_path);
        const auto t_json = nlohmann::json::parse(buffer.begin(), buffer.end());
        padding = t_json.at("padding");
        atlas_size = t_json.at("atlas_size");
        for (std::size_t i = 0; i < glyphs.size(); ++i)
        {
            glyphs.at(i).from_json(t_json.at("glyphs").at(i));
        }
    }
};