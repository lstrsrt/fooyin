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

#include <QBuffer>
#include <QImage>

#include <gtest/gtest.h>

// clazy:excludeall=returning-void-expression

constexpr auto Flags = Fooyin::AudioReader::Metadata;

using namespace Qt::StringLiterals;

namespace Fooyin::Testing {
class TagWriterTest : public ::testing::Test
{
protected:
    TagLibReader m_parser;
};

namespace {
QByteArray createPngCover(const QSize& size)
{
    QImage image(size, QImage::Format_ARGB32);

    for(int y{0}; y < image.height(); ++y) {
        for(int x{0}; x < image.width(); ++x) {
            image.setPixelColor(x, y,
                                QColor::fromRgb((x * 255) / image.width(), (y * 255) / image.height(),
                                                ((x + y) * 255) / (image.width() + image.height())));
        }
    }

    QByteArray data;
    QBuffer buffer{&data};
    if(buffer.open(QIODevice::WriteOnly)) {
        image.save(&buffer, "PNG");
    }

    return data;
}
} // namespace

TEST_F(TagWriterTest, AiffWrite)
{
    const QString filepath = u":/audio/audiotest.aiff"_s;
    TempResource file{filepath};
    file.checkValid();

    AudioSource source;
    source.filepath = file.fileName();
    source.device   = &file;

    {
        Track track{file.fileName()};
        ASSERT_TRUE(m_parser.readTrack(source, track));

        track.setId(0);
        track.setTitle(u"TestTitle"_s);
        track.setAlbum({u"TestAlbum"_s});
        track.setAlbumArtists({u"TestAArtist"_s});
        track.setArtists({u"TestArtist"_s});
        track.setDate(u"2023-12-12"_s);
        track.setTrackNumber(u"9"_s);
        track.setTrackTotal(u"99"_s);
        track.setDiscNumber(u"4"_s);
        track.setDiscTotal(u"44"_s);
        track.setGenres({u"TestGenre"_s});
        track.setPerformers({u"TestPerformer"_s});
        track.setComposers({u"testComposer"_s});
        track.setComment(u"TestComment"_s);
        track.addExtraTag(u"WRITETEST"_s, u"Success"_s);
        track.removeExtraTag(u"TEST"_s);

        ASSERT_TRUE(m_parser.writeTrack(source, track, Flags));
    }

    {
        Track track{file.fileName()};
        ASSERT_TRUE(m_parser.readTrack(source, track));

        EXPECT_EQ(track.title(), u"TestTitle"_s);
        EXPECT_EQ(track.album(), u"TestAlbum"_s);
        EXPECT_EQ(track.albumArtist(), u"TestAArtist"_s);
        EXPECT_EQ(track.artist(), u"TestArtist"_s);
        EXPECT_EQ(track.date(), u"2023-12-12"_s);
        EXPECT_EQ(track.trackNumber(), u"9"_s);
        EXPECT_EQ(track.trackTotal(), u"99"_s);
        EXPECT_EQ(track.discNumber(), u"4"_s);
        EXPECT_EQ(track.discTotal(), u"44"_s);
        EXPECT_EQ(track.genre(), u"TestGenre"_s);
        EXPECT_EQ(track.performer(), u"TestPerformer"_s);
        EXPECT_EQ(track.composer(), u"testComposer"_s);
        EXPECT_EQ(track.comment(), u"TestComment"_s);

        const auto testTag = track.extraTag(u"TEST"_s);
        EXPECT_TRUE(testTag.empty());

        const auto writeTag = track.extraTag(u"WRITETEST"_s);
        ASSERT_TRUE(!writeTag.isEmpty());
        EXPECT_EQ(writeTag.front(), u"Success"_s);
    }
}

TEST_F(TagWriterTest, FlacWrite)
{
    const QString filepath = u":/audio/audiotest.flac"_s;
    TempResource file{filepath};
    file.checkValid();

    AudioSource source;
    source.filepath = file.fileName();
    source.device   = &file;

    {
        Track track{file.fileName()};
        ASSERT_TRUE(m_parser.readTrack({filepath, &file, nullptr}, track));

        track.setId(0);
        track.setTitle(u"TestTitle"_s);
        track.setAlbum({u"TestAlbum"_s});
        track.setAlbumArtists({u"TestAArtist"_s});
        track.setArtists({u"TestArtist"_s});
        track.setDate(u"2023-12-12"_s);
        track.setTrackNumber(u"9"_s);
        track.setTrackTotal(u"99"_s);
        track.setDiscNumber(u"4"_s);
        track.setDiscTotal(u"44"_s);
        track.setGenres({u"TestGenre"_s});
        track.setPerformers({u"TestPerformer"_s});
        track.setComposers({u"testComposer"_s});
        track.setComment(u"TestComment"_s);
        track.addExtraTag(u"WRITETEST"_s, u"Success"_s);
        track.removeExtraTag(u"TEST"_s);

        ASSERT_TRUE(m_parser.writeTrack(source, track, Flags));
    }

    {
        Track track{file.fileName()};
        ASSERT_TRUE(m_parser.readTrack({filepath, &file, nullptr}, track));

        EXPECT_EQ(track.title(), u"TestTitle"_s);
        EXPECT_EQ(track.album(), u"TestAlbum"_s);
        EXPECT_EQ(track.albumArtist(), u"TestAArtist"_s);
        EXPECT_EQ(track.artist(), u"TestArtist"_s);
        EXPECT_EQ(track.date(), u"2023-12-12"_s);
        EXPECT_EQ(track.trackNumber(), u"9"_s);
        EXPECT_EQ(track.trackTotal(), u"99"_s);
        EXPECT_EQ(track.discNumber(), u"4"_s);
        EXPECT_EQ(track.discTotal(), u"44"_s);
        EXPECT_EQ(track.genre(), u"TestGenre"_s);
        EXPECT_EQ(track.performer(), u"TestPerformer"_s);
        EXPECT_EQ(track.composer(), u"testComposer"_s);
        EXPECT_EQ(track.comment(), u"TestComment"_s);

        const auto testTag = track.extraTag(u"TEST"_s);
        EXPECT_TRUE(testTag.empty());

        const auto writeTag = track.extraTag(u"WRITETEST"_s);
        ASSERT_TRUE(!writeTag.isEmpty());
        EXPECT_EQ(writeTag.front(), u"Success"_s);
    }
}

TEST_F(TagWriterTest, FlacCoverWrite)
{
    const QString filepath = u":/audio/audiotest.flac"_s;
    TempResource file{filepath};
    file.checkValid();

    AudioSource source;
    source.filepath = file.fileName();
    source.device   = &file;

    const QByteArray coverData = createPngCover({4000, 4000});
    ASSERT_FALSE(coverData.isEmpty());

    TrackCovers covers;
    covers.emplace(Track::Cover::Front, CoverImage{.mimeType = u"image/png"_s, .data = coverData});

    Track track{file.fileName()};
    ASSERT_TRUE(m_parser.writeCover(source, track, covers, Flags));

    const QByteArray writtenCover = m_parser.readCover(source, track, Track::Cover::Front);
    ASSERT_FALSE(writtenCover.isEmpty());
    EXPECT_EQ(writtenCover, coverData);
}

TEST_F(TagWriterTest, M4aWrite)
{
    const QString filepath = u":/audio/audiotest.m4a"_s;
    TempResource file{filepath};
    file.checkValid();

    AudioSource source;
    source.filepath = file.fileName();
    source.device   = &file;

    {
        Track track{file.fileName()};
        ASSERT_TRUE(m_parser.readTrack(source, track));

        track.setId(0);
        track.setTitle(u"TestTitle"_s);
        track.setAlbum({u"TestAlbum"_s});
        track.setAlbumArtists({u"TestAArtist"_s});
        track.setArtists({u"TestArtist"_s});
        track.setDate(u"2023-12-12"_s);
        track.setTrackNumber(u"9"_s);
        track.setTrackTotal(u"99"_s);
        track.setDiscNumber(u"4"_s);
        track.setDiscTotal(u"44"_s);
        track.setGenres({u"TestGenre"_s});
        track.setPerformers({u"TestPerformer"_s});
        track.setComposers({u"testComposer"_s});
        track.setComment(u"TestComment"_s);
        track.addExtraTag(u"WRITETEST"_s, u"Success"_s);
        track.removeExtraTag(u"TEST"_s);

        ASSERT_TRUE(m_parser.writeTrack(source, track, Flags));
    }

    {
        Track track{file.fileName()};
        ASSERT_TRUE(m_parser.readTrack({filepath, &file, nullptr}, track));

        EXPECT_EQ(track.title(), u"TestTitle"_s);
        EXPECT_EQ(track.album(), u"TestAlbum"_s);
        EXPECT_EQ(track.albumArtist(), u"TestAArtist"_s);
        EXPECT_EQ(track.artist(), u"TestArtist"_s);
        EXPECT_EQ(track.date(), u"2023-12-12"_s);
        EXPECT_EQ(track.trackNumber(), u"9"_s);
        EXPECT_EQ(track.trackTotal(), u"99"_s);
        EXPECT_EQ(track.discNumber(), u"4"_s);
        EXPECT_EQ(track.discTotal(), u"44"_s);
        EXPECT_EQ(track.genre(), u"TestGenre"_s);
        EXPECT_EQ(track.performer(), u"TestPerformer"_s);
        EXPECT_EQ(track.composer(), u"testComposer"_s);
        EXPECT_EQ(track.comment(), u"TestComment"_s);

        const auto testTag = track.extraTag(u"TEST"_s);
        EXPECT_TRUE(testTag.empty());

        const auto writeTag = track.extraTag(u"WRITETEST"_s);
        ASSERT_TRUE(!writeTag.isEmpty());
        EXPECT_EQ(writeTag.front(), u"Success"_s);
    }
}

TEST_F(TagWriterTest, Mp3Write)
{
    const QString filepath = u":/audio/audiotest.mp3"_s;
    TempResource file{filepath};
    file.checkValid();

    AudioSource source;
    source.filepath = file.fileName();
    source.device   = &file;

    {
        Track track{file.fileName()};
        ASSERT_TRUE(m_parser.readTrack(source, track));

        track.setId(0);
        track.setTitle(u"TestTitle"_s);
        track.setAlbum({u"TestAlbum"_s});
        track.setAlbumArtists({u"TestAArtist"_s});
        track.setArtists({u"TestArtist"_s});
        track.setDate(u"2023-12-12"_s);
        track.setTrackNumber(u"9"_s);
        track.setTrackTotal(u"99"_s);
        track.setDiscNumber(u"4"_s);
        track.setDiscTotal(u"44"_s);
        track.setGenres({u"TestGenre"_s});
        track.setPerformers({u"TestPerformer"_s});
        track.setComposers({u"testComposer"_s});
        track.setComment(u"TestComment"_s);
        track.addExtraTag(u"WRITETEST"_s, u"Success"_s);
        track.removeExtraTag(u"TEST"_s);

        ASSERT_TRUE(m_parser.writeTrack(source, track, Flags));
    }

    {
        Track track{file.fileName()};
        ASSERT_TRUE(m_parser.readTrack({filepath, &file, nullptr}, track));

        EXPECT_EQ(track.title(), u"TestTitle"_s);
        EXPECT_EQ(track.album(), u"TestAlbum"_s);
        EXPECT_EQ(track.albumArtist(), u"TestAArtist"_s);
        EXPECT_EQ(track.artist(), u"TestArtist"_s);
        EXPECT_EQ(track.date(), u"2023-12-12"_s);
        EXPECT_EQ(track.trackNumber(), u"9"_s);
        EXPECT_EQ(track.trackTotal(), u"99"_s);
        EXPECT_EQ(track.discNumber(), u"4"_s);
        EXPECT_EQ(track.discTotal(), u"44"_s);
        EXPECT_EQ(track.genre(), u"TestGenre"_s);
        EXPECT_EQ(track.performer(), u"TestPerformer"_s);
        EXPECT_EQ(track.composer(), u"testComposer"_s);
        EXPECT_EQ(track.comment(), u"TestComment"_s);

        const auto testTag = track.extraTag(u"TEST"_s);
        EXPECT_TRUE(testTag.empty());

        const auto writeTag = track.extraTag(u"WRITETEST"_s);
        ASSERT_TRUE(!writeTag.isEmpty());
        EXPECT_EQ(writeTag.front(), u"Success"_s);
    }
}

TEST_F(TagWriterTest, OggWrite)
{
    const QString filepath = u":/audio/audiotest.ogg"_s;
    TempResource file{filepath};
    file.checkValid();

    AudioSource source;
    source.filepath = file.fileName();
    source.device   = &file;

    {
        Track track{file.fileName()};
        ASSERT_TRUE(m_parser.readTrack(source, track));

        track.setId(0);
        track.setTitle(u"TestTitle"_s);
        track.setAlbum({u"TestAlbum"_s});
        track.setAlbumArtists({u"TestAArtist"_s});
        track.setArtists({u"TestArtist"_s});
        track.setDate(u"2023-12-12"_s);
        track.setTrackNumber(u"9"_s);
        track.setTrackTotal(u"99"_s);
        track.setDiscNumber(u"4"_s);
        track.setDiscTotal(u"44"_s);
        track.setGenres({u"TestGenre"_s});
        track.setPerformers({u"TestPerformer"_s});
        track.setComposers({u"testComposer"_s});
        track.setComment(u"TestComment"_s);
        track.addExtraTag(u"WRITETEST"_s, u"Success"_s);
        track.removeExtraTag(u"TEST"_s);

        ASSERT_TRUE(m_parser.writeTrack(source, track, Flags));
    }

    {
        Track track{file.fileName()};
        ASSERT_TRUE(m_parser.readTrack({filepath, &file, nullptr}, track));

        EXPECT_EQ(track.title(), u"TestTitle"_s);
        EXPECT_EQ(track.album(), u"TestAlbum"_s);
        EXPECT_EQ(track.albumArtist(), u"TestAArtist"_s);
        EXPECT_EQ(track.artist(), u"TestArtist"_s);
        EXPECT_EQ(track.date(), u"2023-12-12"_s);
        EXPECT_EQ(track.trackNumber(), u"9"_s);
        EXPECT_EQ(track.trackTotal(), u"99"_s);
        EXPECT_EQ(track.discNumber(), u"4"_s);
        EXPECT_EQ(track.discTotal(), u"44"_s);
        EXPECT_EQ(track.genre(), u"TestGenre"_s);
        EXPECT_EQ(track.performer(), u"TestPerformer"_s);
        EXPECT_EQ(track.composer(), u"testComposer"_s);
        EXPECT_EQ(track.comment(), u"TestComment"_s);

        const auto testTag = track.extraTag(u"TEST"_s);
        EXPECT_TRUE(testTag.empty());

        const auto writeTag = track.extraTag(u"WRITETEST"_s);
        ASSERT_TRUE(!writeTag.isEmpty());
        EXPECT_EQ(writeTag.front(), u"Success"_s);
    }
}

TEST_F(TagWriterTest, OpusWrite)
{
    const QString filepath = u":/audio/audiotest.opus"_s;
    TempResource file{filepath};
    file.checkValid();

    AudioSource source;
    source.filepath = file.fileName();
    source.device   = &file;

    {
        Track track{file.fileName()};
        ASSERT_TRUE(m_parser.readTrack(source, track));

        track.setId(0);
        track.setTitle(u"TestTitle"_s);
        track.setAlbum({u"TestAlbum"_s});
        track.setAlbumArtists({u"TestAArtist"_s});
        track.setArtists({u"TestArtist"_s});
        track.setDate(u"2023-12-12"_s);
        track.setTrackNumber(u"9"_s);
        track.setTrackTotal(u"99"_s);
        track.setDiscNumber(u"4"_s);
        track.setDiscTotal(u"44"_s);
        track.setGenres({u"TestGenre"_s});
        track.setPerformers({u"TestPerformer"_s});
        track.setComposers({u"testComposer"_s});
        track.setComment(u"TestComment"_s);
        track.addExtraTag(u"WRITETEST"_s, u"Success"_s);
        track.removeExtraTag(u"TEST"_s);

        ASSERT_TRUE(m_parser.writeTrack(source, track, Flags));
    }

    {
        Track track{file.fileName()};
        ASSERT_TRUE(m_parser.readTrack({filepath, &file, nullptr}, track));

        EXPECT_EQ(track.title(), u"TestTitle"_s);
        EXPECT_EQ(track.album(), u"TestAlbum"_s);
        EXPECT_EQ(track.albumArtist(), u"TestAArtist"_s);
        EXPECT_EQ(track.artist(), u"TestArtist"_s);
        EXPECT_EQ(track.date(), u"2023-12-12"_s);
        EXPECT_EQ(track.trackNumber(), u"9"_s);
        EXPECT_EQ(track.trackTotal(), u"99"_s);
        EXPECT_EQ(track.discNumber(), u"4"_s);
        EXPECT_EQ(track.discTotal(), u"44"_s);
        EXPECT_EQ(track.genre(), u"TestGenre"_s);
        EXPECT_EQ(track.performer(), u"TestPerformer"_s);
        EXPECT_EQ(track.composer(), u"testComposer"_s);
        EXPECT_EQ(track.comment(), u"TestComment"_s);

        const auto testTag = track.extraTag(u"TEST"_s);
        EXPECT_TRUE(testTag.empty());

        const auto writeTag = track.extraTag(u"WRITETEST"_s);
        ASSERT_TRUE(!writeTag.isEmpty());
        EXPECT_EQ(writeTag.front(), u"Success"_s);
    }
}

TEST_F(TagWriterTest, WavWrite)
{
    const QString filepath = u":/audio/audiotest.wav"_s;
    TempResource file{filepath};
    file.checkValid();

    AudioSource source;
    source.filepath = file.fileName();
    source.device   = &file;

    {
        Track track{file.fileName()};
        ASSERT_TRUE(m_parser.readTrack(source, track));

        track.setId(0);
        track.setTitle(u"TestTitle"_s);
        track.setAlbum({u"TestAlbum"_s});
        track.setAlbumArtists({u"TestAArtist"_s});
        track.setArtists({u"TestArtist"_s});
        track.setDate(u"2023-12-12"_s);
        track.setTrackNumber(u"9"_s);
        track.setTrackTotal(u"99"_s);
        track.setDiscNumber(u"4"_s);
        track.setDiscTotal(u"44"_s);
        track.setGenres({u"TestGenre"_s});
        track.setPerformers({u"TestPerformer"_s});
        track.setComposers({u"testComposer"_s});
        track.setComment(u"TestComment"_s);
        track.addExtraTag(u"WRITETEST"_s, u"Success"_s);
        track.removeExtraTag(u"TEST"_s);

        ASSERT_TRUE(m_parser.writeTrack(source, track, Flags));
    }

    {
        Track track{file.fileName()};
        ASSERT_TRUE(m_parser.readTrack(source, track));

        EXPECT_EQ(track.title(), u"TestTitle"_s);
        EXPECT_EQ(track.album(), u"TestAlbum"_s);
        EXPECT_EQ(track.albumArtist(), u"TestAArtist"_s);
        EXPECT_EQ(track.artist(), u"TestArtist"_s);
        EXPECT_EQ(track.date(), u"2023-12-12"_s);
        EXPECT_EQ(track.trackNumber(), u"9"_s);
        EXPECT_EQ(track.trackTotal(), u"99"_s);
        EXPECT_EQ(track.discNumber(), u"4"_s);
        EXPECT_EQ(track.discTotal(), u"44"_s);
        EXPECT_EQ(track.genre(), u"TestGenre"_s);
        EXPECT_EQ(track.performer(), u"TestPerformer"_s);
        EXPECT_EQ(track.composer(), u"testComposer"_s);
        EXPECT_EQ(track.comment(), u"TestComment"_s);

        const auto testTag = track.extraTag(u"TEST"_s);
        EXPECT_TRUE(testTag.empty());

        const auto writeTag = track.extraTag(u"WRITETEST"_s);
        ASSERT_TRUE(!writeTag.isEmpty());
        EXPECT_EQ(writeTag.front(), u"Success"_s);
    }
}
} // namespace Fooyin::Testing
