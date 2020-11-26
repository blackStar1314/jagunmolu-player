#pragma once
#include <memory>

extern "C" {
    #include <libavformat/avformat.h>
}

namespace jp {
    class FFMpegDemuxer;
    class FFMpegDecoder;
    class FFMpegPacket {
    public:
        bool is_empty() { return empty; }
        
        /// Return the presentation timestamp in milliseconds
        int64_t get_pts() { return internal->pts; }
        
        /// Return the decompression timestamp in milliseconds
        int64_t get_dts() { return internal->dts; }
        
        int64_t get_byte_position() { return internal->pos; }
        
        int64_t get_duration() { return internal->duration; }
        
        void unref() { av_packet_unref(internal); }
        
        void release() {
            av_packet_free(&internal);
        }
        
        ~FFMpegPacket() { release(); }
        
        bool is_audio_packet() { return audio_packet; }
        bool is_video_packet() { return video_packet; }
        bool is_subtitle_packet() { return subtitle_packet; }
        
        uint8_t* get_data() { return internal->data; }
        
        bool is_valid() { return internal != nullptr; }
        
    private:
        friend class FFMpegDemuxer;
        friend class FFMpegDecoder;
        FFMpegPacket() { internal = av_packet_alloc(); }
        bool empty{true};
        AVPacket* internal{nullptr};
        bool audio_packet{false};
        bool video_packet{false};
        bool subtitle_packet{false};
    };
    
    using FFMpegPacket_Ptr = FFMpegPacket*;
}
