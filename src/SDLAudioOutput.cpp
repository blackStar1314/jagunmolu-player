#include "SDLAudioOutput.h"

namespace jp {
	bool SDLAudioOutput::initialize() {

		if (SDL_Init(SDL_INIT_AUDIO) < 0) {
			error = "Unable to initialize the SDL2 audio subsystem";
			return false;
		}

		spec.freq = media_player->get_sample_rate();
		spec.callback = SDLAudioOutput::audio_callback;
		spec.userdata = this;
		spec.channels = media_player->get_channels();
		spec.format = AUDIO_S16;
		spec.silence = 0;
		
		SDL_AudioSpec gotten;

		if (SDL_OpenAudio(&spec, &gotten) < 0) {
			error = "Unable to open audio output!";
			return false;
		}
        
        fifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_S16, gotten.channels, 1);
        if (!fifo) return false;
        
        buffering = false;

		return true;
	}

	bool SDLAudioOutput::play() {
		if (is_playing) return false;
		is_playing = true;
		SDL_PauseAudio(0);
		return true;
	}

	bool SDLAudioOutput::pause() {
		if (!is_playing) return false;
		is_playing = false;
		SDL_PauseAudio(1);
		return true;
	}

	bool SDLAudioOutput::stop() {
		SDL_PauseAudio(1);
		// Clear the internal queue
		return true;
	}

	void SDLAudioOutput::release() {
		SDL_CloseAudio();
        SDL_Quit();
	}

	void SDLAudioOutput::audio_callback(void* opaque, uint8_t* buffer, int len) {
        SDL_memset(buffer, 0, len);
		auto* output = reinterpret_cast<SDLAudioOutput*>(opaque);
        
        int samples = len / output->spec.channels / output->spec.channels;
        
        while (av_audio_fifo_size(output->fifo) < samples) {
            auto frame = output->media_player->get_next_audio_frame();
            if (!frame) {
                // We don't have enough, bail :)
                output->buffering = false;
                output->media_player->buffering_changed();
                break;
            }
            
            if (output->buffering) {
                output->buffering = false;
                output->media_player->buffering_changed();
            }
            
            av_audio_fifo_write(output->fifo, (void**)frame->get_data(), frame->get_number_of_samples());
            
			// Set the last audio presentation time in milliseconds
            output->media_player->set_last_audio_pts(frame->get_presentation_timestamp() * (output->media_player->get_current_media()->get_demuxer()->get_audio_stream()->get_time_base() * 1000.0));
        }
        
        int read = av_audio_fifo_read(output->fifo, (void**)&buffer, samples);
        output->total_samples_written += read;
	}
}
