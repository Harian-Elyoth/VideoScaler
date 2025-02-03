#include "VideoReader.hpp"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#include <iostream>

VideoReader::VideoReader(const std::string& filepath)
    : filepath(filepath), width(0), height(0), fps(0), opened(false), swsContext(nullptr)
{
    formatContext = avformat_alloc_context();
}

VideoReader::~VideoReader()
{
    if (opened) {
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        avformat_free_context(formatContext);
        if (swsContext) {
            sws_freeContext(swsContext);
        }
    }
}

bool VideoReader::open()
{
    if (avformat_open_input(&formatContext, filepath.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Error: Could not open input file\n";
        return false;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        std::cerr << "Error: Could not find stream info\n";
        return false;
    }

    AVCodec* codec = nullptr;
    streamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (streamIndex < 0) {
        std::cerr << "Error: Could not find video stream\n";
        return false;
    }

    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        std::cerr << "Error: Could not allocate codec context\n";
        return false;
    }

    if (avcodec_parameters_to_context(codecContext, formatContext->streams[streamIndex]->codecpar) < 0) {
        std::cerr << "Error: Could not copy codec parameters\n";
        return false;
    }

    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        std::cerr << "Error: Could not open codec\n";
        return false;
    }

    width = codecContext->width;
    height = codecContext->height;
    fps = av_q2d(formatContext->streams[streamIndex]->avg_frame_rate);

    // Initialize scaling context for conversion to RGB24
    swsContext = sws_getContext(
        width, height, codecContext->pix_fmt,
        width, height, AV_PIX_FMT_RGB24,
        SWS_BICUBIC, nullptr, nullptr, nullptr
    );

    if (!swsContext) {
        std::cerr << "Error: Could not initialize scaling context\n";
        return false;
    }

    opened = true;
    return true;
}

Frame VideoReader::readFrame()
{
    if (!opened) {
        return Frame();
    }

    AVPacket* packet = av_packet_alloc();
    Frame frame;
    
    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == streamIndex) {
            AVFrame* avFrame = av_frame_alloc();
            int response = avcodec_send_packet(codecContext, packet);
            
            if (response >= 0) {
                response = avcodec_receive_frame(codecContext, avFrame);
                if (response >= 0) {
                    // Create frame with correct dimensions
                    frame = Frame(avFrame->height, avFrame->width);

                    // Convert directly to frame's data buffer
                    uint8_t* dstData[1] = { frame.getData() };
                    int dstLinesize[1] = { frame.getStride() };

                    // Convert to RGB
                    sws_scale(swsContext,
                        avFrame->data, avFrame->linesize, 0, height,
                        dstData, dstLinesize);

                    av_frame_free(&avFrame);
                    av_packet_unref(packet);
                    av_packet_free(&packet);
                    return frame;
                }
            }
            av_frame_free(&avFrame);
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    return frame;
}

int VideoReader::getWidth() const
{
    return width;
}

int VideoReader::getHeight() const
{
    return height;
}

int VideoReader::getFPS() const
{
    return fps;
}

bool VideoReader::isOpened() const
{
    return opened;
}

