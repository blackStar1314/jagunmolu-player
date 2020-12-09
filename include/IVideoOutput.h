#ifndef IVIDEOOUTPUT_H
#define IVIDEOOUTPUT_H

#include <iostream>
#include <memory>
#include <atomic>
#include "SubtitleManager.h"

namespace jp {

class FFMpegMediaPlayer;

class IVideoOutput {
public:
	IVideoOutput(std::shared_ptr<FFMpegMediaPlayer> player) : player(player) {}
	
	virtual bool initialize() = 0;
	virtual bool play() = 0;
	virtual bool pause() = 0;
	virtual bool stop() = 0;
	virtual void release() = 0;
	std::string get_error() { return error; }
    virtual void clear_buffer() = 0;
	virtual void reset() = 0;
    bool is_buffering() { return buffering; }
    
    void set_subtitle_manager(SubtitleManager* manager) {
        this->subtitle_manager = manager;
    }
    
    void enable_subtitle() { subtitle_enabled = true; }
    void disable_subtitle() { subtitle_enabled = false; }
    bool is_subtitle_enabled() { return subtitle_enabled; }
    int64_t get_subtitle_delay() { return subtitle_delay; }
    void set_subtitle_delay(int64_t delay) { subtitle_delay = delay; }
	
protected:
	std::string error;
	std::shared_ptr<FFMpegMediaPlayer> player;
    std::atomic_bool buffering{false};
    SubtitleManager* subtitle_manager;
    std::atomic_bool subtitle_enabled{true};
    int64_t subtitle_delay{0};
};

using VideoOutput_Ptr = std::shared_ptr<IVideoOutput>;

}

#endif // IVIDEOOUTPUT_H
