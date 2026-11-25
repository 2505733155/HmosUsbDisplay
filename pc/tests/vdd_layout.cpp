#include "../vdd_layout.h"

#include <cassert>

static void ExpectPosition(POINTL position, LONG x, LONG y)
{
    assert(position.x == x && position.y == y);
}

int main()
{
    VddDesktopLayout layout{
        RECT{-5760, 0, -3840, 1080},
        {{0, 0, 2560, 1600}, {-3840, 0, 0, 2160}},
        "two physical screens"};
    const VddSavedPosition below{{-2978, 2160}, "two physical screens"};

    ExpectPosition(VddChoosePosition(layout, 1920, 1080, below, false), -2978, 2160);
    ExpectPosition(VddChoosePosition(layout, 1920, 1080, std::nullopt, false), -5760, 0);
    ExpectPosition(VddChoosePosition(layout, 1920, 1080, below, true, true), 2560, 0);

    layout.key = "different physical screens";
    ExpectPosition(VddChoosePosition(layout, 1920, 1080, below, false), -5760, 0);
    layout.key.clear();
    const VddSavedPosition unidentified{{-2978, 2160}, ""};
    ExpectPosition(VddChoosePosition(layout, 1920, 1080, unidentified, false), -5760, 0);

    layout.key = below.key;
    const VddSavedPosition overlap{{-2978, 2100}, below.key};
    const VddSavedPosition cornerOnly{{0, 2160}, below.key};
    const VddSavedPosition disconnected{{-2978, 2300}, below.key};
    for (const auto &invalid : {overlap, cornerOnly, disconnected}) {
        ExpectPosition(VddChoosePosition(layout, 1920, 1080, invalid, false), -5760, 0);
    }

    // Growing at the current origin would overlap the left monitor. Use the new width.
    ExpectPosition(VddChoosePosition(layout, 2560, 1440, std::nullopt, false), -6400, 0);
    ExpectPosition(VddChoosePosition(layout, 2560, 1440, below, false), -2978, 2160);

    layout.tablet = RECT{-100, 100, 1820, 1180};
    layout.others[0] = RECT{0, 1300, 2560, 2900};
    ExpectPosition(VddChoosePosition(layout, 1920, 1080, std::nullopt, true), 2560, 1300);
}
