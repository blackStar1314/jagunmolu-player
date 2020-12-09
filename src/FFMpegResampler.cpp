#include "FFMpegResampler.h"

namespace jp {
	FFMpegResampler::FFMpegResampler() {
		resampler_context = swr_alloc();
	    if (!resampler_context) {
	        // SWResampler could not be initialized
	        initialized = false;
	    }
	}
	
	bool FFMpegResampler::initialize(int64_t src_channel_layout, int  src_sample_rate, AVSampleFormat src_sample_fmt,
									int64_t out_channel_layout, int out_sample_rate, AVSampleFormat out_sample_fmt) {
        if (!resampler_context) return false;
		av_opt_set_int(resampler_context, "in_channel_layout",    src_channel_layout, 0);
	    av_opt_set_int(resampler_context, "in_sample_rate",       src_sample_rate, 0);
	    av_opt_set_sample_fmt(resampler_context, "in_sample_fmt", src_sample_fmt, 0);

	    av_opt_set_int(resampler_context, "out_channel_layout",    out_channel_layout, 0);
	    av_opt_set_int(resampler_context, "out_sample_rate",       out_sample_rate, 0);
	    av_opt_set_sample_fmt(resampler_context, "out_sample_fmt", out_sample_fmt, 0);

	    this->out_channel_layout = out_channel_layout;
	    this->out_sample_rate = out_sample_rate;
	    this->out_sample_fmt = out_sample_fmt;
	    this->in_channel_layout = src_channel_layout;
	    this->in_sample_rate = src_sample_rate;
	    this->in_sample_fmt = src_sample_fmt;

	    return swr_init(resampler_context) >= 0;
	}
	
	FFMpegFrame_Ptr FFMpegResampler::resample(FFMpegFrame_Ptr& frame) {
        
        if (!resampler_context) return nullptr;
        
        FFMpegFrame_Ptr frame_ptr{new FFMpegFrame()};

		if (!frame_ptr) return nullptr;

		AVFrame* src = frame->internal;
		AVFrame* dest = frame_ptr->internal;
		dest->format = out_sample_fmt;
		dest->sample_rate = out_sample_rate;
		dest->channel_layout = out_channel_layout;

		int ret;

	    if ((ret = swr_convert_frame(resampler_context, dest, src)) < 0) {
	        fprintf(stderr, "Unable to resample frame!\n");

	        if (ret & AVERROR_OUTPUT_CHANGED && frame != nullptr) {
	            fprintf(stderr, "Output has changed!\n");
	        }
	        if (ret & AVERROR_INPUT_CHANGED && frame != nullptr) {
	            fprintf(stderr, "Input has changed!\n");
	        }
	        return nullptr;
	    }

		return frame_ptr;
	}
}

