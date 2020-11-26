#pragma once
#include <memory>
#include <string>

namespace jp {

	class FFMpegMediaPlayer;

	class IAudioOutput {
	public:
		IAudioOutput(FFMpegMediaPlayer* media_player) : media_player(media_player) {}

		/// Initialize the audio output
		virtual bool initialize() = 0;
		virtual bool play() = 0;
		virtual bool pause() = 0;
		virtual bool stop() = 0;
		virtual void release() = 0;
		std::string get_error() { return error; }
        virtual void reset() = 0;
		
	protected:
		FFMpegMediaPlayer* media_player;
		std::string error;
		bool is_playing{false};
	};

	using AudioOutput_Ptr = IAudioOutput*;

}
