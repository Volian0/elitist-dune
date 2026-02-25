#pragma once

#include "SongInfo.hpp"
#include "Utility.hpp"

#include <SDL3_mixer/SDL_mixer.h>
#include <tsf/tsf.h>

#include <chrono>
#include <deque>
#include <iostream>
#include <mutex>

struct QueuedNoteEvent
{
    std::chrono::steady_clock::time_point time;
    NoteInfo note_info;
};

struct Soundfont
{
    Soundfont(const char* t_file, unsigned t_sample_rate)
    {
        const auto buffer = file_to_buffer(t_file);
        std::scoped_lock lock{mutex};
        handle = tsf_load_memory(buffer.data(), buffer.size());
        if (!handle)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Error", "Failed to open soundfont", nullptr);
            std::abort();
        }
        tsf_set_output(handle, TSF_STEREO_INTERLEAVED, t_sample_rate);
        tsf_set_volume(handle, 0.5F);
    }

    ~Soundfont()
    {
        std::scoped_lock lock{mutex};
        tsf_close(handle);
    }

    void note_on(unsigned char t_key, unsigned char t_velocity)
    {
        tsf_note_on(handle, 0, t_key, float(t_velocity) / 127.0F);
    }

    void note_off(unsigned char t_key)
    {
        tsf_note_off(handle, 0, t_key);
    }

    void all_notes_off()
    {
        tsf_note_off_all(handle);
    }

    void reset()
    {
        std::scoped_lock lock{mutex};
        tsf_reset(handle);
    }

    void play_queued_note_events(const std::chrono::steady_clock::time_point& t_time)
    {
        while (!queued_note_events.empty())
        {
            const auto& queued_note_event = queued_note_events[0];
            if (t_time < queued_note_event.time)
            {
                return;
            }
            if (queued_note_event.note_info.get_type() == NoteInfo::Type::ON)
            {
                note_on(queued_note_event.note_info.get_key(), queued_note_event.note_info.get_velocity());
            }
            else if (queued_note_event.note_info.get_type() == NoteInfo::Type::OFF)
            {
                note_off(queued_note_event.note_info.get_key());
            }
            else
            {
                all_notes_off();
            }
            queued_note_events.pop_front();
        }
    }

    void render(float* t_buffer, unsigned t_samples)
    {
        tsf_render_float(handle, t_buffer, t_samples, 1);
    }

    Soundfont(const Soundfont&) = delete;
    Soundfont& operator=(const Soundfont&) = delete;
    Soundfont(Soundfont&&) = delete;
    Soundfont& operator=(Soundfont&&) = delete;

    tsf* handle;
    mutable std::mutex mutex;
    std::deque<QueuedNoteEvent> queued_note_events;
};

inline void soundfont_callback(void* t_soundfont, MIX_Mixer* t_mixer, const SDL_AudioSpec* t_spec, float* t_pcm,
                               int t_samples)
{
    Soundfont& soundfont = *static_cast<Soundfont*>(t_soundfont);
    std::scoped_lock lock{soundfont.mutex};
    soundfont.play_queued_note_events(std::chrono::steady_clock::now());
    soundfont.render(t_pcm, t_samples / 2);
}