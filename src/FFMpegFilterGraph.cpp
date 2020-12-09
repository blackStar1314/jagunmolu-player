#include "FFMpegFilterGraph.h"
#include <algorithm>

namespace jp {

FFMpegFilterGraph::FFMpegFilterGraph(int sample_format, std::string channel_layout, int sample_rate, std::string time_base) {
    initialized = true;
    graph_internal = avfilter_graph_alloc();
    if (!graph_internal) {
        error = "Unable to allocate filter graph!";
        initialized = false;
        
        return;
    }
    
    avfilter_graph_set_auto_convert(graph_internal, AVFILTER_AUTO_CONVERT_ALL);
    
    input = create_filter("abuffer");
    input->set_property("sample_fmt", std::to_string(sample_format));
    input->set_property("channel_layout", channel_layout);
    input->set_property("sample_rate", std::to_string(sample_rate));
//    input->set_property("time_base", time_base);
    if (!input->initialize()) initialized = false;
    
    output = create_filter("abuffersink");
    if (!output->initialize()) initialized = false;
}

FFMpegFilterGraph::FFMpegFilterGraph(int pixel_format, int width, int height, std::string time_base) {
    initialized = true;
    graph_internal = avfilter_graph_alloc();
    if (!graph_internal) {
        error = "Unable to allocate filter graph!";
        initialized = false;
        
        return;
    }
    
    avfilter_graph_set_auto_convert(graph_internal, AVFILTER_AUTO_CONVERT_ALL);
    
    input = create_filter("buffer");
    input->set_property("width", std::to_string(width));
    input->set_property("pix_fmt", std::to_string(pixel_format));
    input->set_property("height", std::to_string(height));
    input->set_property("time_base", time_base);
    if (!input->initialize()) initialized = false;
    
    output = create_filter("buffersink");
    if (!output->initialize()) initialized = false;
}

FFMpegFilterGraph::~FFMpegFilterGraph() {
    fprintf(stderr, "Cleaning up filters...");
    avfilter_graph_free(&graph_internal);
    filters.clear();
}

FFMpegFilter_Ptr FFMpegFilterGraph::create_filter(std::string name) {
    return FFMpegFilter::get_filter(this, name);
}

bool FFMpegFilterGraph::add_filter(FFMpegFilter_Ptr filter) {
    if (!filter) return false;
    
    filters.push_back(filter);
    
    return true;
}

bool FFMpegFilterGraph::remove_filter(int index) {
    if (index >= (int) filters.size()) return false;
    auto filter = filters[index];
    
    return remove_filter(filter);
}

bool FFMpegFilterGraph::remove_filter(FFMpegFilter_Ptr filter) {
    auto iter = filters.begin();
    for (;iter != filters.end(); ++iter) {
        if ((*iter) == filter) {
            filters.erase(iter);
            return true;
        }
    }
    
    return false;
}

bool FFMpegFilterGraph::move_filter(FFMpegFilter_Ptr filter, int filter_index, int new_index) {
    // Same index
    if (filter_index == new_index) return true;
    
    if (filter_index < 0 || new_index < 0 || filter_index >= (int)filters.size() || new_index >= (int)filters.size()) return false;
    
    // 0 1 2 3 4
    // 0 2 3 4
    // 0 2 1 3 4
    auto temp = get_filter(filter_index);
    auto new_filter = get_filter(new_index);
    
    // Check for filter pointer equality
    if (temp != filter) return false;
    
    if (!temp || !new_filter) {
        return false;
    }
    
    if (!remove_filter(filter)) {
        return false;
    }
    
    if (filter_index > new_index) {
        auto iter = filters.begin();
        for (; iter != filters.end(); iter++) {
            
            if (*iter == new_filter) {
                filters.emplace(iter);
                return true;
            }
        }
    } else {
        auto iter = filters.begin();
        for (; iter != filters.end(); iter++) {   
            if (*iter == new_filter) {
                filters.emplace(++iter);
                return true;
            }
        }
    }
    
    return false;
}

FFMpegFilter_Ptr FFMpegFilterGraph::get_filter(int index) {
    if (index >= (int) filters.size() || index < 0) return nullptr;
    return filters[index];
}

bool FFMpegFilterGraph::configure() {
    // Take all the filters in the vector and link them together
    bool good = true;
    
    if (configured) {
        return true;
    }
    
    if (filters.empty()) {
        good = avfilter_link(input->filter_context, 0, output->filter_context, 0) >= 0;
        if (!good) {
            return false;
        }
    }
    
    for (size_t i = 0; i < filters.size(); i++) {
        if (i == 0) {
            fprintf(stderr, "Linking src to %s\n", filters[i]->get_name().c_str());
            good = avfilter_link(input->filter_context, 0, filters[i]->filter_context, 0) >= 0;
            if (!good) {
                fprintf(stderr, "Couldn't link src to %s\n", filters[i]->get_name().c_str());
                return false;
            }
        }
        
        if (i == filters.size() - 1) {
            fprintf(stderr, "Linking %s to sink\n", filters[i]->get_name().c_str());
            good = avfilter_link(filters[i]->filter_context, 0, output->filter_context, 0) >= 0;
        } else {
            good = avfilter_link(filters[i]->filter_context, 0, filters[i + 1]->filter_context, 0) >= 0;
            fprintf(stderr, "Linking %s to %s\n", filters[i]->get_name().c_str(), filters[i + 1]->get_name().c_str());
        }
        if (!good) return false;
    }
    
    if (avfilter_graph_config(graph_internal, nullptr) < 0) {
        fprintf(stderr, "Not configuring filter!\n");
        return false;
    }
    
    configured = true;
    
    return true;
}

}















