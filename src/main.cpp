#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include "alert.hpp"
#include "capture.hpp"
#include "detector.hpp"

namespace {

constexpr std::string_view kWindowName = "dronePatrol viewer";
constexpr std::string_view kDefaultSource = "rtsp://127.0.0.1:8554/live";
constexpr std::string_view kModelFile = "models/yolov8n-oiv7.onnx";
constexpr std::string_view kNamesFile = "models/open-images-v7.names";

[[nodiscard]] std::string resolveSource(int argc, char* argv[]) {
    if (argc > 1 && argv[1] != nullptr && argv[1][0] != '\0') {
        return std::string{argv[1]};
    }
    return std::string{kDefaultSource};
}

[[nodiscard]] std::optional<std::filesystem::path> findExisting(std::string_view relative) {
    namespace fs = std::filesystem;
    const fs::path rel{relative};
    const std::array candidates = {
        rel,
        fs::path{".."} / rel,
        fs::path{"../.."} / rel,
    };
    for (const fs::path& candidate : candidates) {
        if (fs::exists(candidate)) {
            return fs::weakly_canonical(candidate);
        }
    }
    return std::nullopt;
}

void printUsage(std::string_view exeName) {
    std::cout
        << "Usage:\n"
        << "  " << exeName << " [source]\n\n"
        << "Examples:\n"
        << "  " << exeName << " 0\n"
        << "      webcam (good for dog tests)\n"
        << "  " << exeName << "\n"
        << "      MediaMTX live RTSP\n"
        << "  " << exeName << " C:\\\\path\\\\to\\\\video.mp4\n\n"
        << "Keys: q / Esc = quit\n"
        << "Run from the project folder so models\\\\ can be found.\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::string source = resolveSource(argc, argv);
    printUsage(argc > 0 && argv[0] ? argv[0] : "dronePatrol");

    const auto modelPath = findExisting(kModelFile);
    const auto namesPath = findExisting(kNamesFile);
    if (!modelPath || !namesPath) {
        std::cerr
            << "Missing model files. Expected under models\\:\n"
            << "  yolov8n-oiv7.onnx\n"
            << "  open-images-v7.names\n"
            << "Run the exe from the project root folder.\n";
        return EXIT_FAILURE;
    }

    std::unique_ptr<AnimalDetector> detector;
    try {
        detector = std::make_unique<AnimalDetector>(modelPath->string(), namesPath->string());
    } catch (const std::exception& ex) {
        std::cerr << "Detector init failed: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "Opening source: " << source << '\n';
    FrameSource frames(source);
    if (!frames.isOpen()) {
        std::cerr << "Failed to open video source.\n";
        return EXIT_FAILURE;
    }

    DiscordAlert alerts;
    if (alerts.ready()) {
        if (!alerts.send("dronePatrol online (test message)")) {
            std::cerr << "Discord test send failed or cooldown blocked it.\n";
        }
    } else {
        std::cerr << "Discord alerts disabled (check .env).\n";
    }

    cv::namedWindow(std::string{kWindowName}, cv::WINDOW_NORMAL);

    cv::Mat frame;
    while (true) {
        if (!frames.read(frame)) {
            std::cerr << "Frame grab failed or stream ended.\n";
            break;
        }

        const std::vector<Detection> detections = detector->detect(frame);
        detector->draw(frame, detections);

        if (!detections.empty()) {
            std::ostringstream alertText;
            alertText << "Detected: ";
            bool first = true;
            for (const Detection& det : detections) {
                if (!first) {
                    alertText << ", ";
                }
                first = false;
                alertText << det.label << " ("
                          << static_cast<int>(det.confidence * 100.f) << "%)";
            }
            (void)alerts.send(alertText.str());
        }

        for (const Detection& det : detections) {
            std::cout << det.label << " " << det.confidence << '\n';
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
