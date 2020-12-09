#pragma once
#include "FFMpegDecoder.h"
#include "FFMpegIOContext.h"
#include "FFMpegStream.h"

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/avutil.h>
}

namespace jp {
	class FFMpegDemuxer {
	public:
		FFMpegDemuxer(FFMpegIOContext_Ptr& context);
		~FFMpegDemuxer() { release(); }

		bool initialize();
        bool has_audio() { return has_audio_stream; }
        bool has_video() { return has_video_stream; }
        FFMpegPacket_Ptr get_next_packet();
        void release();
        FFMpegDecoder_Ptr get_audio_decoder() { return audio_decoder; }
        FFMpegDecoder_Ptr get_video_decoder() { return video_decoder; }
        bool is_initialized() { return initialized; }
        
        FFMpegStream_Ptr get_audio_stream() { return audio_stream; }
        FFMpegStream_Ptr get_video_stream() { return video_stream; }
        
        uint64_t get_duration() { return format_context->duration; }
        double get_global_time_base() { return 1.0 / (double)AV_TIME_BASE; }
        
        std::string get_error() { return error; }
        
        bool is_finished() { return finished; }
        
        /// Seek to specified position. This should be in seconds
        bool seek(uint64_t position);
        
        void reset() {
            if (!seek(0)) {
                fprintf(stderr, "Demuxer couldn't seek back to the beginning...\n");
            }
            finished = false;
            if (audio_decoder) audio_decoder->set_finished(false);
            if (video_decoder) video_decoder->set_finished(false);
        }

	private:
        FFMpegIOContext_Ptr io_context{nullptr};
		AVFormatContext* format_context{nullptr};
		AVCodecContext* audio_decoder_context{nullptr};
        AVCodec* audio_decoder_internal{nullptr};
		AVCodecContext* video_decoder_context{nullptr};
        AVCodec* video_decoder_internal{nullptr};

        FFMpegDecoder_Ptr audio_decoder{nullptr};
        FFMpegDecoder_Ptr video_decoder{nullptr};
		std::string error;
		
        FFMpegStream_Ptr audio_stream{nullptr};
        FFMpegStream_Ptr video_stream{nullptr};
        FFMpegStream_Ptr subtitle_stream{nullptr};
		
		bool has_audio_stream{false};
		bool has_video_stream{false};
		bool initialized{false};
		bool finished{true};
	};
	
	using FFMpegDemuxer_Ptr = std::shared_ptr<FFMpegDemuxer>;
}
