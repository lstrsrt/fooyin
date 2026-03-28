/*
 * Fooyin
 * Copyright © 2026, Luke Taylor <luket@pm.me>
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

#include "core/engine/output/outputfader.h"

#include <core/engine/audioformat.h>
#include <core/engine/dsp/processingbufferlist.h>

#include <gtest/gtest.h>

#include <utility>
#include <vector>

constexpr auto SampleRate = 1000;

namespace {
Fooyin::ProcessingBufferList makeChunks(const std::vector<double>& samples)
{
    Fooyin::ProcessingBuffer buffer{Fooyin::AudioFormat{Fooyin::SampleFormat::F64, SampleRate, 1}, 0};
    buffer.samples().assign(samples.begin(), samples.end());

    Fooyin::ProcessingBufferList chunks;
    chunks.addChunk(std::move(buffer));
    return chunks;
}
} // namespace

namespace Fooyin::Testing {
TEST(OutputFaderTest, FadeInCompletesAtUnityGain)
{
    OutputFader fader;

    auto fadeChunk = makeChunks({1.0, 1.0, 1.0, 1.0});
    fader.fadeIn(4, 0.35, SampleRate, 7);
    fader.process(fadeChunk);

    const auto completion = fader.takeCompletion();
    ASSERT_TRUE(completion.has_value());
    EXPECT_EQ(completion->type, OutputFader::CompletionType::FadeInComplete);

    auto steadyChunk = makeChunks({1.0});
    fader.process(steadyChunk);

    const auto* chunk = steadyChunk.item(0);
    ASSERT_NE(chunk, nullptr);
    ASSERT_EQ(chunk->sampleCount(), 1);
    EXPECT_DOUBLE_EQ(chunk->samples().front(), 1.0);
}

TEST(OutputFaderTest, FadeOutStartsFromUnityGain)
{
    OutputFader fader;

    auto chunk = makeChunks({1.0});
    fader.fadeOut(4, 0.35, SampleRate, 3);
    fader.process(chunk);

    const auto* processed = chunk.item(0);
    ASSERT_NE(processed, nullptr);
    ASSERT_EQ(processed->sampleCount(), 1);
    EXPECT_DOUBLE_EQ(processed->samples().front(), 1.0);
}
} // namespace Fooyin::Testing
