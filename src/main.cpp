#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

namespace {

constexpr std::string_view kWindowName = "dronePatrol viewer";
constexpr std::string_view kDefaultSource = "rtsp://127.0.0.1:8554/live";

[[nodiscard]] std::string resolveSource(int argc, char* argv[]) {
    if (argc > 1 && argv[1] != nullptr && argv[1][0] != '\0') {
        return std::string{argv[1]};
    }
    return std::string{kDefaultSource};
}

// OpenCV uses camera index for "0", "1", ... otherwise treats the string as a URL/path.
[[nodiscard]] std::unique_ptr<cv::VideoCapture> openCapture(std::string_view source) {
    auto capture = std::make_unique<cv::VideoCapture>();

    if (source.size() == 1 && source[0] >= '0' && source[0] <= '9') {
        const int index = source[0] - '0';
        if (!capture->open(index, cv::CAP_DSHOW)) {
            capture->open(index);
        }
    } else {
        // FFmpeg backend is what vcpkg OpenCV uses for RTSP/RTMP/files.
        if (!capture->open(std::string{source}, cv::CAP_FFMPEG)) {
            capture->open(std::string{source});
        }
    }

    if (!capture->isOpened()) {
        return nullptr;
    }
    return capture;
}

void printUsage(std::string_view exeName) {
    std::cout
        << "Usage:\n"
        << "  " << exeName << " [source]\n\n"
        << "Examples:\n"
        << "  " << exeName << "\n"
        << "      (default) MediaMTX live path via RTSP\n"
        << "  " << exeName << " rtsp://127.0.0.1:8554/live\n"
        << "  " << exeName << " 0\n"
        << "      webcam index 0\n"
        << "  " << exeName << " C:\\\\path\\\\to\\\\video.mp4\n\n"
        << "Keys: q / Esc = quit\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::string source = resolveSource(argc, argv);
    printUsage(argc > 0 && argv[0] ? argv[0] : "dronePatrol");

    std::cout << "Opening source: " << source << '\n';
    std::cout << "Tip: keep MediaMTX running and DJI Fly streaming to path 'live'.\n";

    const std::unique_ptr<cv::VideoCapture> capture = openCapture(source);
    if (!capture) {
        std::cerr << "Failed to open video source.\n"
                  << "If this is the drone stream: start MediaMTX, start DJI RTMP, then retry.\n";
        return EXIT_FAILURE;
    }

    cv::namedWindow(std::string{kWindowName}, cv::WINDOW_NORMAL);

    cv::Mat frame;
    while (true) {
        if (!capture->read(frame) || frame.empty()) {
            std::cerr << "Frame grab failed or stream ended.\n";
            break;
        }

        cv::imshow(std::string{kWindowName}, frame);
        const int key = cv::waitKey(1);
        if (key == 'q' || key == 'Q' || key == 27) {
            break;
        }
    }

    cv::destroyAllWindows();
    std::cout << "Viewer closed.\n";
    return EXIT_SUCCESS;
}
