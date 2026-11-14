#include "../../include/AudioManager.h"

// Used by the tests, which are built without an audio library.
std::unique_ptr<AudioBackend> createDefaultAudioBackend() {
    return nullptr;
}
