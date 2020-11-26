#pragma once
#include "FFMpegIOContext.h"
#include "FFMpegDemuxer.h"
#include <map>
extern "C" {
	#include <libavformat/avformat.h>
}

namespace jp {
    using Metadata = std::map<std::string, std::string>;
	class FFMpegMedia {
	public:
		FFMpegMedia(FFMpegIOContext_Ptr& context);
		bool parse();
		uint64_t get_sample_rate();
        uint64_t get_bitrate();
        uint64_t get_channels();
        uint64_t get_channel_layout();
        
        AVSampleFormat get_sample_format() {
        	if (demuxer && demuxer->has_audio()) {
        		return demuxer->get_audio_stream()->get_sample_format();
        	}
        	return AV_SAMPLE_FMT_NONE;
        }

        AVPixelFormat get_pixel_format() {
        	if (demuxer && demuxer->has_video()) {
        		return demuxer->get_video_stream()->get_pixel_format();
        	}

        	return AV_PIX_FMT_NONE;
        }

        void release();
        
        FFMpegIOContext_Ptr get_io_context() { return context; }
        
        FFMpegDemuxer_Ptr get_demuxer() { return demuxer; }
        
        bool has_audio() { return demuxer->has_audio(); }
        bool has_video() { return demuxer->has_video(); }
        bool has_subtitles() { return demuxer->has_subtitles(); }

		~FFMpegMedia() { release(); }
		
		/// Whether this media file has been parsed
		bool is_parsed() const { return parsed; }
		
		/// Returns the duration of this media (in milliseconds)
		uint64_t get_duration() { return duration; }
		
		/// Returns all the metadata associated with this media file
		const Metadata get_all_metadata() { return metadata; }
		
		std::string get_metadata(std::string key) {
		    return metadata[key];
		}
		
		/// Sets a metadata
		void set_metadata(std::string key, std::string value) {
		    // Remove the key, in case it exists
		    metadata.erase(key);
		    metadata.insert({key, value});
		}
		
		/// Updates the media metadata
		bool update_metadata() { return false; }
		
		std::string get_error() { return error; }
		
    private:
        FFMpegIOContext_Ptr context;
        FFMpegDemuxer_Ptr demuxer;
        
        std::string path{};
        uint64_t duration{};
        bool parsed{};
        std::string error;
        Metadata metadata{};
	};
	
	using FFMpegMedia_Ptr = FFMpegMedia*;
}
