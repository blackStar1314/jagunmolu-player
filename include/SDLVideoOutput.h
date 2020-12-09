#ifndef SDLVIDEOOUTPUT_H
#define SDLVIDEOOUTPUT_H
#include "IVideoOutput.h"
#include "concurrent_queue.h"
#include "FFMpegFrame.h"
#include <thread>
#include <condition_variable>

extern "C" {
#include <libswscale/swscale.h>
}

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

namespace jp {
class SDLVideoOutput : public IVideoOutput {
public:
    SDLVideoOutput(std::shared_ptr<FFMpegMediaPlayer> player);
    bool initialize();
    bool play();
    bool pause();
    bool stop();
    void release();
    
    /// Once you call this function, you have to call initialize again to use this class.
    void reset();
    void clear_buffer() override;
    
private:
    SDL_Window* window{nullptr};
    SDL_Renderer* renderer{nullptr};
    SDL_Texture* texture{nullptr};
    std::atomic_bool initialized{false};
    std::atomic_bool playing{false};
    std::atomic_bool stop_thread{true};
    std::mutex player_mutex{};
    std::condition_variable player_condition{};
    SwsContext* context;
    
    /// This function does the actual playback
    void playback_func();
    
    /// This function decodes and buffers frames
    void buffer_data();
    std::mutex frame_queue_mutex{};
    std::condition_variable frame_queue_condition{};
    moodycamel::ConcurrentQueue<FFMpegFrame_Ptr> video_frame_queue{};
    // We sync to audio by default
    bool sync_to_audio{true};
    
    std::vector<std::string> last_subs{};
    SDL_Texture* sub_texture;
    
    TTF_Font* font;
};
}

#endif // SDLVIDEOOUTPUT_H
