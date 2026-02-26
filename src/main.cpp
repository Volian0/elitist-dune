#define TSF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL

#include "Font.hpp"
#include "Level.hpp"
#include "SongInfo.hpp"
#include "Soundfont.hpp"
#include "Stream.hpp"
#include "Tile.hpp"
#include "Utility.hpp"

#include <GLES3/gl3.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <stb/stb_image.h>
#include <tsf/tsf.h>

#include <chrono>
#include <cstdlib>
#include <iostream> 
#include <string>
#include <thread>
#include <vector>

template <typename T> inline void emplace_vec(std::vector<GLfloat>& t_vector, const T& t_vec)
{
    for (unsigned i = 0; i < t_vec.length(); ++i)
    {
        t_vector.emplace_back(t_vec[i]);
    }
}

glm::mat4 create_ortographic_matrix(const glm::vec2& t_desired_resolution, float t_current_aspect_ratio,
                                    const glm::vec2& t_desired_position = {0.0, 0.0})
{
    const float desired_aspect_ratio = t_desired_resolution.x / t_desired_resolution.y;
    glm::vec2 resolution_delta{0.0, 0.0};
    if (desired_aspect_ratio > t_current_aspect_ratio)
    {
        resolution_delta.y =
            t_desired_resolution.y * desired_aspect_ratio / t_current_aspect_ratio - t_desired_resolution.y;
    }
    else
    {
        resolution_delta.x =
            t_desired_resolution.x * t_current_aspect_ratio / desired_aspect_ratio - t_desired_resolution.x;
    }
    const glm::vec2 weight1 = (t_desired_position + 1.0F) * 0.5F;
    const glm::vec2 weight2 = 1.0F - weight1;
    return glm::ortho(-resolution_delta.x * weight1.x, t_desired_resolution.x + resolution_delta.x * weight2.x,
                      t_desired_resolution.y + resolution_delta.y * weight2.y, -resolution_delta.y * weight1.y, -1.0F,
                      1.0F);
}

int main(int, char*[])
{
    constexpr const char* APP_NAME{"Elitist Dune"};
    SDL_SetAppMetadata(APP_NAME, "0.1.2", "com.volian.elitistdune");
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "Portrait");
    constexpr unsigned SAMPLE_RATE{44100};
    MIX_Init();
    MIX_Mixer* mixer{[]() {
        SDL_AudioSpec audio_spec{.format = SDL_AUDIO_F32, .channels = 2, .freq = SAMPLE_RATE};
        return MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec);
    }()};
    // SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, APP_NAME, SDL_GetAndroidInternalStoragePath(), nullptr);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    glm::uvec2 resolution{360, 640}; //TODO: THIS IS RESOLUTION
    //glm::uvec2 resolution{720, 720}; //TODO: WRONG RESOLUTION
    Font font{"texture/font.json"};
    SDL_Window* window{SDL_CreateWindow(APP_NAME, resolution.x, resolution.y,
                                        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL)}; //TODO: RESIZABLE!!
    SDL_GLContext context{SDL_GL_CreateContext(window)};
    SDL_GL_SetSwapInterval(1);
    glClearColor(0.5, 0.5, 0.5, 1.0);

    // create texture
    GLuint texture_atlas{0};
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texture_atlas);
    glBindTexture(GL_TEXTURE_2D, texture_atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    {
        const auto buffer = file_to_buffer("texture/atlas_linear.png");
        int x, y, c;
        const auto image_data = stbi_load_from_memory(buffer.data(), buffer.size(), &x, &y, &c, 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
        stbi_image_free(image_data);
    }

    // create basic shader
    GLuint shader_basic_vertex{glCreateShader(GL_VERTEX_SHADER)};
    {
        const auto buffer = file_to_buffer<char>("shader/basic.vert");
        const char* string = buffer.data();
        GLint size(buffer.size());
        glShaderSource(shader_basic_vertex, 1, &string, &size);
        glCompileShader(shader_basic_vertex);
    }
    GLuint shader_basic_fragment{glCreateShader(GL_FRAGMENT_SHADER)};
    {
        const auto buffer = file_to_buffer<char>("shader/basic.frag");
        const char* string = buffer.data();
        GLint size(buffer.size());
        glShaderSource(shader_basic_fragment, 1, &string, &size);
        glCompileShader(shader_basic_fragment);
    }
    GLuint program_basic{glCreateProgram()};
    glAttachShader(program_basic, shader_basic_vertex);
    glAttachShader(program_basic, shader_basic_fragment);
    glLinkProgram(program_basic);
    glDetachShader(program_basic, shader_basic_vertex);
    glDetachShader(program_basic, shader_basic_fragment);
    glDeleteShader(shader_basic_vertex);
    glDeleteShader(shader_basic_fragment);
    glUseProgram(program_basic);
    glUniform1i(glGetUniformLocation(program_basic, "u_sampler"), 0);

    // create basic VAO
    GLuint vao_basic{0};
    glGenVertexArrays(1, &vao_basic);
    glBindVertexArray(vao_basic);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    GLuint vbo_basic{0};
    glGenBuffers(1, &vbo_basic);
    GLuint ebo_basic{0};
    glGenBuffers(1, &ebo_basic);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_basic);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_basic);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat),
                          reinterpret_cast<const void*>(0 * sizeof(GLfloat)));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat),
                          reinterpret_cast<const void*>(4 * sizeof(GLfloat)));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat),
                          reinterpret_cast<const void*>(6 * sizeof(GLfloat)));
    std::vector<GLfloat> buffer_vbo_basic;
    std::vector<GLushort> buffer_ebo_basic;
    constexpr unsigned MAX_QUADS_BASIC{5000};
    glBufferData(GL_ARRAY_BUFFER, MAX_QUADS_BASIC * sizeof(GLfloat) * 10 * 4, nullptr, GL_DYNAMIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_QUADS_BASIC * sizeof(GLushort) * 6, nullptr, GL_DYNAMIC_DRAW);

    // create background shader
    GLuint shader_background_vertex{glCreateShader(GL_VERTEX_SHADER)};
    {
        const auto buffer = file_to_buffer<char>("shader/background.vert");
        const char* string = buffer.data();
        GLint size(buffer.size());
        glShaderSource(shader_background_vertex, 1, &string, &size);
        glCompileShader(shader_background_vertex);
    }
    GLuint shader_background_fragment{glCreateShader(GL_FRAGMENT_SHADER)};
    {
        const auto buffer = file_to_buffer<char>("shader/background.frag");
        const char* string = buffer.data();
        GLint size(buffer.size());
        glShaderSource(shader_background_fragment, 1, &string, &size);
        glCompileShader(shader_background_fragment);
    }
    GLuint program_background{glCreateProgram()};
    glAttachShader(program_background, shader_background_vertex);
    glAttachShader(program_background, shader_background_fragment);
    glLinkProgram(program_background);
    glDetachShader(program_background, shader_background_vertex);
    glDetachShader(program_background, shader_background_fragment);
    glDeleteShader(shader_background_vertex);
    glDeleteShader(shader_background_fragment);
    glUseProgram(program_background);
    GLint uniform_location_background_aspect_ratio = glGetUniformLocation(program_background, "u_aspect_ratio");
    GLint uniform_location_background_time = glGetUniformLocation(program_background, "u_time");
    GLint uniform_location_background_id = glGetUniformLocation(program_background, "u_id");
    glUniform1f(uniform_location_background_aspect_ratio, 9.0F / 16.0F);
    glUniform1f(uniform_location_background_time, 0.0);
    glUniform1f(uniform_location_background_id, 0.0);

    // create background VAO
    GLuint vao_background{0};
    glGenVertexArrays(1, &vao_background);
    glBindVertexArray(vao_background);
    glEnableVertexAttribArray(0);
    GLuint vbo_background{0};
    glGenBuffers(1, &vbo_background);
    GLuint ebo_background{0};
    glGenBuffers(1, &ebo_background);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_background);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_background);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat),
                          reinterpret_cast<const void*>(0 * sizeof(GLfloat)));
    constexpr std::array<GLfloat, 8> buffer_vbo_background{-1.0F, -1.0F, 1.0F, -1.0F, 1.0F, 1.0F, -1.0F, 1.0F};
    constexpr std::array<GLubyte, 6> buffer_ebo_background{0, 1, 2, 2, 3, 0};
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 2 * 4, buffer_vbo_background.data(), GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 6, buffer_ebo_background.data(), GL_STATIC_DRAW);
    // end of background shader

    // create helper shader
    GLuint shader_helper_vertex{glCreateShader(GL_VERTEX_SHADER)};
    {
        const auto buffer = file_to_buffer<char>("shader/helper.vert");
        const char* string = buffer.data();
        GLint size(buffer.size());
        glShaderSource(shader_helper_vertex, 1, &string, &size);
        glCompileShader(shader_helper_vertex);
    }
    GLuint shader_helper_fragment{glCreateShader(GL_FRAGMENT_SHADER)};
    {
        const auto buffer = file_to_buffer<char>("shader/helper.frag");
        const char* string = buffer.data();
        GLint size(buffer.size());
        glShaderSource(shader_helper_fragment, 1, &string, &size);
        glCompileShader(shader_helper_fragment);
    }
    GLuint program_helper{glCreateProgram()};
    glAttachShader(program_helper, shader_helper_vertex);
    glAttachShader(program_helper, shader_helper_fragment);
    glLinkProgram(program_helper);
    glDetachShader(program_helper, shader_helper_vertex);
    glDetachShader(program_helper, shader_helper_fragment);
    glDeleteShader(shader_helper_vertex);
    glDeleteShader(shader_helper_fragment);
    glUseProgram(program_helper);
    GLint uniform_location_helper_aspect_ratio = glGetUniformLocation(program_helper, "u_aspect_ratio");
    GLint uniform_location_helper_time = glGetUniformLocation(program_helper, "u_time");
    GLint uniform_location_helper_tile_length = glGetUniformLocation(program_helper, "u_tile_length");
    GLint uniform_location_helper_alpha = glGetUniformLocation(program_helper, "u_alpha");
    glUniform1f(uniform_location_helper_aspect_ratio, 9.0F / 16.0F);
    glUniform1f(uniform_location_helper_time, 0.0);
    glUniform1f(uniform_location_helper_tile_length, 1.0);
    glUniform1f(uniform_location_helper_alpha, 1.0);

    // create helper VAO
    GLuint vao_helper{0};
    glGenVertexArrays(1, &vao_helper);
    glBindVertexArray(vao_helper);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    GLuint vbo_helper{0};
    glGenBuffers(1, &vbo_helper);
    GLuint ebo_helper{0};
    glGenBuffers(1, &ebo_helper);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_helper);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_helper);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                          reinterpret_cast<const void*>(0 * sizeof(GLfloat)));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                          reinterpret_cast<const void*>(2 * sizeof(GLfloat)));
    std::array<GLfloat, 32> buffer_vbo_helper;
    constexpr std::array<GLubyte, 12> buffer_ebo_helper{2, 1, 0, 0, 3, 2, 2 + 4, 1 + 4, 0 + 4, 0 + 4, 3 + 4, 2 + 4};
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4 * 4 * 2, nullptr, GL_DYNAMIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 6 * 2, buffer_ebo_helper.data(), GL_STATIC_DRAW);
    // end of helper shader

    // create SDF texture
    GLuint texture_font{0};
    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &texture_font);
    glBindTexture(GL_TEXTURE_2D, texture_font);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    {
        const auto buffer = file_to_buffer("texture/font.png");
        int x, y, c;
        const auto image_data = stbi_load_from_memory(buffer.data(), buffer.size(), &x, &y, &c, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, x, y, 0, GL_RED, GL_UNSIGNED_BYTE, image_data);
        stbi_image_free(image_data);
    }

    // create sdf shader
    GLuint shader_sdf_vertex{glCreateShader(GL_VERTEX_SHADER)};
    {
        const auto buffer = file_to_buffer<char>("shader/sdf.vert");
        const char* string = buffer.data();
        GLint size(buffer.size());
        glShaderSource(shader_sdf_vertex, 1, &string, &size);
        glCompileShader(shader_sdf_vertex);
    }
    GLuint shader_sdf_fragment{glCreateShader(GL_FRAGMENT_SHADER)};
    {
        const auto buffer = file_to_buffer<char>("shader/sdf.frag");
        const char* string = buffer.data();
        GLint size(buffer.size());
        glShaderSource(shader_sdf_fragment, 1, &string, &size);
        glCompileShader(shader_sdf_fragment);
    }
    GLuint program_sdf{glCreateProgram()};
    glAttachShader(program_sdf, shader_sdf_vertex);
    glAttachShader(program_sdf, shader_sdf_fragment);
    glLinkProgram(program_sdf);
    glDetachShader(program_sdf, shader_sdf_vertex);
    glDetachShader(program_sdf, shader_sdf_fragment);
    glDeleteShader(shader_sdf_vertex);
    glDeleteShader(shader_sdf_fragment);
    glUseProgram(program_sdf);
    glUniform1i(glGetUniformLocation(program_sdf, "u_sampler"), 1);

    // create sdf VAO
    GLuint vao_sdf{0};
    glGenVertexArrays(1, &vao_sdf);
    glBindVertexArray(vao_sdf);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    GLuint vbo_sdf{0};
    glGenBuffers(1, &vbo_sdf);
    GLuint ebo_sdf{0};
    glGenBuffers(1, &ebo_sdf);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_sdf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_sdf);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat),
                          reinterpret_cast<const void*>(0 * sizeof(GLfloat)));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat),
                          reinterpret_cast<const void*>(2 * sizeof(GLfloat)));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat),
                          reinterpret_cast<const void*>(4 * sizeof(GLfloat)));
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat),
                          reinterpret_cast<const void*>(8 * sizeof(GLfloat)));
    std::vector<GLfloat> buffer_vbo_sdf;
    std::vector<GLushort> buffer_ebo_sdf;
    constexpr unsigned MAX_QUADS_SDF{5000};
    glBufferData(GL_ARRAY_BUFFER, MAX_QUADS_SDF * sizeof(GLfloat) * 9 * 4, nullptr, GL_DYNAMIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_QUADS_SDF * sizeof(GLushort) * 6, nullptr, GL_DYNAMIC_DRAW);

    bool in_background{false};
    Soundfont soundfont{"soundfont/piano.sf2", SAMPLE_RATE};
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");
    MIX_SetPostMixCallback(mixer, soundfont_callback, &soundfont);
    // glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    // glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    unsigned ebo_offset{0};
    unsigned ebo_offset_sdf{0};
    float aspect_ratio{9.0F / 16.0F};
    glm::mat4 matrix_test = create_ortographic_matrix(resolution, aspect_ratio);
    glm::mat4 matrix_aspect = create_ortographic_matrix({1.0F, 1.0F / aspect_ratio}, aspect_ratio);
    const auto draw_quad = [&](const glm::uvec2& t_texture_position, const glm::uvec2& t_texture_size,
                               const glm::vec2& t_position, const glm::vec2& t_size, const glm::mat4& t_matrix,
                               const glm::vec4& t_top_tint = glm::vec4{1.0F},
                               const glm::vec4& t_bottom_tint = glm::vec4{1.0F}) {
        constexpr float ATLAS_SIZE{1024.0F};
        const glm::vec2 texture_a = glm::vec2(t_texture_position) / ATLAS_SIZE;
        const glm::vec2 texture_c = glm::vec2(t_texture_position + t_texture_size) / ATLAS_SIZE;
        const glm::vec2 texture_b = glm::vec2(texture_c.x, texture_a.y);
        const glm::vec2 texture_d = glm::vec2(texture_a.x, texture_c.y);
        const glm::vec4 position_a = t_matrix * glm::vec4(t_position, 0.0, 1.0);
        const glm::vec4 position_b = t_matrix * glm::vec4(t_position.x + t_size.x, t_position.y, 0.0, 1.0);
        const glm::vec4 position_c = t_matrix * glm::vec4(t_position + t_size, 0.0, 1.0);
        const glm::vec4 position_d = t_matrix * glm::vec4(t_position.x, t_position.y + t_size.y, 0.0, 1.0);
        emplace_vec(buffer_vbo_basic, position_a);
        emplace_vec(buffer_vbo_basic, texture_a);
        emplace_vec(buffer_vbo_basic, t_top_tint);
        emplace_vec(buffer_vbo_basic, position_b);
        emplace_vec(buffer_vbo_basic, texture_b);
        emplace_vec(buffer_vbo_basic, t_top_tint);
        emplace_vec(buffer_vbo_basic, position_c);
        emplace_vec(buffer_vbo_basic, texture_c);
        emplace_vec(buffer_vbo_basic, t_bottom_tint);
        emplace_vec(buffer_vbo_basic, position_d);
        emplace_vec(buffer_vbo_basic, texture_d);
        emplace_vec(buffer_vbo_basic, t_bottom_tint);
        buffer_ebo_basic.emplace_back(ebo_offset + 2);
        buffer_ebo_basic.emplace_back(ebo_offset + 1);
        buffer_ebo_basic.emplace_back(ebo_offset + 0);
        buffer_ebo_basic.emplace_back(ebo_offset + 0);
        buffer_ebo_basic.emplace_back(ebo_offset + 3);
        buffer_ebo_basic.emplace_back(ebo_offset + 2);
        ebo_offset += 4;
    };
    const auto draw_quad_sdf = [&](const glm::uvec2& t_texture_position, const glm::uvec2& t_texture_size,
                                   unsigned t_atlas_size, const glm::vec2& t_position, const glm::vec2& t_size,
                                   const glm::mat4& t_matrix, const glm::vec4& t_top_tint = glm::vec4{1.0F},
                                   const glm::vec4& t_bottom_tint = glm::vec4{1.0F}, float t_thickness = 0.5F) {
        constexpr float ATLAS_SIZE{1024.0F};
        const glm::vec2 texture_a = glm::vec2(t_texture_position) / float(t_atlas_size);
        const glm::vec2 texture_c = glm::vec2(t_texture_position + t_texture_size) / float(t_atlas_size);
        const glm::vec2 texture_b = glm::vec2(texture_c.x, texture_a.y);
        const glm::vec2 texture_d = glm::vec2(texture_a.x, texture_c.y);
        const glm::vec2 position_a = t_matrix * glm::vec4(t_position, 0.0, 1.0);
        const glm::vec2 position_b = t_matrix * glm::vec4(t_position.x + t_size.x, t_position.y, 0.0, 1.0);
        const glm::vec2 position_c = t_matrix * glm::vec4(t_position + t_size, 0.0, 1.0);
        const glm::vec2 position_d = t_matrix * glm::vec4(t_position.x, t_position.y + t_size.y, 0.0, 1.0);
        emplace_vec(buffer_vbo_sdf, position_a);
        emplace_vec(buffer_vbo_sdf, texture_a);
        emplace_vec(buffer_vbo_sdf, t_top_tint);
        buffer_vbo_sdf.emplace_back(t_thickness);
        emplace_vec(buffer_vbo_sdf, position_b);
        emplace_vec(buffer_vbo_sdf, texture_b);
        emplace_vec(buffer_vbo_sdf, t_top_tint);
        buffer_vbo_sdf.emplace_back(t_thickness);
        emplace_vec(buffer_vbo_sdf, position_c);
        emplace_vec(buffer_vbo_sdf, texture_c);
        emplace_vec(buffer_vbo_sdf, t_bottom_tint);
        buffer_vbo_sdf.emplace_back(t_thickness);
        emplace_vec(buffer_vbo_sdf, position_d);
        emplace_vec(buffer_vbo_sdf, texture_d);
        emplace_vec(buffer_vbo_sdf, t_bottom_tint);
        buffer_vbo_sdf.emplace_back(t_thickness);
        buffer_ebo_sdf.emplace_back(ebo_offset_sdf + 2);
        buffer_ebo_sdf.emplace_back(ebo_offset_sdf + 1);
        buffer_ebo_sdf.emplace_back(ebo_offset_sdf + 0);
        buffer_ebo_sdf.emplace_back(ebo_offset_sdf + 0);
        buffer_ebo_sdf.emplace_back(ebo_offset_sdf + 3);
        buffer_ebo_sdf.emplace_back(ebo_offset_sdf + 2);
        ebo_offset_sdf += 4;
    };
    const auto draw_text_debug = [&](const glm::vec2& t_position, std::string_view t_text, const glm::mat4& t_matrix,
                                     float t_scale = 1.0F, const glm::vec4& t_tint = glm::vec4{1.0F}) {
        constexpr glm::uvec2 CHAR_SIZE{16, 32};
        const glm::vec2 CHAR_SIZE_SCALED{glm::vec2(CHAR_SIZE) * t_scale};
        constexpr unsigned COLUMNS{16};
        constexpr glm::uvec2 ATLAS_OFFSET{0, 512};
        glm::vec2 position = t_position;
        for (char c : t_text)
        {
            if (c != ' ')
            {
                unsigned char_index = c - ' ';
                unsigned atlas_x = ATLAS_OFFSET.x + char_index % COLUMNS * CHAR_SIZE.x;
                unsigned atlas_y = ATLAS_OFFSET.y + char_index / COLUMNS * CHAR_SIZE.y;
                draw_quad({atlas_x, atlas_y}, CHAR_SIZE, position, CHAR_SIZE_SCALED, t_matrix, t_tint);
            }
            position.x += CHAR_SIZE_SCALED.x;
        }
    };
    const auto draw_text_ascii = [&](const Font& t_font, const glm::vec2& t_position, std::string_view t_text,
                                     const glm::mat4& t_matrix, float t_scale = 1.0F,
                                     const glm::vec4& t_tint_top = glm::vec4{1.0F},
                                     const glm::vec4& t_tint_bottom = glm::vec4{1.0F}, float t_thickness = 0.5F) {
        glm::vec2 pen_position{t_position};
        for (char c : t_text) // TODO: at() -> [], const auto& glyph instead of at
        {
            const auto& glyph = t_font.glyphs.at(c);
            // std::cout << "pen:" << glm::to_string(pen_position) << std::endl;
            // TODO: VERY BAD
            glm::vec2 render_position = pen_position;
            render_position += glm::vec2(glyph.bearing.x, -glyph.bearing.y) * t_scale;
            glm::vec2 unpadded_render_size = glm::vec2(glyph.size) * t_scale;
            glm::vec2 padded_render_size =
                unpadded_render_size * (glm::vec2(glyph.atlas_size - t_font.padding * 2) / glm::vec2(glyph.atlas_size));
            glm::vec2 offset = (unpadded_render_size - padded_render_size);
            render_position -= (unpadded_render_size - padded_render_size) * 0.5F;
            // std::cout << "render:" << glm::to_string(render_position) << std::endl;
            // std::cout << glm::to_string((padded_render_size-unpadded_render_size) * 0.5F) << std::endl;
            pen_position.x += glyph.advance * t_scale;
            draw_quad_sdf(glyph.atlas_position, glyph.atlas_size, t_font.atlas_size, render_position,
                          unpadded_render_size * (unpadded_render_size / padded_render_size), t_matrix, t_tint_top,
                          t_tint_bottom, t_thickness);
        }
    };
    const auto get_length_text_ascii = [](const Font& t_font, std::string_view t_text, float t_scale = 1.0F) {
        float length{0.0F};
        for (char c : t_text) // TODO: at() -> [], const auto& glyph instead of at
        {
            const auto& glyph = t_font.glyphs.at(c);
            length += glyph.advance * t_scale;
        }
        return length;
    };
    glm::mat4 matrix_level{glm::ortho(0.0F, 4.0F, 4.0F, 0.0F, -1.0F, 1.0F)};
    Level level{"song/test3.usong", soundfont};
    auto tp_old = std::chrono::steady_clock::now();
    const auto tp_start = tp_old;
    constexpr glm::vec4 CLEAR_TILE{0.1F, 0.6F, 1.0F, 1.0F};
    constexpr glm::vec4 CLEAR_BLUE{0.05F, 0.5F, 1.0F, 1.0F};
    constexpr glm::vec4 CLEAR_BLUE_CLEAR{0.05F, 0.5F, 1.0F, 0.666F};
    constexpr glm::vec4 TOP_BLUE{0.03F, 0.4F, 0.9F, 1.0F};
    constexpr glm::vec4 BOTTOM_BLUE{0.0F, 0.2F, 0.75F, 1.0F};
    constexpr glm::vec4 BLACK{0.0F, 0.0F, 0.0F, 1.0F};
    constexpr glm::vec4 BLACK_CLEAR{0.0F, 0.0F, 0.0F, 0.5};
    const auto draw_single_tile = [&](float t_column_position, float t_position_y, float t_length,
                                      std::optional<float> t_clear_progress = std::nullopt) {
        // constexpr float border = 1.0F/16.0F;
        // constexpr float animation_time = 0.0666F;
        constexpr float border = 1.0F / 16.0F;
        constexpr float animation_time = 0.05F;
        if (!t_clear_progress)
        {
            draw_quad({460, 511}, {0, 0}, {t_column_position, t_position_y}, {1.0F, t_length}, matrix_level, BLACK,
                      BLACK);
        }
        else
        {
            float actual_border = (1.0F - glm::min(*t_clear_progress / animation_time, 1.0F)) * border;
            draw_quad({460, 511}, {0, 0}, {t_column_position, t_position_y}, {1.0F, t_length}, matrix_level,
                      BLACK_CLEAR, BLACK_CLEAR);
            if (actual_border > 0.0F) // TODO: BORDER -> ACTUAL BORDER
            {
                draw_quad({460, 511}, {0, 0}, {t_column_position, t_position_y}, {actual_border, t_length},
                          matrix_level, BLACK, BLACK);
                draw_quad({460, 511}, {0, 0}, {t_column_position + 1.0F - actual_border, t_position_y},
                          {actual_border, t_length}, matrix_level, BLACK, BLACK);
                draw_quad({460, 511}, {0, 0}, {t_column_position, t_position_y}, {1.0F, actual_border * aspect_ratio},
                          matrix_level, BLACK, BLACK);
                draw_quad({460, 511}, {0, 0},
                          {t_column_position, t_position_y + t_length - actual_border * aspect_ratio},
                          {1.0F, actual_border * aspect_ratio}, matrix_level, BLACK, BLACK);
            }
        }
    };
    while (true)
    {
        const auto tp_new = std::chrono::steady_clock::now();
        const float delta_time = std::chrono::duration<float>(tp_new - tp_old).count();
        const float total_time = std::chrono::duration<float>(tp_new - tp_start).count();
        const auto ticks_ms = SDL_GetTicks();
        tp_old = tp_new;
        for (SDL_Event event; SDL_PollEvent(&event);)
        {
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_TERMINATING)
            {
                std::quick_exit(0);
            }
            if (event.type == SDL_EVENT_WILL_ENTER_BACKGROUND)
            {
                in_background = true;
            }
            else if (event.type == SDL_EVENT_DID_ENTER_FOREGROUND)
            {
                in_background = false;
            }
            else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
            {
                resolution = {event.window.data1, event.window.data2};
                aspect_ratio = static_cast<float>(resolution.x) / static_cast<float>(resolution.y);
                glUseProgram(program_background);
                glUniform1f(uniform_location_background_aspect_ratio, aspect_ratio);
                glUseProgram(program_helper);
                glUniform1f(uniform_location_helper_aspect_ratio, aspect_ratio);
                glViewport(0, 0, resolution.x, resolution.y);
                matrix_test = create_ortographic_matrix(resolution, aspect_ratio);
                matrix_aspect = create_ortographic_matrix({1.0F, 1.0F / aspect_ratio}, aspect_ratio);
            }
            else if (event.type == SDL_EVENT_FINGER_DOWN)
            {
                if ((event.tfinger.y > 0.75F || event.tfinger.y < 0.25F) && level.idle)
                {
                    if (event.tfinger.y < 0.25F && event.tfinger.x < 0.25F)
                    {
                        int new_speed_modifier = level.speed_modifier - 1;
                        float new_tps = level.starting_tps + float(new_speed_modifier) * 0.2F;
                        if (new_tps >= 1.0F)
                        {
                            level.speed_modifier = new_speed_modifier;
                            level.tempo = new_tps;
                        }
                    }
                    else if (event.tfinger.y < 0.25F && event.tfinger.x > 0.75F)
                    {
                        int new_speed_modifier = level.speed_modifier + 1;
                        float new_tps = level.starting_tps + float(new_speed_modifier) * 0.2F;
                        if (new_tps <= 20.0F)
                        {
                            level.speed_modifier = new_speed_modifier;
                            level.tempo = new_tps;
                        }
                    }
                    /*if (level.revives_used == 0 && level.tempo <= 20.0F) //TEMPORARY
                    {
                        level.tempo += 0.2F;
                    }*/
                }
                else
                {
                    level.touch_down({event.tfinger.x, event.tfinger.y}, event.tfinger.fingerID, tp_new, soundfont);
                }
                // soundfont.all_notes_off();
                // soundfont.note_on(50, 100);
            }
            else if (event.type == SDL_EVENT_FINGER_UP)
            {
                level.touch_up(event.tfinger.fingerID, tp_new);
            }
        }
        if (in_background)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            std::this_thread::yield();
            continue;
        }
        buffer_vbo_basic.clear();
        buffer_ebo_basic.clear();
        ebo_offset = 0;
        buffer_vbo_sdf.clear();
        buffer_ebo_sdf.clear();
        ebo_offset_sdf = 0;
        // update the game
        level.update(delta_time, tp_new, soundfont);

        // draw_quad({1, 1}, {446, 510}, {0, 0}, {4, 4}, matrix_level);

        // draw tiles
        for (unsigned column_i = 1; column_i < 4; ++column_i)
        {
            float size = 0.02F;
            draw_quad({449, 511}, {9, 0}, {float(column_i) - size * 0.5F, 0.0F}, {size, 4.0F}, matrix_level);
        }
        for (const auto& tile : level.tiles)
        {
            if (level.game_over_column == Column::NONE && ticks_ms / 250 % 2 && &tile == &level.tiles[0])
            {
                continue;
            }
            // 460,511 0,0
            float length = float(level.song_info.get_tiles()[tile.id].get_unit_length()) /
                           float(level.song_info.get_length_units_per_single_tile());
            const auto type = level.song_info.get_tiles()[tile.id].get_type();
            float column_position = column_to_position(tile.column);
            float time_left = std::chrono::duration<float>(tp_new - tile.time_clicked_left).count();
            if (type == TileInfo::Type::LONG)
            {
                if (tile.fully_cleared_long)
                {
                    float alpha = (1.0F - glm::min(time_left / 0.1F, 1.0F));
                    glm::vec4 color = CLEAR_BLUE_CLEAR;
                    color.a += alpha * 0.333F;
                    draw_quad({460, 511}, {0, 0}, {column_position, -(tile.position - level.position - 4.0F) - length},
                              {1.0F, length}, matrix_level, color, color);
                }
                else
                {
                    float extra_length = length - 1.5F;
                    float circle_height = 0.5 * aspect_ratio;
                    // float how_much_cleared = 0.1F;
                    // 449,1  62,62
                    draw_quad({460, 511}, {0, 0}, {column_position, -(tile.position - level.position - 4.0F) - length},
                              {1.0F, extra_length}, matrix_level, TOP_BLUE, BOTTOM_BLUE);
                    draw_quad({460, 511}, {0, 0},
                              {column_position, -(tile.position - level.position - 4.0F) - length + extra_length},
                              {1.0F, 1.0F}, matrix_level, BOTTOM_BLUE, glm::vec4{0.0F, 0.0F, 0.0F, 1.0F});
                    draw_quad(
                        {460, 511}, {0, 0},
                        {column_position, -(tile.position - level.position - 4.0F) - length + extra_length + 1.0F},
                        {1.0F, 0.5F}, matrix_level, glm::vec4{0.0F, 0.0F, 0.0F, 1.0F},
                        glm::vec4{0.0F, 0.0F, 0.0F, 1.0F});
                    if (tile.cleared_column == Column::NONE)
                    {
                        draw_quad({449, 1}, {62, 62},
                                  {column_position + 0.25, -(tile.position - level.position - 4.0F) - length +
                                                               extra_length + 1.0F - circle_height * 0.5F},
                                  {0.5F, circle_height}, matrix_level, glm::vec4{0.0F, 1.0F, 1.0F, 1.0F},
                                  glm::vec4{0.0F, 1.0F, 1.0F, 1.0F});
                    }
                    else
                    {
                        float delta = tile.position_clicked - tile.position;
                        float clearer = aspect_ratio * 0.25F;
                        float delta_minus_clearer = delta - clearer;
                        draw_quad({450, 100}, {128, 64},
                                  {column_position, -(tile.position - level.position - 4.0F) - delta}, {1.0F, clearer},
                                  matrix_level, CLEAR_BLUE, CLEAR_BLUE);
                        draw_quad({460, 511}, {0, 0},
                                  {column_position, -(tile.position - level.position - 4.0F) - delta_minus_clearer},
                                  {1.0F, delta_minus_clearer}, matrix_level, CLEAR_BLUE, TOP_BLUE);
                    }
                }

                // draw_quad({460,511}, {0, 0}, {float(tile.id%4), -(tile.position-level.position - 4.0F) -
                // how_much_cleared}, {1.0F, how_much_cleared}, matrix_level, CLEAR_BLUE, TOP_BLUE);
            }
            else if (type == TileInfo::Type::SINGLE)
            {
                draw_single_tile(column_position, -(tile.position - level.position - 4.0F) - length, length,
                                 tile.cleared_column != Column::NONE ? std::optional<float>(time_left) : std::nullopt);
                /*if (tile.cleared_column == Column::NONE)
                {
                    draw_quad({460, 511}, {0, 0}, {column_position, -(tile.position - level.position - 4.0F) - length},
                              {1.0F, length}, matrix_level, BLACK, BLACK);
                }
                else
                {
                    draw_quad({460, 511}, {0, 0}, {column_position, -(tile.position - level.position - 4.0F) - length},
                              {1.0F, length}, matrix_level, CLEAR_TILE, CLEAR_TILE);
                }*/
            }
            else if (type == TileInfo::Type::DOUBLE)
            {
                float time_right = std::chrono::duration<float>(tp_new - tile.time_clicked_right).count();
                if (tile.column == Column::DT_LEFT)
                {
                    draw_single_tile(0.0F, -(tile.position - level.position - 4.0F) - length, length,
                                     (unsigned(tile.cleared_column) & unsigned(Column::FAR_LEFT))
                                         ? std::optional<float>(time_left)
                                         : std::nullopt);
                    draw_single_tile(2.0F, -(tile.position - level.position - 4.0F) - length, length,
                                     (unsigned(tile.cleared_column) & unsigned(Column::CENTER_RIGHT))
                                         ? std::optional<float>(time_right)
                                         : std::nullopt);
                }
                else
                {
                    draw_single_tile(1.0F, -(tile.position - level.position - 4.0F) - length, length,
                                     (unsigned(tile.cleared_column) & unsigned(Column::CENTER_LEFT))
                                         ? std::optional<float>(time_left)
                                         : std::nullopt);
                    draw_single_tile(3.0F, -(tile.position - level.position - 4.0F) - length, length,
                                     (unsigned(tile.cleared_column) & unsigned(Column::FAR_RIGHT))
                                         ? std::optional<float>(time_right)
                                         : std::nullopt);
                }
            }
        }
        // draw game over column
        if (level.game_over_column && level.game_over_column != Column::NONE)
        {
            Tile* tile = level.get_active_tile();
            float duration_count = std::chrono::duration<float>(tp_new - level.time_game_over).count();
            float alpha = glm::sqrt(glm::abs(glm::sin(duration_count * 3.1415F * 2.0F)));
            {
                float column_position = column_to_position(*level.game_over_column);
                float length = float(level.song_info.get_tiles()[tile->id].get_unit_length()) /
                               float(level.song_info.get_length_units_per_single_tile());
                draw_quad({460, 511}, {0, 0}, {column_position, -(tile->position - level.position - 4.0F) - length},
                          {1.0F, length}, matrix_level, glm::vec4{1.0, 0.047, 0.047, alpha},
                          glm::vec4{1.0, 0.047, 0.047, alpha});
            }
        }

        // 276, 532, 32, 32
        /*draw_text_debug({32, 32}, "Score: " + std::to_string(level.score), matrix_test, 2.0F,
                  glm::vec4{1.0, 0.047, 0.047, 1.0});
        draw_text_debug({32, 32 + 64}, "  TPS: " + std::to_string(level.tempo), matrix_test, 2.0F,
                  glm::vec4{1.0, 0.047, 0.047, 1.0});
        draw_text_debug({32, 32}, "FPS: " + std::to_string(glm::floor(1.0F / delta_time)), matrix_test, 2.0F,
                  glm::vec4{1.0, 0.047, 0.047, 1.0});*/
        /*draw_text_ascii(font, {32,64}, "Score: " + std::to_string(level.score), matrix_test, 0.001F, glm::vec4(0.0F,
        0.0F, 0.0F, 1.0F), 0.9); draw_text_ascii(font, {32,64}, "Score: " + std::to_string(level.score), matrix_test,
        0.001F, glm::vec4(1.0F, 0.1F, 0.1F, 1.0F)); draw_text_ascii(font, {32,64+64}, "TPS: " +
        std::to_string(level.tempo), matrix_test, 0.001F, glm::vec4(0.0F, 0.0F, 0.0F, 1.0F), 0.9); draw_text_ascii(font,
        {32,64+64}, "TPS: " + std::to_string(level.tempo), matrix_test, 0.001F, glm::vec4(1.0F, 0.1F, 0.1F, 1.0F));*/
            std::string string_score{std::to_string(level.score_display)};
            std::string string_tps{std::to_string(level.tempo)};
            if (string_tps.size() > 5)
            {
                string_tps = string_tps.substr(0, 5);
            }
            //float string_song_name_length = get_length_text_ascii(font, level.song_name, 0.0000015F);
            //float string_song_composer_length = get_length_text_ascii(font, level.song_composer, 0.000001F);
            float progress_idle = std::chrono::duration<float>(tp_new - level.time_song_started).count() * 5.0F;
            bool really_idle = (level.idle && level.revives_used == 0);
            if (really_idle || progress_idle < 1.0F)
            {
                float powed = glm::pow(progress_idle, 2.0F);
                std::string best_string = "Best: " + std::to_string(level.best_score);
                float string_best_score_length = get_length_text_ascii(font, best_string, 0.000001F);
                draw_quad({460, 511}, {0, 0}, {0.0F, really_idle ? 3.0F : 3.0F + progress_idle * 4.0F},
                              {4.0F, 1.0F}, matrix_level);
                draw_text_ascii(font, {0.125F * 0.5F, 0.875F / aspect_ratio + 0.05F}, level.song_composer, matrix_aspect,
                            0.000001F, glm::vec4(0.4F, 0.4F, 0.4F, 1.0F), glm::vec4(0.4F, 0.4F, 0.4F, 1.0F),
                            really_idle ? 0.5F : 0.5F - powed * 0.5F);
                draw_text_ascii(font, {1.0F - 0.5F * 0.125F - string_best_score_length, 0.875F / aspect_ratio + 0.05F}, best_string, matrix_aspect,
                            0.000001F, glm::vec4(1.0F, 0.1F, 0.1F, 1.0F), glm::vec4(1.0F, 0.2F, 0.2F, 1.0F),
                            really_idle ? 0.5F : 0.5F - powed * 0.5F);
                draw_text_ascii(font, {0.125F * 0.5F, 0.875F / aspect_ratio - 0.01F}, level.song_name, matrix_aspect,
                            0.0000015F, glm::vec4(0.00F, 0.0F, 0.0F, 1.0F), glm::vec4(0.0F, 0.0F, 0.0F, 1.0F),
                            really_idle ? 0.5F : 0.5F - powed * 0.5F);
                float offset = glm::sin(total_time * 3.1415F) * 0.05F;
                draw_quad({870, 128}, {128, 128}, {0.25F + offset - (really_idle ? 0.0F : powed), 0.5F - 0.25F * aspect_ratio},
                              {0.5F, 0.5F * aspect_ratio}, matrix_level, glm::vec4(1.0F, 0.1F, 0.1F, 0.5F), glm::vec4(1.0F, 0.1F, 0.1F, 0.8F));
                draw_quad({870, 0}, {128, 128}, {3.25F - offset + (really_idle ? 0.0F : powed), 0.5F - 0.25F * aspect_ratio},
                              {0.5F, 0.5F * aspect_ratio}, matrix_level, glm::vec4(1.0F, 0.1F, 0.1, 0.5F), glm::vec4(1.0F, 0.1F, 0.1F, 0.8F));
            }
            if (really_idle)
            {
                string_score = string_tps;
                string_tps = "Starting TPS";
            }
                float additional_score_size =
                    (0.05F - glm::min(std::chrono::duration<float>(tp_new - level.time_last_score_update).count(), 0.05F)) *
                        0.00001F +
                    0.000003F; 
                float string_score_length = get_length_text_ascii(font, string_score, additional_score_size);
                float string_tps_length = get_length_text_ascii(font, string_tps, 0.0000015F);
                bool draw_score = !level.idle || level.revives_used > 0;
                draw_text_ascii(font, {0.5F - string_score_length * 0.5F, 0.125 / aspect_ratio}, string_score, matrix_aspect,
                                additional_score_size, glm::vec4(0.01F, 0.0F, 0.0F, 1.0F), glm::vec4(0.0F, 0.0F, 0.0F, 1.0F),
                                0.9);
                draw_text_ascii(font, {0.5F - string_tps_length * 0.5F, 0.125 / aspect_ratio + 0.09}, string_tps, matrix_aspect,
                                0.0000015F, glm::vec4(0.01F, 0.0F, 0.0F, 1.0F), glm::vec4(0.0F, 0.0F, 0.0F, 1.0F), 0.9);
                glm::vec4 color = level.speed_modifier == 0 ? glm::vec4(1.0F, 0.1F, 0.1F, 1.0F) : glm::vec4(0.9F, 0.9F, 0.1F, 1.0F);
                draw_text_ascii(font, {0.5F - string_score_length * 0.5F, 0.125 / aspect_ratio}, string_score, matrix_aspect,
                                additional_score_size, color, color);
                draw_text_ascii(font, {0.5 - string_tps_length * 0.5F, 0.125 / aspect_ratio + 0.09}, string_tps, matrix_aspect,
                                0.0000015F, glm::vec4(1.0F, 0.9F, 0.9F, 1.0F), glm::vec4(1.0F, 1.0F, 1.0F, 1.0F));
        /*if (level.idle)
        {draw_text_debug({32, 32 + 128+64}, "Reviv: " + std::to_string(3 - level.revives_used), matrix_test, 2.0F,
                  glm::vec4{1.0, 0.047, 0.047, 1.0});}*/
        // draw_text_debug({32, 32+128}, "Tiles: " + std::to_string(level.tiles.size()), matrix_test, 2.0F);

        glClear(GL_COLOR_BUFFER_BIT /*| GL_DEPTH_BUFFER_BIT*/);

        // draw background
        glBindVertexArray(vao_background);
        glUseProgram(program_background);
        glUniform1f(uniform_location_background_time, total_time);
        static float background_id = 0.0F;
        if (background_id != level.background_id)
        {
            background_id += delta_time * glm::sign(level.background_id - background_id);
            background_id = glm::min(background_id, level.background_id);
            glUniform1f(uniform_location_background_id, background_id);
        }
        glDisable(GL_BLEND);
        glDrawElements(GL_TRIANGLES, buffer_ebo_background.size(), GL_UNSIGNED_BYTE, 0);

        // draw basic
        glBindVertexArray(vao_basic);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_basic);
        glUseProgram(program_basic);
        glBufferSubData(GL_ARRAY_BUFFER, 0, buffer_vbo_basic.size() * sizeof(GLfloat), buffer_vbo_basic.data());
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, buffer_ebo_basic.size() * sizeof(GLushort),
                        buffer_ebo_basic.data());
        glEnable(GL_BLEND);
        glDrawElements(GL_TRIANGLES, buffer_ebo_basic.size(), GL_UNSIGNED_SHORT, 0);

        // draw helper
        Tile* active_tile = level.get_active_tile();
        if (!level.game_over_column && active_tile)
        {
            const float tile_length = float(level.song_info.get_tiles()[active_tile->id].get_unit_length()) /
                                      float(level.song_info.get_length_units_per_single_tile());
            bool is_double_tile = level.song_info.get_tiles()[active_tile->id].get_type() == TileInfo::Type::DOUBLE;
            float column_position = is_double_tile ? (active_tile->column == Column::DT_LEFT ? 0.0 : 1.0)
                                                   : column_to_position(active_tile->column);
            glm::vec4 position_a =
                matrix_level *
                glm::vec4(column_position, -active_tile->position - tile_length + level.position + 4.0F, 0.0, 1.0);
            glm::vec4 position_c = matrix_level * glm::vec4(column_position + 1.0,
                                                            -active_tile->position + level.position + 4.0F, 0.0, 1.0);
            buffer_vbo_helper = {
                position_a.x,        position_a.y, 0.0, 0.0, position_c.x,        position_a.y, 1.0, 0.0,
                position_c.x,        position_c.y, 1.0, 1.0, position_a.x,        position_c.y, 0.0, 1.0,
                position_a.x + 1.0F, position_a.y, 0.0, 0.0, position_c.x + 1.0F, position_a.y, 1.0, 0.0,
                position_c.x + 1.0F, position_c.y, 1.0, 1.0, position_a.x + 1.0F, position_c.y, 0.0, 1.0,
            };
            glBindVertexArray(vao_helper);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_helper);
            glUseProgram(program_helper);
            static float last_tile_length = 1.0F;
            float alpha = std::min(
                glm::pow(std::chrono::duration<float>(tp_new - level.tile_full_time_clicked).count() * level.tempo,
                         2.0F),
                1.0F);
            static float last_alpha = 0.0F;
            if (alpha != last_alpha)
            {
                last_alpha = alpha;
                glUniform1f(uniform_location_helper_alpha, alpha);
            }
            if (tile_length != last_tile_length)
            {
                last_tile_length = tile_length;
                glUniform1f(uniform_location_helper_tile_length, tile_length);
            }
            glUniform1f(uniform_location_helper_time, total_time);
            glBufferSubData(GL_ARRAY_BUFFER, 0, buffer_vbo_helper.size() * sizeof(GLfloat), buffer_vbo_helper.data());
            glDrawElements(
                GL_TRIANGLES, (is_double_tile && active_tile->cleared_column == Column::NONE) ? 12 : 6,
                GL_UNSIGNED_BYTE,
                reinterpret_cast<void*>((is_double_tile && (unsigned(active_tile->cleared_column) & 0b1100)) ? 6 : 0));
        }
        // draw SDF
        // glDisable(GL_BLEND);
        glBindVertexArray(vao_sdf);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_sdf);
        glUseProgram(program_sdf);
        glBufferSubData(GL_ARRAY_BUFFER, 0, buffer_vbo_sdf.size() * sizeof(GLfloat), buffer_vbo_sdf.data());
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, buffer_ebo_sdf.size() * sizeof(GLushort), buffer_ebo_sdf.data());
        glDrawElements(GL_TRIANGLES, buffer_ebo_sdf.size(), GL_UNSIGNED_SHORT, 0);

        SDL_GL_SwapWindow(window);
    }
}