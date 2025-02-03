#include "VideoWriter.hpp"
#include <iostream>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

VideoWriter::VideoWriter(const std::string& filepath, int width, int height, int fps)
    : filepath(filepath), width(width), height(height), fps(fps), opened(false), frameCount(0)
{
}

VideoWriter::~VideoWriter()
{
    if (opened) {
        av_write_trailer(formatContext);
        if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&formatContext->pb);
        }
        avcodec_free_context(&codecContext);
        avformat_free_context(formatContext);
        sws_freeContext(swsContext);
    }
}

bool VideoWriter::open()
{
    avformat_alloc_output_context2(&formatContext, nullptr, nullptr, filepath.c_str());
    if (!formatContext) {
        std::cerr << "Error: Could not allocate output context\n";
        return false;
    }

    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "Error: Could not find H264 encoder\n";
        return false;
    }

    stream = avformat_new_stream(formatContext, codec);
    if (!stream) {
        std::cerr << "Error: Could not create new stream\n";
        return false;
    }

    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        std::cerr << "Error: Could not allocate codec context\n";
        return false;
    }

    codecContext->width = width;
    codecContext->height = height;
    codecContext->time_base = {1, fps};
    codecContext->framerate = {fps, 1};
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    codecContext->bit_rate = 2000000;  // 2 Mbps
    codecContext->gop_size = 12;

    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    AVDictionary *param = nullptr;
    av_dict_set(&param, "preset", "medium", 0);
    av_dict_set(&param, "tune", "zerolatency", 0);

    if (avcodec_open2(codecContext, codec, &param) < 0) {
        std::cerr << "Error: Could not open codec\n";
        return false;
    }

    if (avcodec_parameters_from_context(stream->codecpar, codecContext) < 0) {
        std::cerr << "Error: Could not copy codec parameters\n";
        return false;
    }

    av_dump_format(formatContext, 0, filepath.c_str(), 1);

    if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&formatContext->pb, filepath.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Error: Could not open output file\n";
            return false;
        }
    }

    if (avformat_write_header(formatContext, nullptr) < 0) {
        std::cerr << "Error: Could not write header\n";
        return false;
    }

    // Initialize the SwsContext for RGB to YUV conversion
    swsContext = sws_getContext(
        width, height, AV_PIX_FMT_RGB24,
        width, height, AV_PIX_FMT_YUV420P,
        SWS_BICUBIC, nullptr, nullptr, nullptr
    );

    if (!swsContext) {
        std::cerr << "Error: Could not initialize scaling context\n";
        return false;
    }

    opened = true;
    return true;
}

void VideoWriter::writeFrame(const Frame& frame)
{
    if (!opened) {
        return;
    }

    AVFrame* avFrame = av_frame_alloc();
    avFrame->format = codecContext->pix_fmt;
    avFrame->width = codecContext->width;
    avFrame->height = codecContext->height;
    av_frame_get_buffer(avFrame, 0);

    // Convert RGB to YUV420P directly from frame data
    const uint8_t* srcData[1] = { frame.getData() };
    int srcLinesize[1] = { frame.getStride() };

    // Convert RGB to YUV420P
    sws_scale(swsContext, srcData, srcLinesize, 0, height,
              avFrame->data, avFrame->linesize);

    // Set frame timing
    avFrame->pts = frameCount++;

    // Encode and write the frame
    if (avcodec_send_frame(codecContext, avFrame) < 0) {
        std::cerr << "Error: Could not send frame\n";
    }

    AVPacket* packet = av_packet_alloc();
    while (avcodec_receive_packet(codecContext, packet) == 0) {
        packet->stream_index = 0;
        av_packet_rescale_ts(packet, codecContext->time_base, stream->time_base);
        if (av_interleaved_write_frame(formatContext, packet) < 0) {
            std::cerr << "Error: Could not write frame\n";
        }
    }

    av_packet_free(&packet);
    av_frame_free(&avFrame);
}

bool VideoWriter::isOpened() const
{
    return opened;
}
