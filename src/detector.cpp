#include "detector.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include <opencv2/imgproc.hpp>

namespace {

[[nodiscard]] std::vector<std::string> loadClassNames(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not open class names file: " + path);
    }

    std::vector<std::string> names;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            names.push_back(line);
        }
    }
    return names;
}

}  // namespace

AnimalDetector::AnimalDetector(std::string modelPath, std::string namesPath)
    : classNames_(loadClassNames(namesPath)) {
    net_ = cv::dnn::readNetFromONNX(modelPath);
    if (net_.empty()) {
        throw std::runtime_error("Could not load ONNX model: " + modelPath);
    }
    net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

    // Open Images V7 indices we care about for yard wildlife / dog tests.
    const std::vector<std::string_view> allowedLabels = {
        "Animal", "Bear", "Brown bear", "Cat", "Cattle", "Deer", "Dog",
        "Fox", "Horse", "Raccoon", "Sheep",
    };

    for (int i = 0; i < static_cast<int>(classNames_.size()); ++i) {
        for (const auto label : allowedLabels) {
            if (classNames_[static_cast<std::size_t>(i)] == label) {
                allowedClassIds_.insert(i);
                break;
            }
        }
    }

    if (allowedClassIds_.empty()) {
        throw std::runtime_error("No allowed animal classes found in names file.");
    }

    std::cout << "Loaded model: " << modelPath << '\n';
    std::cout << "Loaded names: " << namesPath << " (" << classNames_.size() << " classes)\n";
    std::cout << "Tracking " << allowedClassIds_.size() << " animal labels.\n";
}

AnimalDetector::Letterbox AnimalDetector::letterbox(const cv::Mat& frame) const {
    Letterbox out;
    const float gain = std::min(
        static_cast<float>(kInputWidth) / static_cast<float>(frame.cols),
        static_cast<float>(kInputHeight) / static_cast<float>(frame.rows));

    const int resizedW = std::max(1, static_cast<int>(std::round(frame.cols * gain)));
    const int resizedH = std::max(1, static_cast<int>(std::round(frame.rows * gain)));
    const float padX = (static_cast<float>(kInputWidth) - static_cast<float>(resizedW)) * 0.5f;
    const float padY = (static_cast<float>(kInputHeight) - static_cast<float>(resizedH)) * 0.5f;

    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(resizedW, resizedH), 0, 0, cv::INTER_LINEAR);

    // Ultralytics-style gray pad (114) so aspect ratio is preserved without stretch.
    out.image = cv::Mat(kInputHeight, kInputWidth, frame.type(), cv::Scalar(114, 114, 114));
    const int left = static_cast<int>(std::round(padX - 0.1f));
    const int top = static_cast<int>(std::round(padY - 0.1f));
    resized.copyTo(out.image(cv::Rect(left, top, resizedW, resizedH)));

    out.gain = gain;
    out.padX = static_cast<float>(left);
    out.padY = static_cast<float>(top);
    return out;
}

cv::Rect AnimalDetector::mapBoxToFrame(
    float cx,
    float cy,
    float w,
    float h,
    const Letterbox& lb,
    int frameCols,
    int frameRows) const {
    // Model space (letterboxed 640) -> original frame pixels.
    const float x1 = (cx - w * 0.5f - lb.padX) / lb.gain;
    const float y1 = (cy - h * 0.5f - lb.padY) / lb.gain;
    const float x2 = (cx + w * 0.5f - lb.padX) / lb.gain;
    const float y2 = (cy + h * 0.5f - lb.padY) / lb.gain;

    const int left = std::clamp(static_cast<int>(std::round(x1)), 0, frameCols - 1);
    const int top = std::clamp(static_cast<int>(std::round(y1)), 0, frameRows - 1);
    const int right = std::clamp(static_cast<int>(std::round(x2)), 0, frameCols);
    const int bottom = std::clamp(static_cast<int>(std::round(y2)), 0, frameRows);

    return cv::Rect(left, top, std::max(0, right - left), std::max(0, bottom - top));
}

std::vector<Detection> AnimalDetector::detect(const cv::Mat& frame) const {
    std::vector<Detection> results;
    if (frame.empty()) {
        return results;
    }

    const Letterbox lb = letterbox(frame);

    cv::Mat blob;
    cv::dnn::blobFromImage(
        lb.image,
        blob,
        1.0 / 255.0,
        cv::Size(kInputWidth, kInputHeight),
        cv::Scalar(),
        true,
        false);

    net_.setInput(blob);

    std::vector<cv::Mat> outputs;
    net_.forward(outputs, net_.getUnconnectedOutLayersNames());
    if (outputs.empty()) {
        return results;
    }

    // YOLOv8 ONNX: [1, 4+numClasses, numProposals] -> rows = proposals.
    cv::Mat output = outputs[0];
    const int numAttrs = output.size[1];
    const int numProposals = output.size[2];
    cv::Mat pred(numAttrs, numProposals, CV_32F, output.ptr<float>());
    cv::Mat predictions;
    cv::transpose(pred, predictions);  // [numProposals, 4+numClasses]

    std::vector<int> classIds;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;

    for (int i = 0; i < predictions.rows; ++i) {
        const float* row = predictions.ptr<float>(i);
        const float* scores = row + 4;

        int bestClass = -1;
        float bestScore = 0.f;
        for (int c : allowedClassIds_) {
            if (c + 4 >= numAttrs) {
                continue;
            }
            const float score = scores[c];
            if (score > bestScore) {
                bestScore = score;
                bestClass = c;
            }
        }

        if (bestClass < 0 || bestScore < kConfThreshold) {
            continue;
        }

        const cv::Rect box = mapBoxToFrame(
            row[0],
            row[1],
            row[2],
            row[3],
            lb,
            frame.cols,
            frame.rows);
        if (box.width <= 0 || box.height <= 0) {
            continue;
        }

        classIds.push_back(bestClass);
        confidences.push_back(bestScore);
        boxes.push_back(box);
    }

    std::vector<int> keep;
    cv::dnn::NMSBoxes(boxes, confidences, kConfThreshold, kNmsThreshold, keep);

    results.reserve(keep.size());
    for (int idx : keep) {
        Detection det;
        det.box = boxes[static_cast<std::size_t>(idx)];
        det.confidence = confidences[static_cast<std::size_t>(idx)];
        det.classId = classIds[static_cast<std::size_t>(idx)];
        if (det.classId >= 0 && det.classId < static_cast<int>(classNames_.size())) {
            det.label = classNames_[static_cast<std::size_t>(det.classId)];
        } else {
            det.label = "unknown";
        }
        results.push_back(std::move(det));
    }

    return results;
}

void AnimalDetector::draw(cv::Mat& frame, const std::vector<Detection>& detections) const {
    for (const Detection& det : detections) {
        cv::rectangle(frame, det.box, cv::Scalar(0, 200, 0), 2);
        const std::string text =
            det.label + " " + std::to_string(static_cast<int>(det.confidence * 100.f)) + "%";
        const cv::Point org(det.box.x, std::max(20, det.box.y - 8));
        cv::putText(
            frame,
            text,
            org,
            cv::FONT_HERSHEY_SIMPLEX,
            0.6,
            cv::Scalar(0, 200, 0),
            2);
    }
}
