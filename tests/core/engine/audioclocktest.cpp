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

#include <core/engine/audioclock.h>

#include <gtest/gtest.h>

namespace Fooyin::Testing {
TEST(AudioClockTest, ContinuousPausedSyncDoesNotReanchorBackwards)
{
    AudioClock clock;

    clock.applyPosition(1000, 0, 1.0, 1, AudioClock::UpdateMode::Discontinuity, true);
    EXPECT_EQ(clock.position(), 1000);

    clock.setPlaying(clock.position());
    clock.setPaused();

    clock.applyPosition(100, 200, 1.0, 1, AudioClock::UpdateMode::Continuous, true);

    EXPECT_EQ(clock.position(), 1000);
}

TEST(AudioClockTest, DiscontinuityPausedSyncCanReanchorBackwards)
{
    AudioClock clock;

    clock.applyPosition(1000, 0, 1.0, 1, AudioClock::UpdateMode::Discontinuity, true);
    EXPECT_EQ(clock.position(), 1000);

    clock.setPaused();
    clock.applyPosition(0, 0, 1.0, 1, AudioClock::UpdateMode::Discontinuity, true);

    EXPECT_EQ(clock.position(), 0);
}
} // namespace Fooyin::Testing
