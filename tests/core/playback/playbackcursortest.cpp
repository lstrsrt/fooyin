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

#include "core/playback/playbackcursor.h"

#include <gtest/gtest.h>

namespace Fooyin::Testing {
TEST(PlaybackCursorTest, RestartTargetsDependOnStoppedBoundary)
{
    PlaybackCursor cursor;

    EXPECT_EQ(cursor.restartTargetFor(PlaybackCursor::Command::Play, Player::PlayState::Stopped),
              PlaybackOrderNavigator::RestartTarget::None);
    EXPECT_EQ(cursor.restartTargetFor(PlaybackCursor::Command::Previous, Player::PlayState::Stopped),
              PlaybackOrderNavigator::RestartTarget::None);
    EXPECT_EQ(cursor.restartTargetFor(PlaybackCursor::Command::Next, Player::PlayState::Stopped),
              PlaybackOrderNavigator::RestartTarget::None);

    cursor.stopAt(PlaybackCursor::BoundaryStop::End);

    EXPECT_EQ(cursor.restartTargetFor(PlaybackCursor::Command::Play, Player::PlayState::Stopped),
              PlaybackOrderNavigator::RestartTarget::Beginning);
    EXPECT_EQ(cursor.restartTargetFor(PlaybackCursor::Command::Next, Player::PlayState::Stopped),
              PlaybackOrderNavigator::RestartTarget::Beginning);
    EXPECT_EQ(cursor.restartTargetFor(PlaybackCursor::Command::Previous, Player::PlayState::Stopped),
              PlaybackOrderNavigator::RestartTarget::None);

    cursor.stopAt(PlaybackCursor::BoundaryStop::Start);

    EXPECT_EQ(cursor.restartTargetFor(PlaybackCursor::Command::Previous, Player::PlayState::Stopped),
              PlaybackOrderNavigator::RestartTarget::End);
    EXPECT_EQ(cursor.restartTargetFor(PlaybackCursor::Command::Play, Player::PlayState::Stopped),
              PlaybackOrderNavigator::RestartTarget::None);
    EXPECT_EQ(cursor.restartTargetFor(PlaybackCursor::Command::Next, Player::PlayState::Stopped),
              PlaybackOrderNavigator::RestartTarget::None);
}

TEST(PlaybackCursorTest, ResetsBoundaryStateOnSeekCommitAndReset)
{
    PlaybackCursor cursor;
    cursor.stopAt(PlaybackCursor::BoundaryStop::End);

    EXPECT_EQ(cursor.restartTargetFor(PlaybackCursor::Command::Next, Player::PlayState::Playing),
              PlaybackOrderNavigator::RestartTarget::None);

    cursor.onSeek();
    EXPECT_EQ(cursor.restartTargetFor(PlaybackCursor::Command::Next, Player::PlayState::Stopped),
              PlaybackOrderNavigator::RestartTarget::None);

    cursor.stopAt(PlaybackCursor::BoundaryStop::Start);
    cursor.onTrackCommitted();
    EXPECT_EQ(cursor.restartTargetFor(PlaybackCursor::Command::Previous, Player::PlayState::Stopped),
              PlaybackOrderNavigator::RestartTarget::None);

    cursor.stopAt(PlaybackCursor::BoundaryStop::End);
    cursor.reset();
    EXPECT_EQ(cursor.restartTargetFor(PlaybackCursor::Command::Play, Player::PlayState::Stopped),
              PlaybackOrderNavigator::RestartTarget::None);
}
} // namespace Fooyin::Testing
