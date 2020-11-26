#pragma once
extern "C" {
    #include <libavutil/avutil.h>
    #include <libavformat/avformat.h>
}

namespace jp {
    
    enum class FramePictureType {
        PICTURE_TYPE_I,
        PICTURE_TYPE_B,
        PICTURE_TYPE_P,
        PICTURE_TYPE_S,
        PICTURE_TYPE_SI,
        PICTURE_TYPE_SP,
        PICTURE_TYPE_BI,
        PICTURE_TYPE_NONE
    };
    
    class IFrame {
    public:
        virtual int get_width() = 0;
        virtual int get_height() = 0;
        
        virtual int64_t get_best_effort_timestamp() = 0;
        virtual int64_t get_packet_position() = 0;
        virtual int64_t get_packet_duration() = 0;
        virtual int64_t get_channels() = 0;
        virtual int64_t get_channel_layout() = 0;
        virtual int64_t get_sample_rate() = 0;
        virtual int64_t get_coded_picture_number() = 0;
        virtual int64_t get_display_picture_number() = 0;
        virtual int64_t get_presentation_timestamp() = 0;
        virtual FramePictureType get_picture_type() = 0;
        virtual char get_picture_type_char() = 0;
        virtual void release() = 0;
        virtual int get_sample_format() = 0;

        /// This is a multi-dimensional array pointing to the data buffers
        /// The size of the array is 8, and the data size contains the length of each of the array (in bytes).
        virtual uint8_t** get_data() = 0;
        /// Returns the data size in bytes for each data plane
        virtual int32_t* get_data_size() = 0;
        virtual int32_t get_number_of_samples() = 0;
    };
}