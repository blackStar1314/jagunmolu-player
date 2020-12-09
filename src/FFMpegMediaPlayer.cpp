#include "FFMpegMediaPlayer.h"
#include <algorithm>

namespace jp {
    MediaResult FFMpegMediaPlayer::set_media(FFMpegMedia_Ptr media) {
        if (!media) {
            set_error("Media is null");
            return MediaResult::RESULT_ERROR;
        }
        if (!media->is_parsed() && !media->parse()) {
            set_error("Unable to parse media!");
            return MediaResult::RESULT_ERROR;
        }
        
        current_position = 0;
        error.error = "";
        
        current_media = media;
        
        released = false;
        
        if (media->has_audio()) {
            std::string ch_layout = "0x" + std::to_string(media->get_channel_layout());
            std::string tb = std::to_string(media->get_demuxer()->get_audio_stream()->get_time_base_numerator()) + "/" + std::to_string(media->get_demuxer()->get_audio_stream()->get_time_base_denominator());
            
            filter_graph = FFMpegFilterGraph_Ptr(new FFMpegFilterGraph(media->get_sample_format(), ch_layout, media->get_sample_rate(), tb));
            
            if (!filter_graph || !filter_graph->is_initialized()) {
                set_error("Unable to initialize filter graph!");
                return MediaResult::RESULT_ERROR;
            }
            
            auto volume_filter = filter_graph->create_filter("volume");
            volume_filter->set_property("volume", "1.0");
            volume_filter->initialize();
            filter_graph->add_filter(volume_filter);
            
            auto resample_filter = filter_graph->create_filter("aresample");
            resample_filter->set_property("in_channel_layout", "stereo");
            resample_filter->set_property("out_channel_layout", "stereo");
            resample_filter->set_property("in_sample_fmt", av_get_sample_fmt_name(AV_SAMPLE_FMT_S16));
            resample_filter->set_property("out_sample_fmt", av_get_sample_fmt_name(AV_SAMPLE_FMT_S16));
            resample_filter->set_property("in_sample_rate", std::to_string(get_sample_rate()));
            resample_filter->set_property("out_sample_rate", std::to_string(get_sample_rate()));
            resample_filter->initialize();
            filter_graph->add_filter(resample_filter);
            
            if (!filter_graph->configure()) {
                set_error("Unable to configure filter graph!\n");
                return MediaResult::RESULT_ERROR;
            }
            
            if (audio_output) {
                if (!audio_output->initialize()) {
                    set_error("Unable to initialize audio output");
                    return MediaResult::RESULT_ERROR;
                }
            }
            
            audio_decoder = media->get_demuxer()->get_audio_decoder();
            
            audio_enabled = true;
        }
        
        if (media->has_video()) {
            std::string tb = std::to_string(media->get_demuxer()->get_video_stream()->get_time_base_numerator()) + "/" + std::to_string(media->get_demuxer()->get_video_stream()->get_time_base_denominator());
            
            video_filter_graph = FFMpegFilterGraph_Ptr(new FFMpegFilterGraph(media->get_pixel_format(), media->get_width(), media->get_height(), tb));
            
            if (!video_filter_graph || !video_filter_graph->is_initialized()) {
                set_error("Unable to initialize filter graph!");
                return MediaResult::RESULT_ERROR;
            }
            
            if (!video_filter_graph->configure()) {
                set_error("Unable to configure video filter graph!\n");
                return MediaResult::RESULT_ERROR;
            }
            video_decoder = media->get_demuxer()->get_video_decoder();
            
            if (video_output) {
                video_output->set_subtitle_manager(subtitle_manager.get());
                if (!video_output->initialize()) {
                    set_error("Unable to initialize video output!\n");
                }
            }
            
            video_enabled = true;
        }
        
        start_demuxer_thread();
        
        buffering = true;
        
        return MediaResult::RESULT_SUCCESS;
    }
    
    void FFMpegMediaPlayer::start_demuxer_thread() {
        if (!demuxer_thread.joinable()) {
            uint64_t total_bytes;
            demuxer_thread = std::thread([&]() {
                while (!released) {
                    std::unique_lock<std::mutex> lock(demuxer_wake_mutex);
                    while (demuxer_clear) {
                        demuxer_wake_condition.wait(lock);
                    }
                    
                    auto packet = current_media->get_demuxer()->get_next_packet();

                    if (!packet || packet->is_empty()) {
                        if (current_media->get_demuxer()->is_finished()) {
                            fprintf(stderr, "Demuxer says: %s\n", current_media->get_demuxer()->get_error().c_str());
                            fprintf(stderr, "Total demuxed: %luMB\n", total_bytes / (1024 * 1024));
                            demuxer_wake_condition.wait(lock);
                            continue;
                        }
                    }

                    if (packet->is_audio_packet()) {
                        total_bytes += packet->get_bytes();
                        if (!audio_enabled) {
                            continue;
                        }
                        while (!audio_packet_queue.try_enqueue(packet)) {
                            if (released) break;
                            demuxer_wake_condition.wait(lock);
                        }
                    } else if (packet->is_video_packet()) {
                        total_bytes += packet->get_bytes();
                        if (!video_enabled) {
                            continue;
                        }
                        while (!video_packet_queue.try_enqueue(packet)) {
                            if (released) break;
                            demuxer_wake_condition.wait(lock);
                        }
                    }
                }
            });
        }
    }
    
    MediaResult FFMpegMediaPlayer::play() {
        if (current_media == nullptr || !current_media->is_parsed()) {
            set_error("Media not parsed or media is invalid");
            return MediaResult::RESULT_ERROR;
        }
        
        // Return success, we're still buffering
        if (buffering) {
            requested_play = true;
            return MediaResult::RESULT_SUCCESS;
        }
        
        if (current_media->has_audio()) {
            playing = true;
            requested_play = false;
            if (!audio_output->play()) return MediaResult::RESULT_ERROR;
        }
        if (current_media->has_video()) {
            playing = true;
            requested_play = false;
            if (!video_output->play()) return MediaResult::RESULT_ERROR;
        }
        return MediaResult::RESULT_SUCCESS;
    }
    
    MediaResult FFMpegMediaPlayer::pause(bool temp_pause) {
        if (current_media == nullptr || !current_media->is_parsed()) return MediaResult::RESULT_ERROR;
        if (current_media->has_audio())
            audio_output->pause();
        
        if (current_media->has_video()) {
            video_output->pause();
        }
        
        if (temp_pause) requested_play = true;
        
        playing = false;
        return MediaResult::RESULT_SUCCESS;
    }
    
    MediaResult FFMpegMediaPlayer::stop() {
        if (current_media == nullptr || !current_media->is_parsed()) return MediaResult::RESULT_ERROR;
        if (current_media->has_audio())
            audio_output->stop();
        
        if (current_media->has_video()) {
            video_output->stop();
        }
        
        current_media->get_demuxer()->reset();
        
        requested_play = false;
        playing = false;
        return MediaResult::RESULT_SUCCESS;
    }
    
    bool FFMpegMediaPlayer::seek_to(uint64_t position_millis) {
        bool was_playing = playing;
        pause();
        
        if (current_media->get_demuxer()->seek(position_millis)) {
            current_position = position_millis;
        }
        
        if (current_media->has_audio()) audio_output->reset();
        if (current_media->has_video()) video_output->reset();
        
        // Wake the demuxer in case it's sleeping
        demuxer_wake_condition.notify_all();
        
        demuxer_clear = true;
        
        std::unique_lock<std::mutex> lock(demuxer_wake_mutex);
        
        FFMpegPacket_Ptr ptr;
        while (audio_packet_queue.try_dequeue(ptr)) {}
        while (video_packet_queue.try_dequeue(ptr)) {}
        
        demuxer_clear = false;
        
        demuxer_wake_condition.notify_all();
        
        if (video_enabled) {
            video_output->clear_buffer();
        }
        
        if (was_playing) play();
        
        return true;
    }
    
    FFMpegFrame_Ptr FFMpegMediaPlayer::get_next_audio_frame() {
        FFMpegFrame_Ptr frame;
        
        // Do we have any buffered frames?
        while (!audio_frame_queue.try_dequeue(frame)) {
            FFMpegPacket_Ptr packet;
            if (!audio_packet_queue.try_dequeue(packet)) {
                if (current_media->get_demuxer()->is_finished()) {
                    if (audio_decoder->is_flushed()) return nullptr;
                    auto result = audio_decoder->flush();
                    if (result.empty()) return nullptr;
                    std::for_each(result.begin(), result.end(), [&](FFMpegFrame_Ptr frame) {
                        audio_frame_queue.enqueue(frame);
                    });
                    continue;
                }
            } else {
                auto result = audio_decoder->decode(packet);
                std::for_each(result.begin(), result.end(), [&](FFMpegFrame_Ptr frame) {
                    audio_frame_queue.enqueue(frame);
                });
            }
        }
        
        demuxer_wake_condition.notify_all();
        
        if (!filter_graph->add_frame(frame)) {
            printf("Unable to add frame to filter graph!\n");
        }
        
        FFMpegFrame_Ptr frame2 = FFMpegFrame_Ptr(new FFMpegFrame());
        frame2->internal->sample_rate = current_media->get_sample_rate();
        frame2->internal->channel_layout = current_media->get_channel_layout();
        frame2->internal->channels = current_media->get_channels();
        frame2->internal->format = current_media->get_sample_format();
        
        if (!filter_graph->get_frame(frame2)) {
            printf("Unable to get frame from filter graph!\n");
        } else {
            if (!video_enabled) {
                current_position = frame->get_presentation_timestamp() * current_media->get_demuxer()->get_audio_stream()->get_time_base() * 1000;
            }
        }
        
        return frame2;
    }
    
    FFMpegFrame_Ptr FFMpegMediaPlayer::get_next_video_frame() {
        FFMpegFrame_Ptr frame2 = FFMpegFrame_Ptr(new FFMpegFrame());
        frame2->internal->width = current_media->get_width();
        frame2->internal->height = current_media->get_height();
        frame2->internal->format = AV_PIX_FMT_RGB24;
        
        while (true) {
            FFMpegPacket_Ptr packet;
            while (!video_packet_queue.try_dequeue(packet)) {
                if (current_media->get_demuxer()->get_video_stream()->is_attached_pic() || current_media->get_demuxer()->is_finished()) {
                    return nullptr;
                }
            }
            
            demuxer_wake_condition.notify_all();
            
            auto frames = video_decoder->decode(packet);
            if (frames.empty()) {
                continue;
            }
            
            auto frame = frames[0];
            
            if (!video_filter_graph->add_frame(frame)) {
                printf("Unable to add frame to video filter graph!\n");
            }
            
            if (!video_filter_graph->get_frame(frame2)) {
                printf("Unable to get frame from video filter graph!\n");
            } else {
                break;
            }
        }
        
        return frame2;
    }
    
    bool FFMpegMediaPlayer::add_subtitle(std::string path) { return subtitle_manager->add_subtitle(path); }
    
    void FFMpegMediaPlayer::buffering_changed() {
        if (audio_enabled) {
            buffering.store(audio_output->is_buffering() || audio_output->is_buffering());
            fprintf(stderr, "Audio Buffering: %d\n", audio_output->is_buffering());
        }
        if (video_enabled) {
            buffering.store(audio_enabled ? buffering || video_output->is_buffering() : video_output->is_buffering());
            fprintf(stderr, "Video Buffering: %d\n", video_output->is_buffering());
        }
        
        if (!buffering) {
            if (requested_play) {
                play();
            }
        } else {
            if (playing) pause(true);
        }
    }
    
    void FFMpegMediaPlayer::release() {
        released = true;
        if (demuxer_thread.joinable()) demuxer_thread.join();
    }
}
