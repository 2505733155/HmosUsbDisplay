#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

enum class TouchAction : uint8_t { Down, Move, Up, Cancel, Idle = 0xff };

struct TouchSample {
    TouchAction action = TouchAction::Idle;
    uint16_t x = 0;
    uint16_t y = 0;
};

// Legacy packets carry one action and two big-endian coordinates. Framed
// packets add a marker and carry both slots, including any stationary contact.
constexpr size_t TOUCH_SAMPLE_BYTES = 5;
constexpr uint8_t TOUCH_FRAME_MARKER = 0x80;
using TouchFrame = std::array<TouchSample, 2>;
using TouchPacket = std::array<uint8_t, 1 + TouchFrame{}.size() * TOUCH_SAMPLE_BYTES>;

inline size_t TouchPacketSize(uint8_t first)
{
    if (first == TOUCH_FRAME_MARKER) return TouchPacket{}.size();
    if (first <= static_cast<uint8_t>(TouchAction::Cancel)) return TOUCH_SAMPLE_BYTES;
    return 0;
}

inline void EncodeTouchSample(const TouchSample &sample, uint8_t *out)
{
    out[0] = static_cast<uint8_t>(sample.action);
    out[1] = static_cast<uint8_t>(sample.x >> 8);
    out[2] = static_cast<uint8_t>(sample.x);
    out[3] = static_cast<uint8_t>(sample.y >> 8);
    out[4] = static_cast<uint8_t>(sample.y);
}

inline TouchPacket EncodeTouchFrame(const TouchFrame &frame)
{
    TouchPacket packet{TOUCH_FRAME_MARKER};
    for (size_t slot = 0; slot < frame.size(); ++slot) {
        EncodeTouchSample(frame[slot], packet.data() + 1 + slot * TOUCH_SAMPLE_BYTES);
    }
    return packet;
}

inline bool DecodeTouchFrame(const uint8_t *packet, size_t size, TouchFrame &frame)
{
    if (size == 0 || size != TouchPacketSize(packet[0])) return false;
    frame = {};
    const bool framed = packet[0] == TOUCH_FRAME_MARKER;
    const size_t count = framed ? frame.size() : 1;
    const uint8_t *sample = packet + (framed ? 1 : 0);
    for (size_t slot = 0; slot < count; ++slot, sample += TOUCH_SAMPLE_BYTES) {
        const auto action = static_cast<TouchAction>(sample[0]);
        if (action > TouchAction::Cancel && action != TouchAction::Idle) return false;
        frame[slot] = {action, static_cast<uint16_t>((sample[1] << 8) | sample[2]),
                       static_cast<uint16_t>((sample[3] << 8) | sample[4])};
    }
    return true;
}
