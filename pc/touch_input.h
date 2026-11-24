#pragma once

#include <cstdint>

// Version 1 sends one action byte and two big-endian normalized coordinates.
enum class TouchAction : uint8_t { Down, Move, Up, Cancel };

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
        }
        return false;
    }

private:
    bool down_ = false;
};
