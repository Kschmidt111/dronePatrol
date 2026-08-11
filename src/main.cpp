#include <iostream>
#include <string>

// Phase 1+: OpenCV VideoCapture + person model (DNN / ONNX).
// Phase 2+: Discord webhook (libcurl) + cooldown / persistence.
// Phase 3+: Swap webcam/file source for RTMP from MediaMTX / DJI Fly.

int main(int argc, char* argv[]) {
    const std::string source = (argc > 1) ? argv[1] : "0";  // "0" = default webcam later

    std::cout << "dronePatrol — C++ person detect → Discord (scaffold)\n";
    std::cout << "Video source: " << source << "\n";
    std::cout << "Next: wire OpenCV capture + YOLO person class.\n";

    return 0;
}
