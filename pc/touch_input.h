#pragma once

#include "../harmony/entry/src/main/cpp/touch_protocol.h"

inline int TouchPixel(uint16_t value, int origin, int length)
{
    return origin + static_cast<int>(static_cast<uint64_t>(value) * (length - 1) / 65535);
}

class TouchSequence {
public:
    bool Accept(TouchAction action)
    {
        switch (action) {
        case TouchAction::Down:
            if (down_) return false;
            down_ = true;
            return true;
        case TouchAction::Move:
            return down_;
        case TouchAction::Up:
        case TouchAction::Cancel:
            if (!down_) return false;
            down_ = false;
            return true;
        case TouchAction::Idle:
            return !down_;
        }
        return false;
    }

private:
    bool down_ = false;
};

class TouchFrameSequence {
public:
    bool Accept(const TouchFrame &frame)
    {
        auto next = slots_;
        for (size_t slot = 0; slot < frame.size(); ++slot) {
            if (!next[slot].Accept(frame[slot].action)) return false;
        }
        slots_ = next;
        return true;
    }

private:
    std::array<TouchSequence, TouchFrame{}.size()> slots_{};
};
