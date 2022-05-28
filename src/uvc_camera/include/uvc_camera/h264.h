#pragma once

#include <cstdint>
#include <functional>
namespace uvc_camera {

int InitEncoder(const int width, const int height, const int fps,
                std::function<void(uint8_t *, int)> encode_callback);

int Encode(uint8_t *rgb);

int EncodeFinish();

int SetEncodeCallback(std::function<void(uint8_t *, int)> callback);
}  // namespace uvc_camera