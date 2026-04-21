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

#include <taglib/mp4file.h>

#include <gtest/gtest.h>

// clazy:excludeall=returning-void-expression
using namespace Qt::StringLiterals;

namespace Fooyin::Testing {
class TagReaderTest : public ::testing::Test
{
protected:
    TagLibReader m_parser;
};

namespace {
constexpr auto Mp4AlbumGain    = "----:com.apple.iTunes:replaygain_album_gain";
constexpr auto Mp4AlbumGainAlt = "----:com.apple.iTunes:REPLAYGAIN_ALBUM_GAIN";
constexpr auto Mp4AlbumPeak    = "----:com.apple.iTunes:replaygain_album_peak";
constexpr auto Mp4AlbumPeakAlt = "----:com.apple.iTunes:REPLAYGAIN_ALBUM_PEAK";
constexpr auto Mp4TrackGain    = "----:com.apple.iTunes:replaygain_track_gain";
constexpr auto Mp4TrackGainAlt = "----:com.apple.iTunes:REPLAYGAIN_TRACK_GAIN";
constexpr auto Mp4TrackPeak    = "----:com.apple.iTunes:replaygain_track_peak";
constexpr auto Mp4TrackPeakAlt = "----:com.apple.iTunes:REPLAYGAIN_TRACK_PEAK";

void clearMp4ReplayGainItems(TagLib::MP4::Tag* tag)
{
    ASSERT_NE(tag, nullptr);

    tag->removeItem(Mp4AlbumGain);
    tag->removeItem(Mp4AlbumGainAlt);
    tag->removeItem(Mp4AlbumPeak);
    tag->removeItem(Mp4AlbumPeakAlt);
    tag->removeItem(Mp4TrackGain);
    tag->removeItem(Mp4TrackGainAlt);
    tag->removeItem(Mp4TrackPeak);
    tag->removeItem(Mp4TrackPeakAlt);
}

void setMp4StringItem(TagLib::MP4::Tag* tag, const char* key, const QString& value)
{
    ASSERT_NE(tag, nullptr);

    const QByteArray utf8 = value.toUtf8();
    tag->setItem(key, {TagLib::String(utf8.constData(), TagLib::String::UTF8)});
}
} // namespace

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

TEST_F(TagReaderTest, M4aReadLowercaseReplayGain)
{
    const QString filepath = u":/audio/audiotest.m4a"_s;
    TempResource file{filepath};
    file.checkValid();

    const QByteArray localPath = file.fileName().toLocal8Bit();
    {
        TagLib::MP4::File mp4File(localPath.constData());
        ASSERT_TRUE(mp4File.isValid());
        ASSERT_TRUE(mp4File.tag());

        clearMp4ReplayGainItems(mp4File.tag());
        setMp4StringItem(mp4File.tag(), Mp4TrackGain, QStringLiteral("-7.25 dB"));
        setMp4StringItem(mp4File.tag(), Mp4TrackPeak, QStringLiteral("0.987654"));
        setMp4StringItem(mp4File.tag(), Mp4AlbumGain, QStringLiteral("-6.50 dB"));
        setMp4StringItem(mp4File.tag(), Mp4AlbumPeak, QStringLiteral("0.876543"));
        ASSERT_TRUE(mp4File.save());
    }

    Track track{file.fileName()};
    ASSERT_TRUE(m_parser.readTrack({filepath, &file, nullptr}, track));

    EXPECT_TRUE(track.hasTrackGain());
    EXPECT_TRUE(track.hasTrackPeak());
    EXPECT_TRUE(track.hasAlbumGain());
    EXPECT_TRUE(track.hasAlbumPeak());
    EXPECT_FLOAT_EQ(track.rgTrackGain(), -7.25F);
    EXPECT_FLOAT_EQ(track.rgTrackPeak(), 0.987654F);
    EXPECT_FLOAT_EQ(track.rgAlbumGain(), -6.50F);
    EXPECT_FLOAT_EQ(track.rgAlbumPeak(), 0.876543F);

    EXPECT_TRUE(track.extraTag(QStringLiteral("REPLAYGAIN_TRACK_GAIN")).isEmpty());
    EXPECT_TRUE(track.extraTag(QStringLiteral("REPLAYGAIN_TRACK_PEAK")).isEmpty());
    EXPECT_TRUE(track.extraTag(QStringLiteral("REPLAYGAIN_ALBUM_GAIN")).isEmpty());
    EXPECT_TRUE(track.extraTag(QStringLiteral("REPLAYGAIN_ALBUM_PEAK")).isEmpty());
}

TEST_F(TagReaderTest, M4aReadUppercaseReplayGain)
{
    const QString filepath = u":/audio/audiotest.m4a"_s;
    TempResource file{filepath};
    file.checkValid();

    const QByteArray localPath = file.fileName().toLocal8Bit();
    {
        TagLib::MP4::File mp4File(localPath.constData());
        ASSERT_TRUE(mp4File.isValid());
        ASSERT_TRUE(mp4File.tag());

        clearMp4ReplayGainItems(mp4File.tag());
        setMp4StringItem(mp4File.tag(), Mp4TrackGainAlt, QStringLiteral("-8.00 dB"));
        setMp4StringItem(mp4File.tag(), Mp4TrackPeakAlt, QStringLiteral("0.765432"));
        setMp4StringItem(mp4File.tag(), Mp4AlbumGainAlt, QStringLiteral("-7.50 dB"));
        setMp4StringItem(mp4File.tag(), Mp4AlbumPeakAlt, QStringLiteral("0.654321"));
        ASSERT_TRUE(mp4File.save());
    }

    Track track{file.fileName()};
    ASSERT_TRUE(m_parser.readTrack({filepath, &file, nullptr}, track));

    EXPECT_TRUE(track.hasTrackGain());
    EXPECT_TRUE(track.hasTrackPeak());
    EXPECT_TRUE(track.hasAlbumGain());
    EXPECT_TRUE(track.hasAlbumPeak());
    EXPECT_FLOAT_EQ(track.rgTrackGain(), -8.00F);
    EXPECT_FLOAT_EQ(track.rgTrackPeak(), 0.765432F);
    EXPECT_FLOAT_EQ(track.rgAlbumGain(), -7.50F);
    EXPECT_FLOAT_EQ(track.rgAlbumPeak(), 0.654321F);

    EXPECT_TRUE(track.extraTag(QStringLiteral("REPLAYGAIN_TRACK_GAIN")).isEmpty());
    EXPECT_TRUE(track.extraTag(QStringLiteral("REPLAYGAIN_TRACK_PEAK")).isEmpty());
    EXPECT_TRUE(track.extraTag(QStringLiteral("REPLAYGAIN_ALBUM_GAIN")).isEmpty());
    EXPECT_TRUE(track.extraTag(QStringLiteral("REPLAYGAIN_ALBUM_PEAK")).isEmpty());
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
