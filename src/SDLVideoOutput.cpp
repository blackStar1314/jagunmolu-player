#include "SDLVideoOutput.h"
#include "FFMpegMediaPlayer.h"
#include "FFMpegMedia.h"

namespace jp {

SDLVideoOutput::SDLVideoOutput(std::shared_ptr<FFMpegMediaPlayer> player) : IVideoOutput(player), video_frame_queue(100) {}

bool SDLVideoOutput::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        error = "Unable to initialize SDL2";
        return false;
    }
    
    if (TTF_Init() < 0) {
        error = "Unable to initialize TTF Rendering engine";
        return false;
    }
    
    int width = player->get_current_media()->get_width();
    int height = player->get_current_media()->get_height();
    
    window = SDL_CreateWindow(player->get_current_media()->get_io_context()->get_path().c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        error = "Unable to create window";
        return false;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        error = "Unable to create SDL2 renderer for video";
        fprintf(stderr, "Renderer couldn't be created!\n");
        return false;
    }
    
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texture) {
        fprintf(stderr, "Texture couldn't be created!\n");
        error = "Unable to create video texture";
    }
    
    context = sws_getContext(player->get_current_media()->get_width(), player->get_current_media()->get_height(), (AVPixelFormat)player->get_current_media()->get_pixel_format(), width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!context) {
        fprintf(stderr, "Couldn't initialize context!\n");
    }
    
    initialized = true;
    playing = false;
    stop_thread = false;
    buffering = true;
    
    std::thread buffer_thread = std::thread(&SDLVideoOutput::buffer_data, this);
    buffer_thread.detach();
    
    std::thread player_thread = std::thread(&SDLVideoOutput::playback_func, this);
    player_thread.detach();
    
    if (!player->is_audio_enabled()) {
        sync_to_audio = false; // We do not sync to audio
    }
    
    font = TTF_OpenFont("/home/smallwondertech/.local/share/fonts/FiraCode-Regular.ttf", 20);
    
    return true;
}

void SDLVideoOutput::playback_func() {
    SDL_Event event;
    while (!stop_thread) {
        // Just stay here and do nothing if we're not currently playing
        while (!playing) {
            std::unique_lock<std::mutex> lock(player_mutex);
            player_condition.wait(lock);
            fprintf(stderr, "Said to play\n");
        }
        
        FFMpegFrame_Ptr frame;
        while (!video_frame_queue.try_dequeue(frame)) {
            fprintf(stderr, "Couldn't dequeue video frame!\n");
            buffering = true;
            player->buffering_changed();
            
            while (buffering) {
                std::unique_lock<std::mutex> lock(player_mutex);
                frame_queue_condition.notify_all();
                player_condition.wait(lock);
            }
        }
        frame_queue_condition.notify_all();
        
        if (frame) {
            
            auto tb_v = player->get_current_media()->get_demuxer()->get_video_stream()->get_time_base();
            auto pts = frame->get_presentation_timestamp() * tb_v * 1000;
            
            if (!player->get_current_media()->get_demuxer()->get_video_stream()->is_attached_pic()) {
                if (sync_to_audio) {
                    auto tb_a = player->get_current_media()->get_demuxer()->get_audio_stream()->get_time_base();
                    auto last_audio_pts = player->get_last_audio_pts() * tb_a * 1000;
                    
                    // This is where the synchronization happens
                    auto diff = last_audio_pts - pts;
                    
                    // Drop this frame if we're behind by more than 30 milliseconds
                    if (diff > 30) {
                        fprintf(stderr, "Behind by %f ms\n", diff);
                        continue;
                    }
                    
                    // Is the video ahead? Wait for audio using the diff
                    if (diff < 0) {
                        // I don't know if this is good enough, 30 milliseconds
                       SDL_Delay(abs(diff));
                    }
                } else {
                    double delay = 1.0 / player->get_current_media()->get_demuxer()->get_video_stream()->get_frame_rate();
                    SDL_Delay(delay * 1000);
                }
                
                player->set_last_video_pts(frame->get_presentation_timestamp());
            }
            
            int pitch;
            uint8_t* pixels;
            
            SDL_LockTexture(texture, nullptr, (void**)&pixels, &pitch);
            
            uint8_t* data[4] = { pixels, nullptr, nullptr, nullptr };
            int linesize[4] = { pitch, 0, 0, 0 };
            sws_scale(context, frame->get_data(), frame->get_data_size(), 0, frame->get_height(), data, linesize);
            
            SDL_UnlockTexture(texture);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderClear(renderer);
            SDL_Rect rect { 0, 0, 0, 0 };
            SDL_GetWindowSize(window, &rect.w, &rect.h);
            SDL_RenderCopy(renderer, texture, nullptr, &rect);
            if (subtitle_enabled) {
                int window_w, window_h;
                SDL_GetWindowSize(window, &window_w, &window_h);
                auto subtitles = subtitle_manager->get_subtitle_entries_at(pts - subtitle_delay);
                std::for_each(subtitles.begin(), subtitles.end(), [&](std::string& s) {
                    bool proceed = true;
                    std::for_each(last_subs.begin(), last_subs.end(), [&](std::string& sub) {
                        if (sub == s) {
                            proceed = false;
                        }
                    });
                    if (proceed) {
                        last_subs.push_back(s);
                        SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(font, s.c_str(), { 255, 255, 255, 255 }, window_w - 20);
                        SDL_DestroyTexture(sub_texture);
                        sub_texture = SDL_CreateTextureFromSurface(renderer, surface);
                        SDL_FreeSurface(surface);
                    }
                    uint32_t format;
                    int access;
                    int w, h;
                    SDL_QueryTexture(sub_texture, &format, &access, &w, &h);
                    SDL_Rect dest = { window_w / 2 - w / 2, window_h - h * 2, w, h };
                    SDL_RenderCopy(renderer, sub_texture, nullptr, &dest);
                });
            }
            SDL_RenderPresent(renderer);
        }
        
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                // Stop playback, we're done here...
                player->stop();
                break;
            }
        }
    }
    
    fprintf(stderr, "Finished videoplayback_func\n");
}

void SDLVideoOutput::buffer_data() {
    while (!stop_thread) {
        FFMpegFrame_Ptr frame = player->get_next_video_frame();
        if (frame) {
            std::unique_lock<std::mutex> lock(frame_queue_mutex);
            while (!video_frame_queue.try_enqueue(frame)) {
                if (buffering) {
                    buffering = false;
                    player_condition.notify_all();
                    player->buffering_changed();
                }
                frame_queue_condition.wait(lock);
            }
        } else {
            player_condition.notify_all();
        }
    }
}

void SDLVideoOutput::clear_buffer() {
    FFMpegFrame_Ptr frame;
    std::unique_lock<std::mutex> lock(frame_queue_mutex);
    while (video_frame_queue.try_dequeue(frame)) {}
    frame_queue_condition.notify_all();
}

bool SDLVideoOutput::play() {
    if (!initialized && !initialize()) {
        return false;
    }
    playing = true;
    stop_thread = false;
    player_condition.notify_all();
    return true;
}
bool SDLVideoOutput::pause() {
    playing = false;
    return true;
}
bool SDLVideoOutput::stop() {
    playing = false;
    return true;
}
void SDLVideoOutput::release() {
    fprintf(stderr, "Released called on the video output!\n");
    reset();
    stop_thread = true;
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}
void SDLVideoOutput::reset() {}

}
