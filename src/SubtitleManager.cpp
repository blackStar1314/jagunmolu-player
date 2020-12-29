#include "SubtitleManager.h"
#include <algorithm>

namespace jp {

bool SubtitleManager::add_subtitle(std::string path) {
    // Check whether the path already exists!
    fprintf(stderr, "Adding subtitle: %s\n", path.c_str());
    bool proceed = true;
    std::for_each(subtitles.begin(), subtitles.end(), [&](SubtitleFile& file) {
        if (file.path == path) {
            fprintf(stderr, "Not proceeding\n");
            proceed = false;
        }
    });
    
    if (!proceed) return false;
    
    AVFormatContext* ctx = nullptr;
    const char* url = av_strdup(path.c_str());
    
    if (avformat_open_input(&ctx, url, nullptr, nullptr) < 0) {
        fprintf(stderr, "No subtitle path!\n");
        return false;
    }

    av_free((void*)url);
    
    if (!ctx) {
        fprintf(stderr, "Could not initialize FormatContext!\n");
        return false;
    }
    
    fprintf(stderr, "Moving forward...\n");
    
    if (avformat_find_stream_info(ctx, nullptr) < 0) {
        fprintf(stderr, "Unable to find stream info!\n");
        return false;
    }
    
    fprintf(stderr, "Finding best stream...\n");
    
    int index = av_find_best_stream(ctx, AVMEDIA_TYPE_SUBTITLE, -1, -1, nullptr, 0);
    if (index < 0) {
        fprintf(stderr, "No subtitle stream found!\n");
        return false;
    }
    
    SubtitleFile file{};
    file.path = path;
    
    AVStream* sub_stream = ctx->streams[index];
    
    AVPacket* packet = av_packet_alloc();
    while (av_read_frame(ctx, packet) >= 0) {
        if (packet->stream_index == index) {
            SubtitleEntry entry{};
            entry.text = std::string((char*)packet->data);
            entry.start_msecs = packet->pts * av_q2d(sub_stream->time_base) * 1000;
            entry.end_msecs = (packet->pts + packet->duration) * av_q2d(sub_stream->time_base) * 1000;
            file.entries.push_back(entry);
        }
        av_packet_unref(packet);
    }
    
    subtitles.push_back(file);
    
    avformat_close_input(&ctx);
    av_packet_free(&packet);
    
    fprintf(stderr, "Finished parsing subtitles!\n");
    
    return true;
}

bool SubtitleManager::remove_subtitle(std::string path) {
    for (auto iter = subtitles.begin(); iter != subtitles.end(); ++iter) {
        if ((*iter).path == path) {
            subtitles.erase(iter);
            return true;
        }
    }
    
    return false;
}

std::vector<std::string> SubtitleManager::get_subtitle_entries_at(uint64_t position_msecs) {
    std::vector<std::string> subtitles_str;
    
    std::for_each(subtitles.begin(), subtitles.end(), [&](SubtitleFile& file) {
        std::for_each(file.entries.begin(), file.entries.end(), [&](SubtitleEntry& entry) {
            if (position_msecs >= entry.start_msecs && position_msecs <= entry.end_msecs) {
                subtitles_str.push_back(entry.text);
            }
        });
    });
    
    return subtitles_str;
}

}
