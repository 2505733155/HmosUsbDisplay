#include "../touch_input.h"

#include <cassert>

int main()
{
    constexpr uint8_t legacyDown[]{0, 0x12, 0x34, 0xab, 0xcd};
    constexpr uint8_t twoFingers[]{0x80, 1, 0x12, 0x34, 0xab, 0xcd, 0, 0, 0, 0xff, 0xff};
    constexpr uint8_t liftFirst[]{0x80, 2, 0, 0, 0, 0, 1, 0, 1, 0xff, 0xfe};
    constexpr uint8_t malformed[]{0x80, 0xff, 0, 0, 0, 0, 4, 0, 0, 0, 0};
    constexpr uint8_t bothIdle[]{0x80, 0xff, 0, 0, 0, 0, 0xff, 0, 0, 0, 0};
    constexpr uint8_t cancelSecond[]{0x80, 0xff, 0, 0, 0, 0, 3, 0, 0, 0, 0};

    TouchFrame frame{};
    TouchFrameSequence sequence;
    assert(TouchPacketSize(legacyDown[0]) == sizeof(legacyDown));
    assert(TouchPacketSize(twoFingers[0]) == sizeof(twoFingers));
    assert(TouchPacketSize(0x81) == 0);
    assert(DecodeTouchFrame(legacyDown, sizeof(legacyDown), frame));
    assert(frame[0].x == 0x1234 && frame[0].y == 0xabcd && frame[1].action == TouchAction::Idle);
    assert(sequence.Accept(frame));
    assert(DecodeTouchFrame(liftFirst, sizeof(liftFirst), frame));
    assert(!sequence.Accept(frame));
    assert(!DecodeTouchFrame(twoFingers, sizeof(twoFingers) - 1, frame));
    assert(!DecodeTouchFrame(malformed, sizeof(malformed), frame));
    assert(DecodeTouchFrame(twoFingers, sizeof(twoFingers), frame));
    assert(frame[0].action == TouchAction::Move && frame[1].action == TouchAction::Down);
    const auto encoded = EncodeTouchFrame(frame);
    for (size_t i = 0; i < sizeof(twoFingers); ++i) assert(encoded[i] == twoFingers[i]);
    assert(sequence.Accept(frame));
    assert(DecodeTouchFrame(bothIdle, sizeof(bothIdle), frame));
    assert(!sequence.Accept(frame));
    assert(DecodeTouchFrame(liftFirst, sizeof(liftFirst), frame));
    assert(sequence.Accept(frame));
    assert(DecodeTouchFrame(cancelSecond, sizeof(cancelSecond), frame));
    assert(sequence.Accept(frame));
    assert(!sequence.Accept(frame));
    assert(DecodeTouchFrame(legacyDown, sizeof(legacyDown), frame));
    assert(sequence.Accept(frame));
}
