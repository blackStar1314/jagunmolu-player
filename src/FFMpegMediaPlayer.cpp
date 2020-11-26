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
        
        if (current_media) {
            delete current_media;
        }
        
        current_media = media;
        
        released = false;
        
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
        
        start_demuxer_thread();
        start_audio_thread();
        start_video_thread();
        
        return MediaResult::RESULT_SUCCESS;
    }
    
    void FFMpegMediaPlayer::start_demuxer_thread() {
        if (!demuxer_thread.joinable()) {
            demuxer_thread = std::thread([&]() {
                while (!released) {
                    if (demuxer_clear) {
                        FFMpegPacket_Ptr ptr;
                        while (audio_packet_queue.try_dequeue(ptr)) {}
                        while (video_packet_queue.try_dequeue(ptr)) {}
                        
                        demuxer_clear = false;
                    }
                    
                    auto packet = current_media->get_demuxer()->get_next_packet();

                    // We're done (for now!)
                    if (!packet || packet->is_empty()) {
                        fprintf(stderr, "Packet is empty or invalid packet!\n");
                        if (current_media->get_demuxer()->is_finished()) {
                            fprintf(stderr, "Demuxer says: %s\n", current_media->get_demuxer()->get_error().c_str());
                            break;
                        }
                    }

                    if (packet->is_audio_packet()) {
                        while (!audio_packet_queue.try_enqueue(packet)) {
                            if (released) break;
                        }
                    } else if (packet->is_video_packet()) {
                        while (!video_packet_queue.try_enqueue(packet)) {
                            if (released) break;
                        }
                    }
                }
            });
        }
    }
    
    void FFMpegMediaPlayer::start_audio_thread() {
        if (current_media->has_audio()) {
            if (!audio_output->initialize()) {
                set_error(audio_output->get_error());
                return;
            }

            if (!audio_decoder_thread.joinable()) {
                audio_decoder_thread = std::thread([&]() {
                    auto audio_decoder = current_media->get_demuxer()->get_audio_decoder();
                    FFMpegPacket_Ptr packet;
                    while (!released) {
                        if (audio_decoder->is_flushed()) continue;
                        if (audio_decoder_clear) {
                            FFMpegFrame_Ptr frame;
                            while (audio_frame_queue.try_dequeue(frame)) {}
                            audio_decoder_clear = false;
                        }
                        
                        while (!audio_packet_queue.try_dequeue(packet)) {
                            if (current_media->get_demuxer()->is_finished() || released) break;
                        }
                        
                        // We don't have any more frames to put in the queue
                        if (current_media->get_demuxer()->is_finished()) {
                            audio_decoder->set_finished(true);
                        }
                        
                        // Flush if we've finished decoding all the frames, or flush
                        auto frames = audio_decoder->is_finished() ? audio_decoder->flush() : audio_decoder->decode(packet);
                        std::for_each(frames.begin(), frames.end(), [&](FFMpegFrame_Ptr& frame) {
                            while (!audio_frame_queue.try_enqueue(frame)) {
                                if (!ready) {
                                    ready = true;
                                    if (playing) play();
                                }
                            }
                        });
                    }
                });
                audio_decoder_thread.detach();
            }
        }
    }
    void FFMpegMediaPlayer::start_video_thread() {
        if (current_media->has_video()) {
            if (!video_decoder_thread.joinable()) {
                video_decoder_thread = std::thread([&]() {
                    auto video_decoder = current_media->get_demuxer()->get_video_decoder();
                    FFMpegPacket_Ptr packet;
                    while (!released) {
                        if (video_decoder_clear) {
                            FFMpegFrame_Ptr frame;
                            while (video_frame_queue.try_dequeue(frame)) {}
                            video_decoder_clear = false;
                        }
                        
                        while (!video_packet_queue.try_dequeue(packet)) {
                            if (current_media->get_demuxer()->is_finished() || released) break;
                        }
                        
                        // We don't have any more frames to put in the queue
                        if (current_media->get_demuxer()->is_finished()) {
                            audio_decoder->set_finished(true);
                        }
                        
                        // Flush if we've finished decoding all the frames, or flush
//                        auto frames = video_decoder->is_finished() ? video_decoder->flush() : video_decoder->decode(packet);
//                        std::for_each(frames.begin(), frames.end(), [&](FFMpegFrame_Ptr& frame) {

////                            while (!video_frame_queue.try_enqueue(frame2)) {
////                                if (!ready) {
////                                    ready = true;
////                                    if (playing) play();
////                                }
////                            }
//                        });
                    }
                });
                video_decoder_thread.detach();
            }
        }
    }
     
    MediaResult FFMpegMediaPlayer::play() {
        if (current_media == nullptr || !current_media->is_parsed()) {
            set_error("Media not parsed or media is invalid");
            return MediaResult::RESULT_ERROR;
        }
        if (current_media->has_audio()) {
            playing = true;
            if (ready) {
                if (!audio_output->play()) return MediaResult::RESULT_ERROR;
            }
        }
        return MediaResult::RESULT_SUCCESS;
    }
    
    MediaResult FFMpegMediaPlayer::pause() {
        if (current_media == nullptr || !current_media->is_parsed()) return MediaResult::RESULT_ERROR;
        if (current_media->has_audio())
            audio_output->pause();
        playing = false;
        return MediaResult::RESULT_SUCCESS;
    }
    
    MediaResult FFMpegMediaPlayer::stop() {
        if (current_media == nullptr || !current_media->is_parsed()) return MediaResult::RESULT_ERROR;
        if (current_media->has_audio())
            audio_output->stop();
        playing = false;
        return MediaResult::RESULT_SUCCESS;
    }
    
    bool FFMpegMediaPlayer::seek_to(uint64_t position_millis) {
        ready = false;
        pause();
        audio_output->reset();
        demuxer_clear = true;
        if (current_media->has_audio()) audio_decoder_clear = true;
        if (current_media->has_video()) video_decoder_clear = true;
        play();
        return current_media->get_demuxer()->seek(position_millis / 1000);
    }
    
    FFMpegFrame_Ptr FFMpegMediaPlayer::get_next_audio_frame() {
        FFMpegFrame_Ptr frame;
        FFMpegFrame_Ptr frame2 = FFMpegFrame_Ptr(new FFMpegFrame());
        frame2->internal->sample_rate = current_media->get_sample_rate();
        frame2->internal->channel_layout = current_media->get_channel_layout();
        frame2->internal->channels = current_media->get_channels();
        frame2->internal->format = current_media->get_sample_format();
        if (!audio_frame_queue.try_dequeue(frame)) {
            if (current_media->get_demuxer()->is_finished()) {
                if (filter_graph->add_frame(nullptr)) {
                    if (filter_graph->get_frame(frame2)) {
                        fprintf(stderr, "Time base here: %f", current_media->get_demuxer()->get_audio_stream()->get_time_base());
                        current_position = frame2->get_presentation_timestamp() * current_media->get_demuxer()->get_audio_stream()->get_time_base() * 1000;
                        return frame2;
                    }
                } else {
                    ready = false;
                    audio_output->pause();
                }
            }
            return nullptr;
        }
        
        if (!filter_graph->add_frame(frame)) {
            printf("Unable to add frame to filter graph!\n");
        }
        
        if (!filter_graph->get_frame(frame2)) {
            printf("Unable to get frame from filter graph!\n");
        } else {
            current_position = frame2->get_presentation_timestamp() * current_media->get_demuxer()->get_audio_stream()->get_time_base() * 1000;
        }
        
        return frame2;
    }
    
    void FFMpegMediaPlayer::audio_output_finished() {
        if (current_media->get_demuxer()->is_finished() && current_media->get_demuxer()->get_audio_decoder()->is_finished()) {
            pause();
            current_media->get_demuxer()->reset();
        }
    }
    
    void FFMpegMediaPlayer::release() {
        if (current_media) {
            current_media->release();
            delete current_media;
        }
        released = true;
        if (audio_decoder_thread.joinable()) audio_decoder_thread.join();
        if (video_decoder_thread.joinable()) video_decoder_thread.join();
    }
}
