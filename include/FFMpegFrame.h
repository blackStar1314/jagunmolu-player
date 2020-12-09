#pragma once
#include "IFrame.h"
#include <memory>

namespace jp {
    class FFMpegDecoder;
    class FFMpegResampler;
    class FFMpegFilterGraph;
    class FFMpegMediaPlayer;
    class FFMpegFrame : public IFrame {
    public:
        int get_width() { return internal->width; }
        int get_height() { return internal->height; }
        virtual ~FFMpegFrame() { av_frame_free(&internal); }
        
        int64_t get_best_effort_timestamp() { return internal->best_effort_timestamp; }
        int64_t get_packet_position() { return internal->pkt_pos; }
        int64_t get_packet_duration() { return internal->pkt_duration; }
        int64_t get_channels() { return internal->channels; }
        int64_t get_channel_layout() { return internal->channel_layout; }
        int64_t get_sample_rate() { return internal->sample_rate; }
        int64_t get_coded_picture_number() { return internal->coded_picture_number; }
        int64_t get_display_picture_number() { return internal->display_picture_number; }
        int64_t get_presentation_timestamp() { return internal->pts; }
        FramePictureType get_picture_type() {
            switch (internal->pict_type) {
            case AV_PICTURE_TYPE_B:
                return FramePictureType::PICTURE_TYPE_B;
            case AV_PICTURE_TYPE_BI:
                return FramePictureType::PICTURE_TYPE_BI;
            case AV_PICTURE_TYPE_I:
                return FramePictureType::PICTURE_TYPE_I;
            case AV_PICTURE_TYPE_P:
                return FramePictureType::PICTURE_TYPE_P;
            case AV_PICTURE_TYPE_S:
                return FramePictureType::PICTURE_TYPE_S;
            case AV_PICTURE_TYPE_SI:
                return FramePictureType::PICTURE_TYPE_SI;
            case AV_PICTURE_TYPE_SP:
                return FramePictureType::PICTURE_TYPE_SP;
            case AV_PICTURE_TYPE_NONE:
                return FramePictureType::PICTURE_TYPE_NONE;
            }
            return FramePictureType::PICTURE_TYPE_NONE;
        }
        char get_picture_type_char() { return av_get_picture_type_char(internal->pict_type); }
        void release() { av_frame_unref(internal); }
        int get_sample_format() { return internal->format; }
        
        int get_pixel_format() { return internal->format; }
        
        uint8_t** get_data() { return internal->data; }
        /// Returns the data size in bytes
        int32_t* get_data_size() { return internal->linesize; }
        int32_t get_number_of_samples() { return internal->nb_samples; }
        
        /// Whether the frame is a valid frame
        bool is_valid() { return internal != nullptr; }
        
    private:
        FFMpegFrame() { internal = av_frame_alloc(); }
        friend class FFMpegDecoder;
        friend class FFMpegResampler;
        friend class FFMpegFilterGraph;
        friend class FFMpegMediaPlayer;
        AVFrame* internal;
    };
    
    using FFMpegFrame_Ptr = std::shared_ptr<FFMpegFrame>;
}
