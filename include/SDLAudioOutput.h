#pragma once
#include "IAudioOutput.h"
#include "FFMpegMediaPlayer.h"
#include <SDL2/SDL.h>

#include <atomic>
extern "C" {
#include <libavutil/audio_fifo.h>
}

namespace jp {
	class SDLAudioOutput : public IAudioOutput {
	public:
		SDLAudioOutput(FFMpegMediaPlayer_Ptr media_player) : IAudioOutput::IAudioOutput(media_player) {}
        
        ~SDLAudioOutput() { av_audio_fifo_free(fifo); }

		bool initialize() override;

		bool play() override;
		
		bool pause() override;
		
		bool stop() override;
		
		void release() override;

		static void audio_callback(void* opaque, uint8_t* buffer, int len);

		~SDLAudioOutput() { release(); }
        
        uint64_t get_num_written_samples() { return total_samples_written; }
        
        void reset() {
            if (fifo) {
                av_audio_fifo_reset(fifo);
                total_samples_written = 0;
            }
        }
        
        /// Get how many seconds has played in this media
        int64_t get_playback_duration() {
            // Sample rate and number of samples already written
            int sample_rate = spec.freq;
            // 1 second = {sample_rate} samples
            // x seconds = {total_samples_written} samples
            // {total_samples_written} = {sample_rate} * x
            // x = {total_samples_written} / {sample_rate}
            return total_samples_written / sample_rate;
        }

	private:
		SDL_AudioSpec spec{};

		/// This stores the last pts from the frames we received
        uint64_t last_pts{0};
        uint64_t total_samples_written{0};

		std::atomic<bool> running;
        AVAudioFifo* fifo{nullptr};
	};
}
