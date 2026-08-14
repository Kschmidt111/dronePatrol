#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

// Owns how frames arrive: webcam index, file path, or RTSP/RTMP URL.
class FrameSource {
public:
    explicit FrameSource(std::string source);

    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] bool read(cv::Mat& frame);
    [[nodiscard]] const std::string& source() const;

private:
    std::string source_;
    std::unique_ptr<cv::VideoCapture> capture_;
};
