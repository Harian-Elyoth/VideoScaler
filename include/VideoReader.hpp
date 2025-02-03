#ifndef VIDEOREADER_HPP
#define VIDEOREADER_HPP

#include <string>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#include "ScalerUtils.hpp"

class VideoReader {
public:
    VideoReader(const std::string& filepath);
    ~VideoReader();

    bool open();
    Frame readFrame();
    int getWidth() const;
    int getHeight() const;
    int getFPS() const;
    bool isOpened() const;

private:
    std::string filepath;
    int width, height, fps;
    bool opened;
    AVFormatContext* formatContext;
    AVCodecContext* codecContext;
    SwsContext* swsContext;
    int streamIndex;
};

#endif // VIDEOREADER_HPP