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

    [[nodiscard]] std::vector<Detection> detect(const cv::Mat& frame) const;
    void draw(cv::Mat& frame, const std::vector<Detection>& detections) const;

private:
    cv::dnn::Net net_;
    std::vector<std::string> classNames_;
    std::unordered_set<int> allowedClassIds_;

    static constexpr int kInputWidth = 640;
    static constexpr int kInputHeight = 640;
    static constexpr float kConfThreshold = 0.35f;
    static constexpr float kNmsThreshold = 0.45f;
};
