#include "FFMpegIOContext.h"

namespace jp {

    /// Opens the file path with the specified open mode
    bool FFMpegIOContext::open(std::string path, OpenMode open_mode) {
    	int flag = open_mode == OpenMode::OPEN_MODE_READ ? AVIO_FLAG_READ : AVIO_FLAG_WRITE;
    	
    	int ret = avio_check(path.c_str(), AVIO_FLAG_READ_WRITE);
    	if (ret < 0) {
    	    char buffer[1024];
    	    snprintf(buffer, 2048, "Path doesn't exist: %s\n", path.c_str());
    	    error = std::string(buffer);
    	    return false;
    	}
    	
    	AVIOContext* context{nullptr};
    	if (avio_open(&context, path.c_str(), flag) < 0) {
    	    error = "Unable to open IO Context";
    	    return false;
    	};
    	if (!context) {
    	    error = "Unable to create IO Context";
    	    return false;
    	}
    
    	io_context = context;
    	
    	if (!io_context) {
    	    error = "FFMPEG_IO_CONTEXT: Unable to initialize IO Context";
    	    return false;
    	}
    	
    	return true;
    }
    
    /// Read size bytes from this IOContext. It returns the number of bytes actually read. It returns -1 when we've reached the end of the file
    uint64_t FFMpegIOContext::read(uint8_t* data, uint64_t size) {
        return avio_read(io_context, data, size);
    }
    
    /// Write size bytes to this file path. Returns false if the file was not open in write mode or write error occurs
    bool FFMpegIOContext::write(uint8_t* data, uint64_t size) {
        avio_write(io_context, data, size);
        return true;
    }
    
    /// Returns the length of this file (in bytes). It returns -1 if the file is not valid, no open file, or an error occurs
    uint64_t FFMpegIOContext::length() {
        // this is EXTREMELY WRONG!!!
        return avio_tell(io_context);
    }
    
    /// Returns the path to the current file handled by this IO context. This might return an empty string if there is no file open.
    std::string FFMpegIOContext::get_path() {
        return path;
    }
    
    /// Whether a file is currently open by this IO context
    bool FFMpegIOContext::is_open() {
        return true;
    }
    
    bool FFMpegIOContext::is_readable() {
        return true;
    }
    
    bool FFMpegIOContext::is_writable() {
        return true;
    }
    
    uint64_t FFMpegIOContext::seek(int64_t offset, int whence) {
        return avio_seek(io_context, offset, whence);
    }
    
    /// Close this file
    void FFMpegIOContext::close() {
        avio_close(io_context);
    }
    
    

}
