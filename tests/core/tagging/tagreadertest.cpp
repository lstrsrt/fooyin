/*
 * Fooyin
 * Copyright © 2023, Luke Taylor <LukeT1@proton.me>
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

#include "testutils.h"

#include <core/engine/input/taglibparser.h>
#include <core/track.h>

#include <gtest/gtest.h>

// clazy:excludeall=returning-void-expression
using namespace Qt::StringLiterals;

namespace Fooyin::Testing {
class TagReaderTest : public ::testing::Test
{
protected:
    TagLibReader m_parser;
};

TEST_F(TagReaderTest, AiffRead)
{
    const QString filepath = u":/audio/audiotest.aiff"_s;
    TempResource file{filepath};
    file.checkValid();

    Track track{file.fileName()};
    ASSERT_TRUE(m_parser.readTrack({filepath, &file, nullptr}, track));

    EXPECT_EQ(track.codec(), u"AIFF"_s);
    EXPECT_EQ(track.title(), u"AIFF Test"_s);
    EXPECT_EQ(track.album(), u"Fooyin Audio Tests"_s);
    EXPECT_EQ(track.albumArtist(), u"Fooyin"_s);
    EXPECT_EQ(track.artist(), u"Fooyin"_s);
    EXPECT_EQ(track.date(), u"2023"_s);
    EXPECT_EQ(track.trackNumber(), u"1"_s);
    EXPECT_EQ(track.trackTotal(), u"7"_s);
    EXPECT_EQ(track.discNumber(), u"1"_s);
    EXPECT_EQ(track.discTotal(), u"1"_s);
    EXPECT_EQ(track.genre(), u"Testing"_s);
    EXPECT_EQ(track.performer(), u"Fooyin"_s);
    EXPECT_EQ(track.composer(), u"Fooyin"_s);
    EXPECT_EQ(track.comment(), u"A fooyin test"_s);
    EXPECT_GT(track.duration(), 0);

    const auto testTag = track.extraTag(u"TEST"_s);
    ASSERT_TRUE(!testTag.isEmpty());
    EXPECT_EQ(testTag.front(), u"A custom tag"_s);
}

TEST_F(TagReaderTest, FlacRead)
{
    const QString filepath = u":/audio/audiotest.flac"_s;
    TempResource file{filepath};
    file.checkValid();

    Track track{file.fileName()};
    ASSERT_TRUE(m_parser.readTrack({filepath, &file, nullptr}, track));

    EXPECT_EQ(track.codec(), u"FLAC"_s);
    EXPECT_EQ(track.title(), u"FLAC Test"_s);
    EXPECT_EQ(track.album(), u"Fooyin Audio Tests"_s);
    EXPECT_EQ(track.albumArtist(), u"Fooyin"_s);
    EXPECT_EQ(track.artist(), u"Fooyin"_s);
    EXPECT_EQ(track.date(), u"2023"_s);
    EXPECT_EQ(track.trackNumber(), u"2"_s);
    EXPECT_EQ(track.trackTotal(), u"7"_s);
    EXPECT_EQ(track.discNumber(), u"1"_s);
    EXPECT_EQ(track.discTotal(), u"1"_s);
    EXPECT_EQ(track.genre(), u"Testing"_s);
    EXPECT_EQ(track.performer(), u"Fooyin"_s);
    EXPECT_EQ(track.composer(), u"Fooyin"_s);
    EXPECT_EQ(track.comment(), u"A fooyin test"_s);
    EXPECT_GT(track.duration(), 0);

    const auto testTag = track.extraTag(u"TEST"_s);
    ASSERT_TRUE(!testTag.isEmpty());
    EXPECT_EQ(testTag.front(), u"A custom tag"_s);
}

TEST_F(TagReaderTest, M4aRead)
{
    const QString filepath = u":/audio/audiotest.m4a"_s;
    TempResource file{filepath};
    file.checkValid();

    Track track{file.fileName()};
    ASSERT_TRUE(m_parser.readTrack({filepath, &file, nullptr}, track));

    EXPECT_EQ(track.codec(), u"AAC"_s);
    EXPECT_EQ(track.title(), u"M4A Test"_s);
    EXPECT_EQ(track.album(), u"Fooyin Audio Tests"_s);
    EXPECT_EQ(track.albumArtist(), u"Fooyin"_s);
    EXPECT_EQ(track.artist(), u"Fooyin"_s);
    EXPECT_EQ(track.date(), u"2023"_s);
    EXPECT_EQ(track.trackNumber(), u"3"_s);
    EXPECT_EQ(track.trackTotal(), u"7"_s);
    EXPECT_EQ(track.discNumber(), u"1"_s);
    EXPECT_EQ(track.discTotal(), u"1"_s);
    EXPECT_EQ(track.genre(), u"Testing"_s);
    EXPECT_EQ(track.composer(), u"Fooyin"_s);
    EXPECT_EQ(track.comment(), u"A fooyin test"_s);
    EXPECT_GT(track.duration(), 0);

    const auto testTag = track.extraTag(u"TEST"_s);
    ASSERT_TRUE(!testTag.isEmpty());
    EXPECT_EQ(testTag.front(), u"A custom tag"_s);
}

TEST_F(TagReaderTest, Mp3Read)
{
    const QString filepath = u":/audio/audiotest.mp3"_s;
    TempResource file{filepath};
    file.checkValid();

    Track track{file.fileName()};
    ASSERT_TRUE(m_parser.readTrack({filepath, &file, nullptr}, track));

    EXPECT_EQ(track.codec(), u"MP3"_s);
    EXPECT_EQ(track.title(), u"MP3 Test"_s);
    EXPECT_EQ(track.album(), u"Fooyin Audio Tests"_s);
    EXPECT_EQ(track.albumArtist(), u"Fooyin"_s);
    EXPECT_EQ(track.artist(), u"Fooyin"_s);
    EXPECT_EQ(track.date(), u"2023"_s);
    EXPECT_EQ(track.trackNumber(), u"4"_s);
    EXPECT_EQ(track.trackTotal(), u"7"_s);
    EXPECT_EQ(track.discNumber(), u"1"_s);
    EXPECT_EQ(track.discTotal(), u"1"_s);
    EXPECT_EQ(track.genre(), u"Testing"_s);
    EXPECT_EQ(track.performer(), u"Fooyin"_s);
    EXPECT_EQ(track.composer(), u"Fooyin"_s);
    EXPECT_EQ(track.comment(), u"A fooyin test"_s);
    EXPECT_GT(track.duration(), 0);

    const auto testTag = track.extraTag(u"TEST"_s);
    ASSERT_TRUE(!testTag.isEmpty());
    EXPECT_EQ(testTag.front(), u"A custom tag"_s);
}

TEST_F(TagReaderTest, OggRead)
{
    const QString filepath = u":/audio/audiotest.ogg"_s;
    TempResource file{filepath};
    file.checkValid();

    Track track{file.fileName()};
    ASSERT_TRUE(m_parser.readTrack({filepath, &file, nullptr}, track));

    EXPECT_EQ(track.codec(), u"Vorbis"_s);
    EXPECT_EQ(track.title(), u"OGG Test"_s);
    EXPECT_EQ(track.album(), u"Fooyin Audio Tests"_s);
    EXPECT_EQ(track.albumArtist(), u"Fooyin"_s);
    EXPECT_EQ(track.artist(), u"Fooyin"_s);
    EXPECT_EQ(track.date(), u"2023"_s);
    EXPECT_EQ(track.trackNumber(), u"5"_s);
    EXPECT_EQ(track.trackTotal(), u"7"_s);
    EXPECT_EQ(track.discNumber(), u"1"_s);
    EXPECT_EQ(track.discTotal(), u"1"_s);
    EXPECT_EQ(track.genre(), u"Testing"_s);
    EXPECT_EQ(track.performer(), u"Fooyin"_s);
    EXPECT_EQ(track.composer(), u"Fooyin"_s);
    EXPECT_EQ(track.comment(), u"A fooyin test"_s);
    EXPECT_GT(track.duration(), 0);

    const auto testTag = track.extraTag(u"TEST"_s);
    ASSERT_TRUE(!testTag.isEmpty());
    EXPECT_EQ(testTag.front(), u"A custom tag"_s);
}

TEST_F(TagReaderTest, OpusRead)
{
    const QString filepath = u":/audio/audiotest.opus"_s;
    TempResource file{filepath};
    file.checkValid();

    Track track{file.fileName()};
    ASSERT_TRUE(m_parser.readTrack({filepath, &file, nullptr}, track));

    EXPECT_EQ(track.codec(), u"Opus"_s);
    EXPECT_EQ(track.title(), u"OPUS Test"_s);
    EXPECT_EQ(track.album(), u"Fooyin Audio Tests"_s);
    EXPECT_EQ(track.albumArtist(), u"Fooyin"_s);
    EXPECT_EQ(track.artist(), u"Fooyin"_s);
    EXPECT_EQ(track.date(), u"2023"_s);
    EXPECT_EQ(track.trackNumber(), u"6"_s);
    EXPECT_EQ(track.trackTotal(), u"7"_s);
    EXPECT_EQ(track.discNumber(), u"1"_s);
    EXPECT_EQ(track.discTotal(), u"1"_s);
    EXPECT_EQ(track.genre(), u"Testing"_s);
    EXPECT_EQ(track.performer(), u"Fooyin"_s);
    EXPECT_EQ(track.composer(), u"Fooyin"_s);
    EXPECT_EQ(track.comment(), u"A fooyin test"_s);
    EXPECT_GT(track.duration(), 0);

    const auto testTag = track.extraTag(u"TEST"_s);
    ASSERT_TRUE(!testTag.isEmpty());
    EXPECT_EQ(testTag.front(), u"A custom tag"_s);
}

TEST_F(TagReaderTest, WavRead)
{
    const QString filepath = u":/audio/audiotest.wav"_s;
    TempResource file{filepath};
    file.checkValid();

    Track track{file.fileName()};
    ASSERT_TRUE(m_parser.readTrack({filepath, &file, nullptr}, track));

    EXPECT_EQ(track.codec(), u"PCM"_s);
    EXPECT_EQ(track.title(), u"WAV Test"_s);
    EXPECT_EQ(track.album(), u"Fooyin Audio Tests"_s);
    EXPECT_EQ(track.albumArtist(), u"Fooyin"_s);
    EXPECT_EQ(track.artist(), u"Fooyin"_s);
    EXPECT_EQ(track.date(), u"2023"_s);
    EXPECT_EQ(track.trackNumber(), u"7"_s);
    EXPECT_EQ(track.trackTotal(), u"7"_s);
    EXPECT_EQ(track.discNumber(), u"1"_s);
    EXPECT_EQ(track.discTotal(), u"1"_s);
    EXPECT_EQ(track.genre(), u"Testing"_s);
    EXPECT_EQ(track.performer(), u"Fooyin"_s);
    EXPECT_EQ(track.composer(), u"Fooyin"_s);
    EXPECT_EQ(track.comment(), u"A fooyin test"_s);
    EXPECT_GT(track.duration(), 0);

    const auto testTag = track.extraTag(u"TEST"_s);
    ASSERT_TRUE(!testTag.isEmpty());
    EXPECT_EQ(testTag.front(), u"A custom tag"_s);
}
} // namespace Fooyin::Testing
