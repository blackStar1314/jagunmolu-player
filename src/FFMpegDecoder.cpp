#include "FFMpegDecoder.h"

namespace jp {
    /// Decodes this packet and returns the list of decoded frames. If an error occurred, the error string will be set to the specified value and an empty vector will be returned
    std::vector<FFMpegFrame_Ptr> FFMpegDecoder::decode(FFMpegPacket_Ptr packet) {
        std::vector<FFMpegFrame_Ptr> frames;
        int error;
        if ((error = avcodec_send_packet(params.codec_context, packet->internal) >= 0)) {
            FFMpegFrame_Ptr frame_ptr{new FFMpegFrame()};
            
            while ((error = avcodec_receive_frame(params.codec_context, frame_ptr->internal)) >= 0) {
                frames.emplace_back(frame_ptr);
                frame_ptr.reset(new FFMpegFrame());
            }
            
        } else {
            if (error == AVERROR_EOF) {
                this->error = "Decoder flushed! No more receiving packets";
            }
            if (error == AVERROR(EAGAIN)) {
                this->error = "Input not accepted in current state";
            }
            if (error == AVERROR(EINVAL)) {
                this->error = "Invalid argument!";
            }
        }
        
        return frames;
    }
    
    /// Flush this decoder and returns its buffered frames
    std::vector<FFMpegFrame_Ptr> FFMpegDecoder::flush() {
        fprintf(stderr, "Flushing...\n");
        std::vector<FFMpegFrame_Ptr> frames;
        
        if (flushed) return frames;
        
        int error = 0;
        if ((error = avcodec_send_packet(params.codec_context, nullptr)) >= 0) {
            fprintf(stderr, "Sent flush packet!\n");
            FFMpegFrame_Ptr frame_ptr{new FFMpegFrame()};
            
            while ((error = avcodec_receive_frame(params.codec_context, frame_ptr->internal)) >= 0) {
                fprintf(stderr, "Got one flush frame!\n");
                frames.emplace_back(frame_ptr);
                frame_ptr.reset(new FFMpegFrame());
            }
            char buf[2048];
            fprintf(stderr, "Error gotten from decoder: %s\n", av_make_error_string(buf, 2048, error));
            
            if (frames.empty()) {
                fprintf(stderr, "No frame was gotten from flush!\n");
            }
            
        } else {
            fprintf(stderr, "Error while flushing!\n");
            if (error == AVERROR_EOF) {
                this->error = "Decoder flushed! No more receiving frames";
            }
            if (error == AVERROR(EAGAIN)) {
                this->error = "Input not accepted in current state";
            }
            if (error == AVERROR(EINVAL)) {
                this->error = "Invalid argument!";
            }
        }
        
        flushed = true;
        
        fprintf(stderr, "Flushed %zu frames\n", frames.size());
        
        return frames;
    }
    
    void FFMpegDecoder::release() {
        avcodec_free_context(&params.codec_context);
    }

}
