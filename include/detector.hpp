#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>

struct Detection {
    cv::Rect box;
    float confidence = 0.f;
    int classId = -1;
    std::string label;
};

// Loads YOLOv8 Open Images ONNX + class names. Filters to yard-relevant animals.
class AnimalDetector {
public:
    AnimalDetector(std::string modelPath, std::string namesPath);

    // const for callers: OpenCV's Net::forward is non-const, so net_ is mutable.
    [[nodiscard]] std::vector<Detection> detect(const cv::Mat& frame) const;
    void draw(cv::Mat& frame, const std::vector<Detection>& detections) const;

private:
    struct Letterbox {
        cv::Mat image;   // padded kInputW x kInputH BGR
        float gain = 1.f;
        float padX = 0.f;
        float padY = 0.f;
    };

    [[nodiscard]] Letterbox letterbox(const cv::Mat& frame) const;
    [[nodiscard]] cv::Rect mapBoxToFrame(
        float cx,
        float cy,
        float w,
        float h,
        const Letterbox& lb,
        int frameCols,
        int frameRows) const;

    // Mutable: cv::dnn::Net::setInput/forward are non-const APIs.
    mutable cv::dnn::Net net_;
    std::vector<std::string> classNames_;
    std::unordered_set<int> allowedClassIds_;

    static constexpr int kInputWidth = 640;
    static constexpr int kInputHeight = 640;
    static constexpr float kConfThreshold = 0.35f;
    static constexpr float kNmsThreshold = 0.45f;
};
