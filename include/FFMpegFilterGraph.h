#ifndef FFMPEGFILTERGRAPH_H
#define FFMPEGFILTERGRAPH_H
#include "FFMpegFilter.h"
#include <vector>
#include "FFMpegFrame.h"

extern "C" {
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}

namespace jp {

/// This class contains the filters for a particular filter graph
/// After adding and removing filters from this class, make sure to call configure, so that the filters can be linked. You can also move the filters around the chain, so that it is processed as you'd like. Filters are linked according to the order of insertion into the list
/// The filter graph always contain a filter at the beginning (the source filter) and at the end (the sink filter)
class FFMpegFilterGraph
{
public:
    /// Provide info about the data that will be put inside this graph
    FFMpegFilterGraph(int sample_format, std::string channel_layout, int sample_rate, std::string time_base);
    
    ~FFMpegFilterGraph();
    
    /// Adds a filter to the end of the filter graph. If for some reason you pass a null filter to this graph, it does not add it to the filter graph. Returns true if operation completed successfully and false otherwise
    bool add_filter(FFMpegFilter_Ptr filter);
    
    /// Removes the specified filter at this index. Returns true if operation is successful and false otherwise
    bool remove_filter(int index);
    
    /// Removes the specified filter if it's in the list and returns true. Returns false otherwise
    bool remove_filter(FFMpegFilter_Ptr filter);
    
    /// Moves this filter to the specified index. Note that this will push down the other filters indices from the specified index. Returns true if operation succeeds and false otherwise
    /// If you try to move a filter that is not already in the list, this function does nothing and just returns false
    bool move_filter(FFMpegFilter_Ptr filter, int filter_index, int new_index);
    
    /// Returns the filter at the specified index if it exists and null otherwise
    FFMpegFilter_Ptr get_filter(int index);
    
    FFMpegFilter_Ptr get_filter(std::string name);
    
    /// Configures the filter graph and links the filters. Returns true if the graph has been configured and false otherwise
    bool configure();
    
    /// Return the set error message by the filter graph
    std::string get_error() { return error; }
    
    FFMpegFilter_Ptr create_filter(std::string name);
    
    bool get_frame(FFMpegFrame_Ptr frame) {
        return av_buffersink_get_frame(output->filter_context, frame->internal) >= 0;
    }
    
    size_t get_filters_size() { return filters.size(); }
    auto get_filters() { return filters; }
    
    bool add_frame(FFMpegFrame_Ptr frame) {
        if (!initialized) return false;
        return av_buffersrc_add_frame(input->filter_context, (!frame) ? nullptr : frame->internal) >= 0;
    }
    
    bool send_command(FFMpegFilter_Ptr filter, std::string value) {
        char response[2048];
        int result;
        if ((result = avfilter_graph_send_command(graph_internal.get(), filter->get_name().c_str(), filter->get_name().c_str(), value.c_str(), response, 2048, 0) < 0)) {
            fprintf(stderr, "Unable to send command! Response: %s", response);
            fprintf(stderr, "AVERROR(ENOSYS)?: %s", av_make_error_string(response, 2048, result));
        } else {
            fprintf(stderr, "Sent command! Response: %s\n", response);
        }
        
        return false;
    }
    
    bool is_initialized() { return initialized; }
private:
    friend class FFMpegFilter;
    std::string error{};
    std::vector<FFMpegFilter_Ptr> filters{};
    AVFilterGraph* graph_internal{nullptr};
    bool initialized{false};
    FFMpegFilter_Ptr input{nullptr};
    FFMpegFilter_Ptr output{nullptr};
    AVFilterLink* link{};
    bool configured{false};
};

using FFMpegFilterGraph_Ptr = std::shared_ptr<FFMpegFilterGraph>;

}

#endif // FFMPEGFILTERGRAPH_H
