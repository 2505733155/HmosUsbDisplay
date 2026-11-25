#pragma once

#include <windows.h>

#include <climits>
#include <optional>
#include <string>
#include <vector>

struct VddSavedPosition
{
    POINTL position;
    std::string key;
};

struct VddDesktopLayout
{
    std::optional<RECT> tablet;
    std::vector<RECT> others;
    std::string key;
};

inline bool VddPositionFits(POINTL position, LONG width, LONG height, const std::vector<RECT> &others)
{
    const auto right = static_cast<long long>(position.x) + width;
    const auto bottom = static_cast<long long>(position.y) + height;
    if (width <= 0 || height <= 0 || right > LONG_MAX || bottom > LONG_MAX) {
        return false;
    }
    bool adjacent = others.empty();
    for (const auto &screen : others) {
        const bool across = position.x < screen.right && right > screen.left;
        const bool along = position.y < screen.bottom && bottom > screen.top;
        if (across && along) {
            return false;
        }
        adjacent = adjacent || (across && (position.y == screen.bottom || bottom == screen.top)) ||
                   (along && (position.x == screen.right || right == screen.left));
    }
    return adjacent;
}

inline POINTL VddChoosePosition(const VddDesktopLayout &layout, LONG width, LONG height,
                                const std::optional<VddSavedPosition> &saved, bool right, bool forceSide = false)
{
    if (!forceSide && saved && !layout.key.empty() && saved->key == layout.key &&
        VddPositionFits(saved->position, width, height, layout.others)) {
        return saved->position;
    }
    if (!forceSide && layout.tablet) {
        const POINTL current{layout.tablet->left, layout.tablet->top};
        if (VddPositionFits(current, width, height, layout.others)) {
            return current;
        }
    }

    if (layout.others.empty()) {
        return POINTL{0, 0};
    }
    const RECT *edge = &layout.others.front();
    for (const auto &screen : layout.others) {
        if (right ? screen.right > edge->right : screen.left < edge->left) {
            edge = &screen;
        }
    }
    const LONG x = right ? edge->right : edge->left - width;
    const POINTL aligned{x, 0};
    return VddPositionFits(aligned, width, height, layout.others) ? aligned : POINTL{x, edge->top};
}
