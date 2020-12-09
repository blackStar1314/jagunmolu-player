#ifndef SUBTITLEMANAGER_H
#define SUBTITLEMANAGER_H
#include <string>
#include <vector>
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

namespace jp {

struct SubtitleEntry {
    std::string text{};
	
	/// The start time of this subtitle entry
    uint64_t start_msecs{0};
	
	/// The end time of this subtitle entry
    uint64_t end_msecs{0};
};

struct SubtitleFile {
	std::string path{};
	std::vector<SubtitleEntry> entries{};
};
class SubtitleManager
{
public:
    SubtitleManager() = default;
	/// Adds and parses this subtitle file
	bool add_subtitle(std::string path);
	/// Removes a subtitle identified by this path
	bool remove_subtitle(std::string path);
	/// Gets the subtitle at the specified index.
	/// This returns one subtitle entry for each added subtitle
	std::vector<std::string> get_subtitle_entries_at(uint64_t position_msecs);
    
    bool has_subtitle() { return subtitles.size() > 0; }
	
private:
    std::vector<SubtitleFile> subtitles{};
};

}

#endif // SUBTITLEMANAGER_H
