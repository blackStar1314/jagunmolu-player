#include "FFMpegFilter.h"
#include "FFMpegFilterGraph.h"
#include <algorithm>


namespace jp {
FFMpegFilter::FFMpegFilter() {}

bool FFMpegFilter::initialize() {
    std::string final_val;
    std::for_each(properties.begin(), properties.end(), [&final_val] (std::pair<std::string, std::string> pair) {
        final_val += pair.first + "=" + pair.second + ":";
    });
    
    final_val.pop_back();
    
    if (avfilter_init_str(filter_context, final_val.empty() ? nullptr : final_val.c_str()) < 0) {
        fprintf(stderr, "Not initializing filter!\n");
        return false;
    }
    
    return true;
}

void FFMpegFilter::set_property(std::string key, std::string value) {
    properties[key] = value;
}

std::string FFMpegFilter::get_property(std::string key) {
    return properties[key];
}

FFMpegFilter_Ptr FFMpegFilter::get_filter(FFMpegFilterGraph* graph, std::string name) {
    FFMpegFilter_Ptr filt = new FFMpegFilter();
    
    filt->filter = avfilter_get_by_name(name.c_str());
    if (!filt->filter) return nullptr;
    
    filt->filter_context = avfilter_graph_alloc_filter(graph->graph_internal, filt->filter, av_strdup(name.c_str()));
    
    if (!filt->filter_context) {
        return nullptr;
    }
    
    filt->name = name;
    return filt;
}

}
