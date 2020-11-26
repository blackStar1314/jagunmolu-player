#include <iostream>
#include "FFMpegMediaPlayer.h"
#include "SDLAudioOutput.h"
#include "FFMpegIOContext.h"

int main() {
    jp::FFMpegMediaPlayer_Ptr media_player{new jp::FFMpegMediaPlayer()};
    jp::AudioOutput_Ptr audio_output{new jp::SDLAudioOutput(media_player)};
    
    jp::FFMpegIOContext_Ptr io_context{new jp::FFMpegIOContext()};
    if (!io_context->open("/home/smallwondertech/Music/07 Saturdays.m4a", jp::OpenMode::OPEN_MODE_READ)) {
        printf("IO Context couldn't open the file!\n");
        return -1;
    }
    jp::FFMpegMedia_Ptr media{new jp::FFMpegMedia(io_context)};
    if (!media->is_parsed() && !media->parse()) {
        printf("Could not parse media!\n");
        printf("Reason: %s\n", media->get_error().c_str());
        return -1;
    }
    
    printf("Parsed media successfully!\n");
    printf("Media info\n");
    printf("=======================================================\n");
    printf("Path: %s", media->get_io_context()->get_path().c_str());
    printf("Bitrate: %ld\n", media->get_bitrate());
    printf("Sample rate: %ld\n", media->get_sample_rate());
    printf("Channels: %ld\n", media->get_channels());
    printf("Duration: %ld milliseconds\n", media->get_duration());
    
    media_player->set_audio_output(audio_output);
    auto result = media_player->set_media(media);
    if (result == jp::MediaResult::RESULT_ERROR) {
        printf("Error while setting media: %s\n", media_player->get_error().error.c_str());
        return -1;
    }
    
    if (!media_player->seek_to(20000 * 3)) {
        fprintf(stderr, "Couldn't seek!\n");
    } else {
        fprintf(stderr, "Seeked!\n");
    }
    
    printf("Starting playback...\n");
    
    
    if (media_player->play() != jp::MediaResult::RESULT_SUCCESS) {
        printf("Not going to play media! Error: %s\n", media_player->get_error().error.c_str());
        return -1;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    
//    media_player->pause();
    media_player->set_volume(0.2);
//    media_player->play();
    
    while (media_player->is_playing()) {}
    
    return 0;
}
