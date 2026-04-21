#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <vector>
#include <codecvt>     //for std::codecvt_utf8
#include <locale>      //for std::wstring_convert

inline std::string get_res_path(const char* t_file)
{
#ifdef __ANDROID__
    return t_file;
#else
    return std::string{"res/"} + t_file;
#endif
}

inline std::u32string utf8_to_utf32(const std::string& t_string)
{
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    return converter.from_bytes(t_string.data());
}

inline std::string get_pref_path(const char* t_file)
{
    static std::string pref_path{[](){
        char* pref_path_cstr = SDL_GetPrefPath("volian", "elitistdune");
        std::string pref_path_str = pref_path_cstr;
        SDL_free(pref_path_cstr);
        return pref_path_str;
    }()};
    return pref_path + t_file;
}

template <typename T = unsigned char> inline std::vector<T> file_to_buffer(const char* t_file)
{
    std::vector<T> buffer;
    SDL_IOStream* file{SDL_IOFromFile(get_res_path(t_file).data(), "rb")};
    if (!file)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Error", "Couldn't open resource file", nullptr);
        std::abort();
    }
    std::size_t written_bytes{0};
    const auto size{SDL_GetIOSize(file)};
    if (size < 0)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Error", "Couldn't get resource file's size", nullptr);
        std::abort();
    }
    buffer.resize(size);
    while (written_bytes < size)
    {
        const auto read_bytes = SDL_ReadIO(file, buffer.data() + written_bytes, size - written_bytes);
        if (read_bytes == 0)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Error", "Couldn't read resource file", nullptr);
            std::abort();
        }
        written_bytes += read_bytes;
    }
    SDL_CloseIO(file);
    return buffer;
}