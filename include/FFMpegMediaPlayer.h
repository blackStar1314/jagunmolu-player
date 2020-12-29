#pragma once
#include "FFMpegMedia.h"
#include "FFMpegDecoder.h"
#include "IAudioOutput.h"
#include "IVideoOutput.h"
#include "FFMpegFilterGraph.h"
#include <thread>
#include "concurrent_queue.h"
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include "SubtitleManager.h"

namespace jp {
    enum class MediaResult { RESULT_SUCCESS, RESULT_ERROR };
    struct MediaError {
        std::string error{};
    };
    class FFMpegMediaPlayer {
    public:
        FFMpegMediaPlayer() {}
        MediaResult set_media(FFMpegMedia_Ptr media);
        void set_audio_output(AudioOutput_Ptr audio_output) { this->audio_output = audio_output; }
        void set_video_output(VideoOutput_Ptr video_output) { this->video_output = video_output; }
        MediaResult play();
        
        /// The temp_pause is so that buffering can resume when enough data is available
        MediaResult pause(bool temp_pause = false);
        MediaResult stop();
        bool is_playing() { return playing || requested_play; }
        bool seek_to(uint64_t position_millis);
        void release();
        
        void disable_subtitle() { if (video_output) video_output->disable_subtitle(); }
        void enable_subtitle() { if (video_output) video_output->enable_subtitle(); }
        
        bool is_subtitle_enabled() { if (video_output) return video_output->is_subtitle_enabled(); else return false; }
        
        /// Whether this media has been released
        bool is_released() const { return released; }
        
        /// Sets the volume. The current volume is always 1.0 when starting
        void set_volume(double volume) {
            auto filters = filter_graph->get_filters();
            std::for_each(filters.begin(), filters.end(), [&](FFMpegFilter_Ptr filter) {
                if (filter->get_name() == "volume") {
                    filter->set_property("volume", std::to_string(volume));
                    filter_graph->send_command(filter, std::to_string(volume));
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
        
        uint64_t get_last_audio_pts() { return last_audio_pts; }
        uint64_t get_last_video_pts() { return last_video_pts; }
        
        FFMpegMedia_Ptr get_current_media() { return current_media; }
        
        void set_last_audio_pts(uint64_t pts) {
            last_audio_pts = pts;
        }
        
        void set_last_video_pts(uint64_t pts) {
            last_video_pts = pts;
            current_position = pts * current_media->get_demuxer()->get_video_stream()->get_time_base() * 1000;
        }
        
        void start_demuxer_thread();
        
        /// Sets the error message
        void set_error(std::string err_str) {
            error.error = err_str;
        }
        
        /// Returns the current error message
        const MediaError& get_error() {
            return error;
        }
        
        void set_audio_enabled(bool enabled) {
            if (audio_enabled == enabled) return;
            audio_enabled = enabled;
            if (!enabled) {
                if (audio_output) {
                    audio_output->stop();
                }
            } else {
                if (audio_output) {
                    if (playing) {
                        audio_output->play();
                    }
                }
            }
        }
        
        bool is_audio_enabled() { return audio_enabled; }
        bool is_video_enabled() { return video_enabled; }
        
        void set_video_enabled(bool enabled) {
            if (video_enabled == enabled) return;
            video_enabled = enabled;
            if (!enabled) {
                if (video_output) {
                    video_output->stop();
                }
            } else {
                if (video_output) {
                    if (playing) {
                        video_output->play();
                    }
                }
            }
        }
        
        /**
         * @brief Set the subtitle delay in milliseconds
         * A positive value slows down the subtitle
         * A negative value speeds up the subtitle
         * @param delay the delay in milliseconds
         */
        void set_subtitle_delay(int64_t delay) {
            if (video_output) video_output->set_subtitle_delay(delay);
        }
        
        /**
         * @brief Get the subtitle delay
         * 
         * @return subtitle delay if it there is a subtitle, else returns 0
         */
        int64_t get_subtitle_delay() { if (video_output) return video_output->get_subtitle_delay(); else return 0; }
        
        /**
         * @brief Add a subtitle to the media file. This parses and adds the subtitle to the currently playing media.
         * 
         * @param path - The path to the subtitle to add
         * @return true if the subtitle was successfully added and false otherwise
         */
        bool add_subtitle(std::string path);
        
        /**
         * @brief Called by the video and audio outputs attached to this media player to signify that the buffering state has changed
         * 
         */
        void buffering_changed();
        
        /**
         * @brief Returns the next decoded audio frame
         */
        FFMpegFrame_Ptr get_next_audio_frame();

        /**
         * @brief Get the next video frame
         */
        FFMpegFrame_Ptr get_next_video_frame();

        /**
         * @brief Destroy the FFMpegMediaPlayer object (Am I really supposed to document a destructor?)
         */
        ~FFMpegMediaPlayer() { release(); }
    private:
        /**
         * @brief are we currently playing media?
         */
        bool playing{false};
        /**
         * @brief Used internally for buffering
         */
        bool requested_play{false};
        /**
         * @brief Represents the audio output
         */
        AudioOutput_Ptr audio_output;
        /**
         * @brief Represents the video output
         */
        VideoOutput_Ptr video_output;

        /**
         * @brief Demuxer thread
         */
        std::thread demuxer_thread;
        /**
         * @brief Audio frame queue (contains uncompressed audio frames)
         */
        moodycamel::ConcurrentQueue<FFMpegFrame_Ptr> audio_frame_queue;
        
        /**
         * @brief Audio packet queue (contains compressed audio frames)
         */
        moodycamel::ConcurrentQueue<FFMpegPacket_Ptr> audio_packet_queue{300};
        
        /**
         * @brief Video packet queue (contains compressed video frames)
         */
        moodycamel::ConcurrentQueue<FFMpegPacket_Ptr> video_packet_queue{200};
        
        /**
         * @brief Current media played by this media player
         */
        FFMpegMedia_Ptr current_media{nullptr};

        /**
         * @brief The playback position of the currently playing media
         */
        uint64_t current_position{0};

        /**
         * @brief The audio filter graph
         */
        FFMpegFilterGraph_Ptr filter_graph{nullptr};

        /**
         * @brief The video filter graph
         */
        FFMpegFilterGraph_Ptr video_filter_graph{nullptr};
        
        /**
         * @brief An error message handle
         */
        MediaError error;
        
        /**
         * @brief Has this media player instance been released?
         */
        std::atomic_bool released{false};

        /**
         * @brief Do we need to flush the buffered data?
         */
        std::atomic_bool demuxer_clear{false};

        /**
         * @brief Are we buffering?
         */
        std::atomic_bool buffering{false};
        
        /**
         * @brief For tracking the last audio time played (used in A/V Sync)
         */
        std::atomic<uint64_t> last_audio_pts{0};
        /**
         * @brief Same as audio above
         */
        std::atomic<uint64_t> last_video_pts{0};
        
        /**
         * @brief The audio decoder instance
         */
        FFMpegDecoder_Ptr audio_decoder;
        /**
         * @brief The video decoder
         */
        FFMpegDecoder_Ptr video_decoder;
        
        /**
         * @brief Whether audio is enabled
         */
        std::atomic_bool audio_enabled{false};

        /**
         * @brief Whether video is enabled
         */
        std::atomic_bool video_enabled{false};
        
        /**
         * @brief For some basic demuxer synchronizations
         */
        std::mutex demuxer_wake_mutex{};
        std::condition_variable demuxer_wake_condition{};
        
        /**
         * @brief Responsible for parsing and managing subtitles
         */
        std::shared_ptr<SubtitleManager> subtitle_manager{new SubtitleManager};
    };
    
    using FFMpegMediaPlayer_Ptr = std::shared_ptr<FFMpegMediaPlayer>;
}
