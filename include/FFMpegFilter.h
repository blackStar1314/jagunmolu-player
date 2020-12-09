#ifndef FFMPEGFILTER_H
#define FFMPEGFILTER_H
#include <iostream>
#include <memory>
#include <map>

extern "C" {
#include <libavfilter/avfilter.h>
}

namespace jp {
class FFMpegFilter;
class FFMpegFilterGraph;

using FFMpegFilter_Ptr = std::shared_ptr<FFMpegFilter>;

/// After setting all the filter properties, make sure to call initialize on the filter instance. Failure to call this might cause problems
class FFMpegFilter
{
public:
    
    bool initialize();
    
    /// Sets a filter property. Invalid properties set will be ignored
    void set_property(std::string key, std::string value);
    
    /// Gets a property from this filter. Returns an empty string if the filter's property does not have the specified key
    std::string get_property(std::string key);
    
    ~FFMpegFilter() {
        avfilter_free(filter_context);
    }
    
    std::string get_name() { return name; }
    
private:
    friend class FFMpegFilterGraph;
    FFMpegFilter();
    std::string name{};
    AVFilterContext* filter_context{nullptr};
    const AVFilter* filter{nullptr};
    std::map<std::string, std::string> properties;
    
    /// Finds and returns a filter with this name. Returns a valid pointer to the FFMpegFilter object if it exists and nullptr if it doesn't
    static FFMpegFilter_Ptr get_filter(FFMpegFilterGraph* graph, std::string name);
};

}

#endif // FFMPEGFILTER_H
