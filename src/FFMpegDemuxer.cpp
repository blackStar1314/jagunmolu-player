#include "FFMpegDemuxer.h"

namespace jp {
	FFMpegDemuxer::FFMpegDemuxer(FFMpegIOContext_Ptr& context) : io_context(context) {}
	
	bool FFMpegDemuxer::initialize() {
	    release();
		format_context = avformat_alloc_context();
		if (!format_context) {
			error = "FFMPEG_DEMUXER: Unable to allocate format context!";
			return false;
		}
		
		format_context->pb = io_context->get_context_internal();
		AVFormatContext* context = format_context;
		
		if (avformat_open_input(&context, nullptr, nullptr, nullptr) < 0) {
		    error = "FFMPEG_DEMUXER: Unable to open input file!";
		    return false;
		}
		
		if (avformat_find_stream_info(format_context, nullptr) < 0) {
		    error = "Couldn't find stream info!";
		    return false;
		}
		
		int audio_stream_index = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &audio_decoder_internal, 0);
		has_audio_stream = audio_stream_index >= 0;
		
		int video_stream_index = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &video_decoder_internal, 0);
		has_video_stream = video_stream_index >= 0;
		
		if (!has_audio_stream && !has_video_stream) {
		    error = "No audio or video stream found.";
		    return false;
		}
		
		if (has_audio_stream) {
		    audio_stream = new FFMpegStream();
		    audio_stream->index = audio_stream_index;
		    audio_stream->internal = format_context->streams[audio_stream_index];
		    AVCodecContext* codec_context = avcodec_alloc_context3(audio_decoder_internal);
		    if (!codec_context) {
		        error = "Couldn't allocate context for audio decoder!";
		        return false;
		    }
		    
		    if (avcodec_parameters_to_context(codec_context, audio_stream->internal->codecpar) < 0) {
		        error = "Unable to initialize the audio decoder context";
		        return false;
		    }
		    
		    // Open the codec
		    if (avcodec_open2(codec_context, audio_decoder_internal, nullptr) < 0) {
		        error = "Couldn't open audio decoder";
		        return false;
		    }
            
            audio_decoder_context = codec_context;
		    
		    FFMpegDecoder* decoder = new FFMpegDecoder();
		    DecoderParams decoder_params;
		    decoder_params.codec = audio_decoder_internal;
		    decoder_params.codec_context = audio_decoder_context;
		    decoder->params = decoder_params;
		    audio_decoder = decoder;
		}
		
		if (has_video_stream) {
		    video_stream = new FFMpegStream();
            video_stream->index = video_stream_index;
            video_stream->internal = format_context->streams[video_stream_index];
            AVCodecContext* codec_context = avcodec_alloc_context3(video_decoder_internal);
            if (!codec_context) {
                error = "Couldn't allocate context for video decoder!";
                return false;
            }
            
            if (avcodec_parameters_to_context(codec_context, video_stream->internal->codecpar) < 0) {
                error = "Unable to initialize the video decoder context";
                return false;
            }
            
            // Open the codec
            if (avcodec_open2(codec_context, video_decoder_internal, nullptr) < 0) {
                error = "Couldn't open video decoder";
                return false;
            }
            
            video_decoder_context = codec_context;
            
            FFMpegDecoder* decoder = new FFMpegDecoder();
            DecoderParams decoder_params;
            decoder_params.codec = audio_decoder_internal;
            decoder_params.codec_context = video_decoder_context;
            decoder->params = decoder_params;
            video_decoder = decoder;
        }
        
        reset();
        
        initialized = true;
        finished = false;

		return true;
	}
    
    FFMpegPacket_Ptr FFMpegDemuxer::get_next_packet() {
        if (!initialized) return nullptr;
        FFMpegPacket* packet = new FFMpegPacket();
        if (!packet) {
            if (packet) delete packet;
            error = "Unable to allocate memory for compressed packet";
            return nullptr;
        }
        
        if (finished) {
            return nullptr;
        }
        
        while (true) {
            finished = av_read_frame(format_context, packet->internal) < 0;
            if (packet->internal->stream_index != audio_stream->index && packet->internal->stream_index != video_stream->index) {
                packet->unref();
                continue;
            }
            
            if (packet->internal->stream_index == audio_stream->index)
                packet->audio_packet = true;
            else if (packet->internal->stream_index == video_stream->index)
                packet->video_packet = true;
            
            if (finished) {
                if (packet) delete packet;
                error = "DEMUXER: --EOF--";
                return nullptr;
            }
            
            packet->empty = false;
            return packet;
        }
    }
    
    bool FFMpegDemuxer::seek(uint64_t pos) {
        uint64_t final_position = pos * AV_TIME_BASE;
        return av_seek_frame(format_context, -1, final_position, AVSEEK_FLAG_ANY) >= 0;
    }
    
    void FFMpegDemuxer::release() {
        if (format_context) {
            format_context = nullptr;
        }
        
        if (audio_decoder) {
            delete audio_decoder;
        }
        
        if (video_decoder) {
            delete video_decoder;
        }
        
        if (audio_stream) delete audio_stream;
        if (video_stream) delete video_stream;
    }
}
