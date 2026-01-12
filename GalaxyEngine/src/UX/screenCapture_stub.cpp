#include "UX/screenCapture.h"

#include "parameters.h"

#include <raylib.h>

bool ScreenCapture::screenGrab(RenderTexture2D& /*myParticlesTexture*/, UpdateVariables& /*myVar*/,
                               UpdateParameters& /*myParam*/) {
    return false;
}

void ScreenCapture::cleanupFFmpeg() {}

void ScreenCapture::exportFrameToFile(const Image& /*frame*/, const std::string& /*videoFolder*/,
                                      const std::string& /*videoName*/, int /*frameNumber*/) {}

void ScreenCapture::exportMemoryFramesToDisk() {}

void ScreenCapture::discardMemoryFrames() {}

void ScreenCapture::createFramesFolder(const std::string& /*folderPath*/) {}

void ScreenCapture::discardRecording() {}
