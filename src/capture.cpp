#include "capture.hpp"

FrameSource::FrameSource(std::string source) : source_(std::move(source)) {
    capture_ = std::make_unique<cv::VideoCapture>();

    if (source_.size() == 1 && source_[0] >= '0' && source_[0] <= '9') {
        const int index = source_[0] - '0';
        if (!capture_->open(index, cv::CAP_MSMF) && !capture_->open(index, cv::CAP_DSHOW)) {
            capture_->open(index);
        }
    } else {
        if (!capture_->open(source_, cv::CAP_FFMPEG)) {
            capture_->open(source_);
        }
    }

    if (!capture_->isOpened()) {
        capture_.reset();
    }
}

bool FrameSource::isOpen() const {
    return capture_ != nullptr && capture_->isOpened();
}

bool FrameSource::read(cv::Mat& frame) {
    if (!isOpen()) {
        return false;
    }
    return capture_->read(frame) && !frame.empty();
}

const std::string& FrameSource::source() const {
    return source_;
}
