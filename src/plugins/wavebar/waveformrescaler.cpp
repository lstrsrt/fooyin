/*
 * Fooyin
 * Copyright © 2024, Luke Taylor <LukeT1@proton.me>
 *
 * Fooyin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Fooyin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fooyin.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "waveformrescaler.h"

#include <utils/settings/settingsmanager.h>

namespace {
double buildSample(Fooyin::WaveBar::WaveformSample& sample, const Fooyin::WaveBar::WaveformData<float>& data,
                   int channel, double start, double end)
{
    double sampleWeight{0.0};

    if(channel < 0 || channel >= static_cast<int>(data.channelData.size())) {
        return sampleWeight;
    }

    const auto& [inMax, inMin, inRms] = data.channelData[static_cast<size_t>(channel)];
    const auto availableSamples       = std::min({inMax.size(), inMin.size(), inRms.size()});
    if(availableSamples == 0) {
        return sampleWeight;
    }

    const int firstIndex = std::max(0, static_cast<int>(std::floor(start)));
    const int lastIndex  = std::min(static_cast<int>(availableSamples), static_cast<int>(std::ceil(end)));

    for(int index{firstIndex}; index < lastIndex; ++index) {
        const double sampleStart = index;
        const double sampleEnd   = sampleStart + 1.0;
        const double overlap     = std::min(end, sampleEnd) - std::max(start, sampleStart);
        if(overlap <= 0.0) {
            continue;
        }

        const auto sampleIndex = static_cast<size_t>(index);
        const float sampleMax  = inMax[sampleIndex];
        const float sampleMin  = inMin[sampleIndex];
        const float sampleRms  = inRms[sampleIndex];

        sample.max = std::max(sample.max, sampleMax);
        sample.min = std::min(sample.min, sampleMin);
        sample.rms += sampleRms * sampleRms * static_cast<float>(overlap);

        sampleWeight += overlap;
    }

    return sampleWeight;
}
} // namespace

namespace Fooyin::WaveBar {
WaveformRescaler::WaveformRescaler(QObject* parent)
    : Worker{parent}
    , m_width{0}
    , m_sampleWidth{1}
    , m_supersampleFactor{1}
    , m_downMix{DownmixOption::Off}
{ }

void WaveformRescaler::rescale()
{
    if(m_width == 0) {
        return;
    }

    setState(Running);

    WaveformData<float> data{m_data};
    data.channelData.clear();

    if(m_downMix == DownmixOption::Stereo && data.channels > 2) {
        data.channels = 2;
    }
    else if(m_downMix == DownmixOption::Mono) {
        data.channels = 1;
    }

    data.channelData.resize(data.channels);

    const double sampleSpan       = m_data.complete ? m_data.sampleCount() : m_data.samplesPerChannel;
    const int outputSlotCount     = std::max(1, 1 + (std::max(0, m_width - 1) / m_sampleWidth));
    const int outputSampleCount   = outputSlotCount * std::max(1, m_supersampleFactor);
    const double samplesPerOutput = sampleSpan / outputSampleCount;

    for(int ch{0}; ch < data.channels; ++ch) {
        if(ch < 0 || ch >= static_cast<int>(data.channelData.size())) {
            continue;
        }

        auto& [outMax, outMin, outRms] = data.channelData[static_cast<size_t>(ch)];

        double start{0.0};

        for(int x{0}; x < outputSampleCount; ++x) {
            if(!mayRun()) {
                return;
            }

            const double end = (x + 1) * samplesPerOutput;

            double sampleWeight{0.0};
            WaveformSample sample;

            if(m_downMix == DownmixOption::Mono || (m_downMix == DownmixOption::Stereo && m_data.channels > 2)) {
                for(int mixCh{0}; mixCh < m_data.channels; ++mixCh) {
                    sampleWeight += buildSample(sample, m_data, mixCh, start, end);
                }
            }
            else {
                sampleWeight += buildSample(sample, m_data, ch, start, end);
            }

            if(sampleWeight > 0.0) {
                sample.rms /= static_cast<float>(sampleWeight);
                sample.rms = std::sqrt(sample.rms);

                outMax.emplace_back(sample.max);
                outMin.emplace_back(sample.min);
                outRms.emplace_back(sample.rms);
            }

            start = end;
        }
    }

    setState(Idle);
    emit waveformRescaled(data);
}

void WaveformRescaler::rescale(int width)
{
    m_width = width;

    if(m_data.empty()) {
        return;
    }

    rescale();
}

void WaveformRescaler::rescale(const WaveformData<float>& data, int width)
{
    if(std::exchange(m_data, data) != data) {
        rescale(width);
    }
}

void WaveformRescaler::changeSampleWidth(int width)
{
    if(std::exchange(m_sampleWidth, width) != width) {
        rescale(m_width);
    }
}

void WaveformRescaler::changeDownmix(DownmixOption option)
{
    if(std::exchange(m_downMix, option) != option) {
        rescale(m_width);
    }
}

void WaveformRescaler::changeSupersampleFactor(int factor)
{
    factor = std::max(1, factor);
    if(std::exchange(m_supersampleFactor, factor) != factor) {
        rescale(m_width);
    }
}
} // namespace Fooyin::WaveBar
