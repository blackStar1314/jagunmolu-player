#pragma once

#include <string>
#include <vector>
#include "FFMpegPacket.h"
#include "FFMpegFrame.h"

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libavcodec/avcodec.h>
}

namespace jp {
    enum class DecoderType { DECODER_TYPE_AUDIO, DECODER_TYPE_VIDEO, DECODER_TYPE_SUBTITLE };
    struct DecoderParams {
        AVCodec* codec;
        AVCodecContext* codec_context;
    };
    
    class FFMpegDemuxer;
    
    class FFMpegDecoder {
    public:        
        /// Returns the name of this decoder
        std::string get_name() {
            if (params.codec == nullptr) return "<unknown>";
            return avcodec_get_name(params.codec_context->codec_id);
        }
        
        /// Decodes this packet and returns the list of decoded frames. If an error occurred, the error string will be set to the specified value and an empty vector will be returned
        std::vector<FFMpegFrame_Ptr> decode(FFMpegPacket_Ptr packet);
        
        /// Flush this decoder and returns its buffered frames
        std::vector<FFMpegFrame_Ptr> flush();
        
        std::string get_error() { return error; }
        
        void release();
        
        bool is_finished() { return finished; }
        void set_finished(bool value) { finished = value; }
        
        ~FFMpegDecoder() { release(); }
        
        DecoderType getType() { return type; }
        
    private:
        FFMpegDecoder(DecoderType type) : type(type) {}
        friend class FFMpegDemuxer;
        DecoderType type;
        std::string error;
        DecoderParams params{};
        bool finished{false};
    };
    
    using FFMpegDecoder_Ptr = std::shared_ptr<FFMpegDecoder>;
}
