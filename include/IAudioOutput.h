#pragma once
#include <memory>
#include <string>
#include <atomic>

namespace jp {

	class FFMpegMediaPlayer;

	class IAudioOutput {
	public:
		IAudioOutput(std::shared_ptr<FFMpegMediaPlayer> media_player) : media_player(media_player) {}

		/// Initialize the audio output
		virtual bool initialize() = 0;
		virtual bool play() = 0;
		virtual bool pause() = 0;
		virtual bool stop() = 0;
		virtual void release() = 0;
		std::string get_error() { return error; }
        virtual void reset() = 0;
        bool is_buffering() { return buffering; }
		
	protected:
		std::shared_ptr<FFMpegMediaPlayer> media_player;
		std::string error;
		std::atomic_bool is_playing{false};
        std::atomic_bool buffering{false};
	};

	using AudioOutput_Ptr = std::shared_ptr<IAudioOutput>;

}
