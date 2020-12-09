#include "FFMpegMedia.h"

namespace jp {

	FFMpegMedia::FFMpegMedia(FFMpegIOContext_Ptr& context) : context(context) {}

	bool FFMpegMedia::parse() {
	    release();
	    demuxer.reset(new FFMpegDemuxer(context));
	    if (!demuxer) {
	        error = "Unable to allocate demuxer";
	        return false;
	    }
	    
	    if (!demuxer->initialize()) {
	        error = demuxer->get_error();
	        return false;
	    }
	    
	    duration = demuxer->get_duration() * demuxer->get_global_time_base() * 1000;
	    parsed = true;
	    
	    return true;
	}

	uint64_t FFMpegMedia::get_sample_rate() {
	    if (!demuxer->has_audio()) return 0;
	    return demuxer->get_audio_stream()->get_sample_rate();
	}

    uint64_t FFMpegMedia::get_bitrate() {
        if (!demuxer->has_audio() && !demuxer->has_video()) return 0;
        if (demuxer->has_audio()) return demuxer->get_audio_stream()->get_bit_rate();
        else return demuxer->get_video_stream()->get_bit_rate();
    }
    
    uint64_t FFMpegMedia::get_channels() {
        if (!demuxer->has_audio()) return 0;
        return demuxer->get_audio_stream()->get_channels();
    }
    
    uint64_t FFMpegMedia::get_channel_layout() {
        if (!demuxer->has_audio()) return 0;
        return demuxer->get_audio_stream()->get_channel_layout();
    }
    
    void FFMpegMedia::release() {
        parsed = false;
    }
	
}
