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
    if (!io_context->open("/home/adebayo/Videos/Uri The Surgical Strike (2019) (NetNaija.com).mp4", jp::OpenMode::OPEN_MODE_READ)) {
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
    printf("Path: %s\n", media->get_io_context()->get_path().c_str());
    printf("Bitrate: %ld\n", media->get_bitrate());
    printf("Sample rate: %ld\n", media->get_sample_rate());
    printf("Channels: %ld\n", media->get_channels());
    printf("Duration: %ld milliseconds\n", media->get_duration());
    
    media_player->set_audio_output(audio_output);
    
    media_player->set_video_output(video_output);
    
    
    media_player->add_subtitle(media->get_io_context()->get_path());
    auto result = media_player->set_media(media);
    
    if (result == jp::MediaResult::RESULT_ERROR) {
        printf("Error while setting media: %s\n", media_player->get_error().error.c_str());
        return -1;
    }
    media_player->add_subtitle("/home/adebayo/Videos/Uri The Surgical Strike (2019) (NetNaija.com).srt");
    
    fprintf(stderr, "Starting playback...\n");
    
    if (media_player->play() != jp::MediaResult::RESULT_SUCCESS) {
        fprintf(stderr, "Not going to play media! Error: %s\n", media_player->get_error().error.c_str());
        return -1;
    }
    
    while (media_player->is_playing()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    
    fprintf(stderr, "Aspect ratio: %d / %d\n", media->get_demuxer()->get_video_stream()->get_display_aspect_ratio_numerator(), media->get_demuxer()->get_video_stream()->get_display_aspect_ratio_denominator());
    
    return 0;
}
