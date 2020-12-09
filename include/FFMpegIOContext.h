#pragma once
#include <iostream>
#include <memory>

extern "C" {
	#include <libavformat/avformat.h>
}

namespace jp {
    
    enum class OpenMode { OPEN_MODE_READ, OPEN_MODE_WRITE };

    class FFMpegIOContext {
    public:
    	FFMpegIOContext() = default;
        ~FFMpegIOContext() { avio_context_free(&io_context); }
    	/// Opens the file path with the specified open mode
        bool open(std::string path, OpenMode open_mode);
        
        /// Read size bytes from this IOContext. It returns the number of bytes actually read. It returns -1 when we've reached the end of the file
        uint64_t read(uint8_t* data, uint64_t size);
        
        uint64_t seek(int64_t offset, int whence);
        
        /// Write size bytes to this file path. Returns false if the file was not open in write mode or write error occurs
        bool write(uint8_t* data, uint64_t size);
        
        /// Returns the length of this file (in bytes). It returns -1 if the file is not valid, no open file, or an error occurs
        uint64_t length();
        
        /// Returns the path to the current file handled by this IO context. This might return an empty string if there is no file open.
        std::string get_path();
        
        /// Whether a file is currently open by this IO context
        bool is_open();
        
        bool is_readable();
        
        bool is_writable();
        
        /// Close this file
        void close();
    	
    	std::string get_error() { return error; }
    	
    	AVIOContext* get_context_internal() { return io_context; }
    
    private:
    	AVIOContext* io_context;
    	std::string error{};
    	std::string path;
    	
    };

    using FFMpegIOContext_Ptr = std::shared_ptr<FFMpegIOContext>;

}
