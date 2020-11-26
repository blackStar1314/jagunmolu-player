#ifndef FFMPEGRESAMPLER_H
#define FFMPEGRESAMPLER_H
extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

#include <memory>
#include "FFMpegFrame.h"

namespace jp {

class FFMpegResampler
{
public:
    /// Creates an empty resampler
    FFMpegResampler();
    ~FFMpegResampler() { swr_free(&resampler_context); }
    
    /// Initializes this resampler with input and output options
    bool initialize(int64_t src_channel_layout, int  src_sample_rate, AVSampleFormat src_sample_fmt,
					int64_t out_channel_layout, int out_sample_rate, AVSampleFormat out_sample_fmt);
    
    /// Resamples the given frame and returns the resampled frame
    /// Note that this will return nullptr when invalid frames are provided
    FFMpegFrame_Ptr resample(FFMpegFrame_Ptr& frame);

    bool is_initialized() { return initialized; }
    
private:
    SwrContext* resampler_context{};
	int64_t in_channel_layout{0};
    int64_t in_sample_rate{0};
    AVSampleFormat in_sample_fmt{};
    int64_t out_channel_layout{0};
    int64_t out_sample_rate{0};
    AVSampleFormat out_sample_fmt{};

    bool initialized{false};
    
};

using FFMpegResampler_Ptr = FFMpegResampler;

}

#endif // FFMPEGRESAMPLER_H
