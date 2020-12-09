#pragma once
extern "C" {
    #include <libavformat/avformat.h>
}
#include <memory>

namespace jp {
    class FFMpegDemuxer;
    struct FFMpegStream {
        double get_display_aspect_ratio() { return av_q2d(internal->display_aspect_ratio); }
        int get_display_aspect_ratio_numerator() { return internal->display_aspect_ratio.num; }
        int get_display_aspect_ratio_denominator() { return internal->display_aspect_ratio.den; }
        double get_time_base() { return av_q2d(internal->time_base); }
        double get_frame_rate() { return av_q2d(internal->r_frame_rate); }
        
        int get_number_of_frames() { return internal->nb_frames; }
        
        /// This value must be multiplied by the time_base to get the duration in seconds
        int get_duration() { return internal->duration; }
        
        bool is_valid() { return index != -1; }
        
        int get_sample_rate() { return internal->codecpar->sample_rate; }

        AVPixelFormat get_pixel_format() { return static_cast<AVPixelFormat>(internal->codecpar->format); }
        
        AVSampleFormat get_sample_format() { return static_cast<AVSampleFormat>(internal->codecpar->format); }
        
        int get_bit_rate() { return internal->codecpar->bit_rate; }
        
        int get_channels() { return internal->codecpar->channels; }
        
        int64_t get_channel_layout() { return internal->codecpar->channel_layout; }
        
        int get_time_base_numerator() {
            return internal->time_base.num;
        }
        
        int get_time_base_denominator() {
            return internal->time_base.den;
        }
        
        int get_width() { return internal->codecpar->width; }
        int get_height() { return internal->codecpar->height; }
        
        bool is_attached_pic() { return attached_pic; }
        
    private:
        friend class FFMpegDemuxer;
        FFMpegStream() = default;
        AVStream* internal{nullptr};
        int index{-1};
        bool attached_pic{false};
    };
    
    using FFMpegStream_Ptr = std::shared_ptr<FFMpegStream>;
}
