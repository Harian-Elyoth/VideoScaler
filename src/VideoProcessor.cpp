#include <iostream>

#include "VideoProcessor.hpp"
#include "VideoWriter.hpp"
#include "FrameScaler.hpp"
#include "VideoReader.hpp"

VideoProcessor::VideoProcessor(const std::string& inputPath, const std::string& outputPath, double scaleFactor)
    : inputPath(inputPath), outputPath(outputPath), scaleFactor(scaleFactor)
{
}

void VideoProcessor::process()
{
    VideoReader reader(inputPath);
    if (!reader.open()) {
        std::cerr << "Error: Could not open input video file\n";
        return;
    }

    int width = reader.getWidth();
    int height = reader.getHeight();
    int newWidth = width * scaleFactor;
    int newHeight = height * scaleFactor;

    std::cout << "Input video resolution: " << width << "x" << height << "\n";
    std::cout << "Output video resolution: " << newWidth << "x" << newHeight << "\n";

    VideoWriter writer(outputPath, newWidth, newHeight, reader.getFPS());
    if (!writer.open()) {
        std::cerr << "Error: Could not open output video file\n";
        return;
    }

    FrameScaler scaler;
    while (reader.isOpened()) {
        Frame frame = reader.readFrame();
        if (frame.empty()) {
            break;
        }
        Frame scaledFrame = scaler.resize(frame, newWidth, newHeight);
        writer.writeFrame(scaledFrame);
    }

    std::cout << "Video processing completed\n";
}