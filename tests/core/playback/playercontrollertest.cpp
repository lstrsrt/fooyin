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

#include <core/player/playercontroller.h>

#include <core/coresettings.h>
#include <core/track.h>
#include <utils/settings/settingsmanager.h>

#include <QDir>

#include <gtest/gtest.h>

using namespace Qt::StringLiterals;

namespace Fooyin::Testing {
namespace {
void registerControllerSettings(SettingsManager& settings)
{
    settings.createSetting<Settings::Core::PlayMode>(0, QString::fromLatin1(Settings::Core::PlayModeKey));
    settings.createSetting<Settings::Core::StopAfterCurrent>(false, u"Playback/StopAfterCurrent"_s);
    settings.createSetting<Settings::Core::ResetStopAfterCurrent>(false, u"Playback/ResetStopAfterCurrent"_s);
    settings.createSetting<Settings::Core::PlayedThreshold>(0.5, u"Playback/PlayedThreshold"_s);
    settings.createSetting<Settings::Core::RewindPreviousTrack>(false, u"Playlist/RewindPreviousTrack"_s);
    settings.createSetting<Settings::Core::PlaybackQueueStopWhenFinished>(false,
                                                                          u"Playback/PlaybackQueueStopWhenFinished"_s);
}

Track makeTrack(const QString& path, int id, uint64_t durationMs)
{
    Track track{path, 0};
    track.setId(id);
    track.setDuration(durationMs);
    track.generateHash();
    return track;
}
} // namespace

TEST(PlayerControllerTest, ChangeAndCommitTrackUpdatePublicState)
{
    SettingsManager settings{QDir::tempPath() + u"/fooyin_playercontroller_change_commit_test.ini"_s};
    registerControllerSettings(settings);
    PlayerController controller{&settings, nullptr};

    int requestCount{0};
    Player::TrackChangeRequest lastRequest;
    QObject::connect(&controller, &PlayerController::trackChangeRequested, &controller,
                     [&requestCount, &lastRequest](const Player::TrackChangeRequest& request) {
                         ++requestCount;
                         lastRequest = request;
                     });

    const Track track = makeTrack(u"/tmp/controller.flac"_s, 11, 2000);
    controller.changeCurrentTrack(PlaylistTrack{.track = track, .playlistId = UId::create(), .indexInPlaylist = 0},
                                  {.reason = Player::AdvanceReason::ManualSelection, .userInitiated = true});

    ASSERT_EQ(requestCount, 1);
    EXPECT_EQ(lastRequest.track.track, track);
    EXPECT_GT(lastRequest.itemId, 0);
    EXPECT_EQ(lastRequest.context.reason, Player::AdvanceReason::ManualSelection);
    EXPECT_TRUE(lastRequest.context.userInitiated);

    controller.commitCurrentTrack(lastRequest);

    EXPECT_EQ(controller.currentTrack(), track);
    EXPECT_EQ(controller.currentTrackId(), track.id());
    EXPECT_EQ(controller.currentPlaylistTrack().track, track);
    EXPECT_EQ(controller.lastTrackChangeContext().reason, Player::AdvanceReason::ManualSelection);
    EXPECT_TRUE(controller.lastTrackChangeContext().userInitiated);
}

TEST(PlayerControllerTest, PlayAndPauseEmitTransportStateChanges)
{
    SettingsManager settings{QDir::tempPath() + u"/fooyin_playercontroller_play_pause_test.ini"_s};
    registerControllerSettings(settings);
    PlayerController controller{&settings, nullptr};

    int transportPlayCount{0};
    int transportPauseCount{0};
    int playStateChanges{0};

    QObject::connect(&controller, &PlayerController::transportPlayRequested, &controller,
                     [&transportPlayCount]() { ++transportPlayCount; });
    QObject::connect(&controller, &PlayerController::transportPauseRequested, &controller,
                     [&transportPauseCount]() { ++transportPauseCount; });
    QObject::connect(&controller, &PlayerController::playStateChanged, &controller,
                     [&playStateChanges]() { ++playStateChanges; });

    controller.changeCurrentTrack(makeTrack(u"/tmp/pending.flac"_s, 12, 1500));
    controller.play();

    EXPECT_EQ(controller.playState(), Player::PlayState::Playing);
    EXPECT_EQ(transportPlayCount, 1);
    EXPECT_EQ(playStateChanges, 1);

    controller.pause();

    EXPECT_EQ(controller.playState(), Player::PlayState::Paused);
    EXPECT_EQ(transportPauseCount, 1);
    EXPECT_EQ(playStateChanges, 2);
}

TEST(PlayerControllerTest, TrackPlayedEmitsOnceAfterCrossingThreshold)
{
    SettingsManager settings{QDir::tempPath() + u"/fooyin_playercontroller_track_played_test.ini"_s};
    registerControllerSettings(settings);
    PlayerController controller{&settings, nullptr};

    int playedCount{0};
    QObject::connect(&controller, &PlayerController::trackPlayed, &controller,
                     [&playedCount](const Track&) { ++playedCount; });

    controller.commitCurrentTrack(makeTrack(u"/tmp/played.flac"_s, 13, 1000));
    controller.setCurrentPosition(100);
    controller.setCurrentPosition(500);
    controller.setCurrentPosition(800);

    EXPECT_EQ(playedCount, 1);
}

TEST(PlayerControllerTest, NextClearsStopAfterCurrentWhenResetEnabled)
{
    SettingsManager settings{QDir::tempPath() + u"/fooyin_playercontroller_next_clears_stop_current_test.ini"_s};
    registerControllerSettings(settings);
    PlayerController controller{&settings, nullptr};

    settings.set<Settings::Core::StopAfterCurrent>(true);
    settings.set<Settings::Core::ResetStopAfterCurrent>(true);

    controller.next();

    EXPECT_FALSE(settings.value<Settings::Core::StopAfterCurrent>());
    EXPECT_TRUE(controller.trackEndAutoTransitionsEnabled());
}

TEST(PlayerControllerTest, PreviousClearsStopAfterCurrentWhenResetEnabled)
{
    SettingsManager settings{QDir::tempPath() + u"/fooyin_playercontroller_prev_clears_stop_current_test.ini"_s};
    registerControllerSettings(settings);
    PlayerController controller{&settings, nullptr};

    settings.set<Settings::Core::StopAfterCurrent>(true);
    settings.set<Settings::Core::ResetStopAfterCurrent>(true);
    settings.set<Settings::Core::RewindPreviousTrack>(true);

    controller.commitCurrentTrack(makeTrack(u"/tmp/prev-stop-current.flac"_s, 15, 10000));
    controller.setCurrentPosition(6000);
    controller.previous();

    EXPECT_FALSE(settings.value<Settings::Core::StopAfterCurrent>());
    EXPECT_TRUE(controller.trackEndAutoTransitionsEnabled());
}

TEST(PlayerControllerTest, StopAfterCurrentResetWaitsForEngineStoppedState)
{
    SettingsManager settings{QDir::tempPath() + u"/fooyin_playercontroller_deferred_stop_current_reset_test.ini"_s};
    registerControllerSettings(settings);
    PlayerController controller{&settings, nullptr};

    settings.set<Settings::Core::StopAfterCurrent>(true);
    settings.set<Settings::Core::ResetStopAfterCurrent>(true);

    EXPECT_TRUE(settings.value<Settings::Core::StopAfterCurrent>());
    EXPECT_FALSE(controller.trackEndAutoTransitionsEnabled());

    controller.commitCurrentTrack(makeTrack(u"/tmp/stop-current.flac"_s, 14, 1000));
    controller.play();
    controller.advance(Player::AdvanceReason::NaturalEnd);

    EXPECT_TRUE(settings.value<Settings::Core::StopAfterCurrent>());
    EXPECT_FALSE(controller.trackEndAutoTransitionsEnabled());

    controller.syncPlayStateFromEngine(Player::PlayState::Stopped);

    EXPECT_FALSE(settings.value<Settings::Core::StopAfterCurrent>());
    EXPECT_TRUE(controller.trackEndAutoTransitionsEnabled());
}
} // namespace Fooyin::Testing
