#pragma once

#include <core/engine/fadingdefs.h>

#include <array>

namespace Fooyin::PlaybackSettings {
constexpr auto MinBufferSize = 500;

[[nodiscard]] inline int minimumBufferLengthForFades(bool fadingEnabled, const Engine::FadingValues& fadingValues,
                                                     bool crossfadingEnabled,
                                                     const Engine::CrossfadingValues& crossfadingValues)
{
    const auto enabledValue = [](bool groupEnabled, const Engine::FadeSpec& spec, auto member) {
        return (groupEnabled && spec.enabled) ? spec.*member : 0;
    };

    const std::array values{
        MinBufferSize,
        enabledValue(fadingEnabled, fadingValues.pause, &Engine::FadeSpec::in),
        enabledValue(fadingEnabled, fadingValues.pause, &Engine::FadeSpec::out),
        enabledValue(fadingEnabled, fadingValues.stop, &Engine::FadeSpec::in),
        enabledValue(fadingEnabled, fadingValues.stop, &Engine::FadeSpec::out),
        enabledValue(fadingEnabled, fadingValues.boundary, &Engine::FadeSpec::in),
        enabledValue(fadingEnabled, fadingValues.boundary, &Engine::FadeSpec::out),
        enabledValue(crossfadingEnabled, crossfadingValues.manualChange, &Engine::FadeSpec::in),
        enabledValue(crossfadingEnabled, crossfadingValues.manualChange, &Engine::FadeSpec::out),
        enabledValue(crossfadingEnabled, crossfadingValues.autoChange, &Engine::FadeSpec::in),
        enabledValue(crossfadingEnabled, crossfadingValues.autoChange, &Engine::FadeSpec::out),
        enabledValue(crossfadingEnabled, crossfadingValues.seek, &Engine::FadeSpec::in),
        enabledValue(crossfadingEnabled, crossfadingValues.seek, &Engine::FadeSpec::out),
    };

    return std::ranges::max(values);
}
} // namespace Fooyin::PlaybackSettings
