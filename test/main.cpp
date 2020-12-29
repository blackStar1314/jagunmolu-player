#include <iostream>
#include "FFMpegMediaPlayer.h"
#include "SDLAudioOutput.h"
#include "SDLVideoOutput.h"
#include "FFMpegIOContext.h"

int main() {
    jp::FFMpegMediaPlayer_Ptr media_player{new jp::FFMpegMediaPlayer()};
    jp::AudioOutput_Ptr audio_output{new jp::SDLAudioOutput(media_player)};
    jp::VideoOutput_Ptr video_output{new jp::SDLVideoOutput(media_player)};
    
    jp::FFMpegIOContext_Ptr io_context{new jp::FFMpegIOContext()};
    if (!io_context->open("/home/smallwondertech/Desktop/CodeIgniter 4 RESTapi Server - Part 1 - OAuth 2.0 Authorization.mp4", jp::OpenMode::OPEN_MODE_READ)) {
        fprintf(stderr, "IO Context couldn't open the file!\n");
        return -1;
    }
    jp::FFMpegMedia_Ptr media{new jp::FFMpegMedia(io_context)};
    if (!media->is_parsed() && !media->parse()) {
        fprintf(stderr, "Could not parse media!\n");
        fprintf(stderr, "Reason: %s\n", media->get_error().c_str());
        return -1;
    }
    
    fprintf(stderr, "Parsed media successfully!\n");
    fprintf(stderr, "Media info\n");
    fprintf(stderr, "=======================================================\n");
    fprintf(stderr, "Path: %s\n", media->get_io_context()->get_path().c_str());
    fprintf(stderr, "Bitrate: %ld\n", media->get_bitrate());
    fprintf(stderr, "Sample rate: %ld\n", media->get_sample_rate());
    fprintf(stderr, "Channels: %ld\n", media->get_channels());
    fprintf(stderr, "Duration: %ld milliseconds\n", media->get_duration());
    
    media_player->set_audio_output(audio_output);
    
    media_player->set_video_output(video_output);
    
    media_player->add_subtitle(media->get_io_context()->get_path());
    auto result = media_player->set_media(media);
    
    media_player->set_volume(2.0);
    
    if (result == jp::MediaResult::RESULT_ERROR) {
        fprintf(stderr, "Error while setting media: %s\n", media_player->get_error().error.c_str());
        return -1;
    }
    
    fprintf(stderr, "Starting playback...\n");
    
    if (media_player->play() != jp::MediaResult::RESULT_SUCCESS) {
        fprintf(stderr, "Not going to play media! Error: %s\n", media_player->get_error().error.c_str());
        return -1;
    }
    
    SDL_Init(SDL_INIT_EVENTS);

    // I know this is probably fucked up, but I don't see any other way (yet)
    SDL_Event event;
    while (true) {
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                // Stop playback, we're done here...
                break;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    if (media_player->is_playing()) {
                        media_player->pause();
                    } else media_player->play();
                } else if (event.key.keysym.sym == SDLK_RIGHT) {
                    // Seek 10 seconds by default
                    media_player->seek_to(media_player->get_position() + 10000);
                } else if (event.key.keysym.sym == SDLK_LEFT) {
                    media_player->seek_to(media_player->get_position() - 10000);
                }
            }
        }
        SDL_Delay(30);
    }
    
    return 0;
}
