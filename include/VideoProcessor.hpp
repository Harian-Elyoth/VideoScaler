#ifndef VIDEOPROCESSOR_HPP
#define VIDEOPROCESSOR_HPP

#include <string>


class VideoProcessor {
public:
    VideoProcessor(const std::string& inputPath, const std::string& outputPath, double scaleFactor);
    void process();

private:
    std::string inputPath;
    std::string outputPath;
    double scaleFactor;
};

#endif // VIDEOPROCESSOR_HPP