#include "VideoProcessor.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input_video> <output_video> <scale_factor>\n";
        return -1;
    }

    std::string inputVideo = argv[1];
    std::string outputVideo = argv[2];
    double scaleFactor = std::stod(argv[3]);

    VideoProcessor processor(inputVideo, outputVideo, scaleFactor);
    processor.process();

    return 0;
}