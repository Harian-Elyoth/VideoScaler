#ifndef VIDEOWRITER_HPP
#define VIDEOWRITER_HPP

#include <string>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#include "ScalerUtils.hpp"

class VideoWriter {
public:
    VideoWriter(const std::string& filepath, int width, int height, int fps);
    ~VideoWriter();

    bool open();
    void writeFrame(const Frame& frame);
    bool isOpened() const;

private:
    std::string filepath;
    int width, height, fps;
    bool opened;
    AVFormatContext* formatContext;
    AVCodecContext* codecContext;
    AVStream* stream;
    SwsContext* swsContext;
    int64_t frameCount;
};

#endif // VIDEOWRITER_HPP