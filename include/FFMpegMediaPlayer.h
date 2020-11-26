#pragma once
#include "FFMpegMedia.h"
#include "FFMpegDecoder.h"
#include "IAudioOutput.h"
#include "FFMpegFilterGraph.h"
#include <thread>
#include "concurrent_queue.h"
//#include "readerwriterqueue.h"
#include <algorithm>

namespace jp {
    enum class MediaResult { RESULT_SUCCESS, RESULT_ERROR };
    struct MediaError {
        std::string error{};
    };
    class FFMpegMediaPlayer {
    public:
        FFMpegMediaPlayer() : audio_frame_queue(500), video_frame_queue(500), audio_packet_queue(500), video_packet_queue(500) {}
        MediaResult set_media(FFMpegMedia_Ptr media);
        void set_audio_output(AudioOutput_Ptr audio_output) { this->audio_output = audio_output; }
        MediaResult play();
        MediaResult pause();
        MediaResult stop();
        bool is_playing() { return playing; }
        bool seek_to(uint64_t position_millis);
        void release();
        /// Whether this media has been released
        bool is_released() const { return released; }
        
        /// Sets the volume. The current volume is always 1.0 when starting
        void set_volume(double volume) {
            auto filters = filter_graph->get_filters();
            std::for_each(filters.begin(), filters.end(), [&](FFMpegFilter_Ptr filter) {
                if (filter->get_name() == "volume") {
                    filter->set_property("volume", std::to_string(volume));
                    filter_graph->send_command(filter, "0.2");
                }
            });
        }
        
        bool has_media() { return current_media != nullptr; }
        
        /// Returns the current playback position in milliseconds
        uint64_t get_position() const { return current_position; }
        
        uint64_t get_duration() const {
            if (!current_media) return 0;
            return current_media->get_duration();
        }
        
        uint64_t get_sample_rate() { return current_media->get_sample_rate(); }
        uint64_t get_channels() { return current_media->get_channels(); }
        uint64_t get_channel_layout() { return current_media->get_channel_layout(); }
        
        void start_demuxer_thread();
        void start_audio_thread();
        void start_video_thread();
        
        /// Sets the error message
        void set_error(std::string err_str) {
            error.error = err_str;
        }
        
        /// Returns the current error message
        const MediaError& get_error() {
            return error;
        }
        
        void audio_output_finished();
        
        FFMpegFrame_Ptr get_next_audio_frame();
        ~FFMpegMediaPlayer() { release(); }
    private:
        bool playing{false};
        AudioOutput_Ptr audio_output;
        std::thread demuxer_thread;
        std::thread audio_decoder_thread;
        std::thread video_decoder_thread;
        moodycamel::ConcurrentQueue<FFMpegFrame_Ptr> audio_frame_queue;
        moodycamel::ConcurrentQueue<FFMpegFrame_Ptr> video_frame_queue;
        moodycamel::ConcurrentQueue<FFMpegPacket_Ptr> audio_packet_queue;
        moodycamel::ConcurrentQueue<FFMpegPacket_Ptr> video_packet_queue;
        
        /// The current media being played by this media player
        FFMpegMedia_Ptr current_media{nullptr};
        uint64_t current_position{0};
        FFMpegFilterGraph_Ptr filter_graph;
        
        /// An error handle
        MediaError error;
        
        /// Whether this media player has been released and thus cannot be used anymore
        std::atomic_bool released{false};
        std::atomic_bool demuxer_clear{false};
        std::atomic_bool audio_decoder_clear{false};
        std::atomic_bool video_decoder_clear{false};
        
        FFMpegDecoder_Ptr audio_decoder;
        FFMpegDecoder_Ptr video_decoder;
        FFMpegDecoder_Ptr subtitle_decoder;
        std::atomic_bool ready{false};
    };
    
    using FFMpegMediaPlayer_Ptr = FFMpegMediaPlayer*;
}
