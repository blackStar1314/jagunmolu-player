# JAGUNMOLU PLAYER
This is a C++ library with audio and video processing capabilities. It 
provides the ability to decode audio and video files, add filters, audio and video playback and synchronization, subtitle display (and more in the works!). It also supports a lot of audio, video and subtitle formats (typically as much as FFMpeg supports).

## Features
- [x] Audio and video demuxing
- [x] Audio and Video decoding
- [x] Subtitle Rendering
- [x] Display multiple subtitles at the same time (not finished)
- [x] Video and Audio Filters
- [x] Works on Desktop and Android
- [ ] Video and Audio Encoding
- [ ] Hardware-accelerated decoding

## Compiling
CMake is required to compile this library. From the root of the project directory, run
```bash
mkdir build && cd build && cmake .. && make
```
You can also take a look at the CMakeLists.txt file and make changes before compiling the code.
Note that this requires the FFMpeg libraries to be installed in your system. Windows users will probably need to modify the CMakeLists.txt file to point CMake to the appropriate location of the libraries.

The test executable also requires SDL2. You can remove the entry if you do not want to compile the test executable

## License
This project does not have any license. I guess they call it Public Domain or something, I don't care. You can do whatever you want with it as long as you don't hold me responsible if/when your device gets fried as a result of using this library.
