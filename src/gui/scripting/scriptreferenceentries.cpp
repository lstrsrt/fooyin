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

#include "scriptreferenceentries.h"

#include <core/constants.h>
#include <gui/scripting/scriptcommandhandler.h>
#include <gui/scripting/scripteditor.h>

using namespace Qt::StringLiterals;

namespace {
QString tr(const char* text)
{
    return Fooyin::ScriptEditor::tr(text);
}

QString variableLabel(const QString& name)
{
    return u"%%1%"_s.arg(name.toLower());
}

Fooyin::ScriptReferenceEntry variableEntry(const QString& name, QString category, QString description)
{
    const QString label = variableLabel(name);

    return {.kind         = Fooyin::ScriptReferenceKind::Variable,
            .label        = label,
            .insertText   = label,
            .category     = std::move(category),
            .description  = std::move(description),
            .cursorOffset = 0};
}

Fooyin::ScriptReferenceEntry variableEntry(const char* name, QString category, QString description)
{
    return variableEntry(QString::fromLatin1(name), std::move(category), std::move(description));
}

Fooyin::ScriptReferenceEntry functionEntry(const char* name, QString signature, QString description,
                                           QString category = tr("Functions"))
{
    const QString functionName = QString::fromLatin1(name);

    return {.kind         = Fooyin::ScriptReferenceKind::Function,
            .label        = std::move(signature),
            .insertText   = u"$%1()"_s.arg(functionName),
            .category     = std::move(category),
            .description  = std::move(description),
            .cursorOffset = 1};
}

Fooyin::ScriptReferenceEntry formattingEntry(const char* name, const char* signature, QString category,
                                             QString description, int cursorOffset)
{
    const QString tagName = QString::fromLatin1(name);
    const QString sig     = QString::fromLatin1(signature);

    return {.kind         = Fooyin::ScriptReferenceKind::Formatting,
            .label        = sig,
            .insertText   = u"%1</%2>"_s.arg(sig, tagName),
            .category     = std::move(category),
            .description  = std::move(description),
            .cursorOffset = cursorOffset};
}

Fooyin::ScriptReferenceEntry commandAliasEntry(const Fooyin::ScriptCommandAlias& alias)
{
    return {
        .kind         = Fooyin::ScriptReferenceKind::CommandAlias,
        .label        = alias.alias.toString(),
        .insertText   = alias.alias.toString(),
        .category     = tr(alias.category),
        .description  = tr(alias.description),
        .cursorOffset = 0,
    };
}
} // namespace

namespace Fooyin {
const std::vector<ScriptReferenceEntry>& scriptReferenceEntries()
{
    using namespace Fooyin::Constants;

    static const std::vector Entries = {
        variableEntry(MetaData::Title, tr("Metadata"), tr("Track title")),
        variableEntry(MetaData::Artist, tr("Metadata"), tr("Primary artist")),
        variableEntry(MetaData::UniqueArtist, tr("Metadata"), tr("Unique artists combined into one value")),
        variableEntry(MetaData::Album, tr("Metadata"), tr("Album title")),
        variableEntry(MetaData::AlbumArtist, tr("Metadata"), tr("Album artist")),
        variableEntry(MetaData::Track, tr("Metadata"), tr("Track number")),
        variableEntry(MetaData::TrackTotal, tr("Metadata"), tr("Total tracks on the release")),
        variableEntry(MetaData::Disc, tr("Metadata"), tr("Disc number")),
        variableEntry(MetaData::DiscTotal, tr("Metadata"), tr("Total discs on the release")),
        variableEntry(MetaData::Genre, tr("Metadata"), tr("Genres")),
        variableEntry(MetaData::Composer, tr("Metadata"), tr("Composers")),
        variableEntry(MetaData::Performer, tr("Metadata"), tr("Performers")),
        variableEntry(MetaData::Duration, tr("Metadata"), tr("Track duration formatted as time")),
        variableEntry(MetaData::DurationSecs, tr("Metadata"), tr("Track duration in seconds")),
        variableEntry(MetaData::DurationMSecs, tr("Metadata"), tr("Track duration in milliseconds")),
        variableEntry(MetaData::Comment, tr("Metadata"), tr("Comment tag")),
        variableEntry(MetaData::Date, tr("Metadata"), tr("Release date")),
        variableEntry(MetaData::Year, tr("Metadata"), tr("Release year")),
        variableEntry(MetaData::FileSize, tr("Metadata"), tr("File size in bytes")),
        variableEntry(MetaData::FileSizeNatural, tr("Metadata"), tr("Human readable file size")),
        variableEntry(MetaData::Bitrate, tr("Metadata"), tr("Track bitrate")),
        variableEntry(MetaData::SampleRate, tr("Metadata"), tr("Sample rate")),
        variableEntry(MetaData::BitDepth, tr("Metadata"), tr("Bit depth")),
        variableEntry(MetaData::FirstPlayed, tr("Metadata"), tr("First played timestamp")),
        variableEntry(MetaData::LastPlayed, tr("Metadata"), tr("Last played timestamp")),
        variableEntry(MetaData::PlayCount, tr("Metadata"), tr("Play count")),
        variableEntry(MetaData::Rating, tr("Metadata"), tr("Numeric rating")),
        variableEntry(MetaData::RatingStars, tr("Metadata"), tr("Rating shown as stars")),
        variableEntry(MetaData::RatingEditor, tr("Metadata"), tr("Rating editor representation")),
        variableEntry(MetaData::Codec, tr("Metadata"), tr("Codec name")),
        variableEntry(MetaData::CodecProfile, tr("Metadata"), tr("Codec profile")),
        variableEntry(MetaData::Tool, tr("Metadata"), tr("Encoding tool")),
        variableEntry(MetaData::TagType, tr("Metadata"), tr("Tag type list")),
        variableEntry(MetaData::Encoding, tr("Metadata"), tr("Encoding description")),
        variableEntry(MetaData::Channels, tr("Metadata"), tr("Channel layout")),
        variableEntry(MetaData::AddedTime, tr("Metadata"), tr("Library added timestamp")),
        variableEntry(MetaData::LastModified, tr("Metadata"), tr("Last modified timestamp")),
        variableEntry(MetaData::FilePath, tr("Metadata"), tr("Full file path")),
        variableEntry(MetaData::RelativePath, tr("Metadata"), tr("Path relative to the library root")),
        variableEntry(MetaData::FileName, tr("Metadata"), tr("Filename without extension")),
        variableEntry(MetaData::Extension, tr("Metadata"), tr("File extension")),
        variableEntry(MetaData::FileNameWithExt, tr("Metadata"), tr("Filename including extension")),
        variableEntry(MetaData::Directory, tr("Metadata"), tr("Containing directory name")),
        variableEntry(MetaData::Path, tr("Metadata"), tr("Containing directory path")),
        variableEntry(MetaData::Subsong, tr("Metadata"), tr("Subsong index")),
        variableEntry(MetaData::RGTrackGain, tr("Metadata"), tr("ReplayGain track gain")),
        variableEntry(MetaData::RGTrackPeak, tr("Metadata"), tr("ReplayGain track peak")),
        variableEntry(MetaData::RGTrackPeakDB, tr("Metadata"), tr("ReplayGain track peak in dB")),
        variableEntry(MetaData::RGAlbumGain, tr("Metadata"), tr("ReplayGain album gain")),
        variableEntry(MetaData::RGAlbumPeak, tr("Metadata"), tr("ReplayGain album peak")),
        variableEntry(MetaData::RGAlbumPeakDB, tr("Metadata"), tr("ReplayGain album peak in dB")),
        variableEntry("trackcount", tr("Playlist"), tr("Number of tracks in the list")),
        variableEntry("playtime", tr("Playlist"), tr("Combined duration of the track list")),
        variableEntry("playlist_duration", tr("Playlist"), tr("Alias for total playlist duration")),
        variableEntry("playlist_elapsed", tr("Playback"), tr("Elapsed time within the active playlist")),
        variableEntry("genres", tr("Playlist"), tr("Unique genres across the track list")),
        variableEntry("queue_index", tr("Playback Queue"), tr("First playback queue index for the specified item")),
        variableEntry("queue_indexes", tr("Playback Queue"), tr("Playback queue indexes for the specified item")),
        variableEntry("queue_total", tr("Playback Queue"),
                      tr("Total amount of tracks in the playback queue for queued items")),
        variableEntry("playback_time", tr("Playback"), tr("Current playback position formatted as time")),
        variableEntry("playback_time_s", tr("Playback"), tr("Current playback position in seconds")),
        variableEntry("playback_time_remaining", tr("Playback"), tr("Remaining playback time formatted as time")),
        variableEntry("playback_time_remaining_s", tr("Playback"), tr("Remaining playback time in seconds")),
        variableEntry("isplaying", tr("Playback"), tr("Returns 1 while playback is active")),
        variableEntry("ispaused", tr("Playback"), tr("Returns 1 while playback is paused")),
        variableEntry("libraryname", tr("Library"), tr("Current library name")),
        variableEntry("librarypath", tr("Library"), tr("Current library path")),
        formattingEntry("b", "<b>", tr("Style"), tr("Makes the enclosed text bold"), 4),
        formattingEntry("i", "<i>", tr("Style"), tr("Makes the enclosed text italic"), 4),
        formattingEntry("font", "<font=sans>", tr("Style"), tr("Sets the font family for the enclosed text"), 7),
        formattingEntry("size", "<size=12>", tr("Style"), tr("Sets the font size in points"), 7),
        formattingEntry("sized", "<sized=2>", tr("Style"),
                        tr("Adjusts the current font size by a positive or negative delta"), 8),
        formattingEntry("alpha", "<alpha=180>", tr("Colour"), tr("Sets the text alpha channel from 0 to 255"), 8),
        formattingEntry("rgb", "<rgb=255,0,0>", tr("Colour"),
                        tr("Sets the text colour from red, green and blue components"), 6),
        formattingEntry("rgba", "<rgba=255,0,0,255>", tr("Colour"),
                        tr("Sets the text colour from red, green, blue, and alpha components"), 7),
        formattingEntry("color", "<color=red>", tr("Colour"),
                        tr("Sets the text colour from a named colour or hex code"), 8),
        functionEntry("add", u"$add(x,y,...)"_s, tr("Adds numeric arguments"), tr("Numeric")),
        functionEntry("sub", u"$sub(x,y,...)"_s, tr("Subtracts later values from the first"), tr("Numeric")),
        functionEntry("mul", u"$mul(x,y,...)"_s, tr("Multiplies numeric arguments"), tr("Numeric")),
        functionEntry("div", u"$div(x,y)"_s, tr("Divides the first value by the second"), tr("Numeric")),
        functionEntry("min", u"$min(x,y,...)"_s, tr("Returns the smallest numeric value"), tr("Numeric")),
        functionEntry("max", u"$max(x,y,...)"_s, tr("Returns the largest numeric value"), tr("Numeric")),
        functionEntry("mod", u"$mod(x,y)"_s, tr("Returns the remainder of a division"), tr("Numeric")),
        functionEntry("rand", u"$rand(min,max)"_s, tr("Returns a random number in range"), tr("Numeric")),
        functionEntry("round", u"$round(value)"_s, tr("Rounds a numeric value"), tr("Numeric")),
        functionEntry("num", u"$num(value,length)"_s, tr("Formats a number with leading zeroes"), tr("Numeric")),
        functionEntry("replace", u"$replace(text,from,to,...)"_s, tr("Replaces text fragments"), tr("String")),
        functionEntry("ascii", u"$ascii(text)"_s, tr("Converts text to ASCII"), tr("String")),
        functionEntry("slice", u"$slice(text,start,end)"_s, tr("Returns a slice of text"), tr("String")),
        functionEntry("chop", u"$chop(text,count)"_s, tr("Removes characters from the end"), tr("String")),
        functionEntry("left", u"$left(text,count)"_s, tr("Returns characters from the left"), tr("String")),
        functionEntry("right", u"$right(text,count)"_s, tr("Returns characters from the right"), tr("String")),
        functionEntry("insert", u"$insert(text,insert,pos)"_s, tr("Inserts text at a position"), tr("String")),
        functionEntry("substr", u"$substr(text,start,length)"_s, tr("Returns a substring"), tr("String")),
        functionEntry("strstr", u"$strstr(text,needle)"_s, tr("Finds a substring position"), tr("String")),
        functionEntry("stristr", u"$stristr(text,needle)"_s, tr("Finds a substring position ignoring case"),
                      tr("String")),
        functionEntry("strstrlast", u"$strstrlast(text,needle)"_s, tr("Finds the last substring position"),
                      tr("String")),
        functionEntry("stristrlast", u"$stristrlast(text,needle)"_s,
                      tr("Finds the last substring position ignoring case"), tr("String")),
        functionEntry("split", u"$split(text,sep,index)"_s, tr("Returns one split segment (1-based index)"),
                      tr("String")),
        functionEntry("len", u"$len(text)"_s, tr("Returns the text length"), tr("String")),
        functionEntry("longest", u"$longest(a,b,...)"_s, tr("Returns the longest string"), tr("String")),
        functionEntry("strcmp", u"$strcmp(a,b)"_s, tr("Compares two strings"), tr("String")),
        functionEntry("stricmp", u"$stricmp(a,b)"_s, tr("Compares two strings ignoring case"), tr("String")),
        functionEntry("longer", u"$longer(a,b)"_s, tr("Tests whether one string is longer"), tr("String")),
        functionEntry("sep", u"$sep()"_s, tr("Returns the unit separator character"), tr("String")),
        functionEntry("crlf", u"$crlf()"_s, tr("Returns a newline"), tr("String")),
        functionEntry("tab", u"$tab()"_s, tr("Returns a tab character"), tr("String")),
        functionEntry("swapprefix", u"$swapprefix(text)"_s, tr("Moves leading articles to the end"), tr("String")),
        functionEntry("stripprefix", u"$stripprefix(text)"_s, tr("Removes leading articles"), tr("String")),
        functionEntry("pad", u"$pad(text,length,char)"_s, tr("Pads text on the left"), tr("String")),
        functionEntry("padright", u"$padright(text,length,char)"_s, tr("Pads text on the right"), tr("String")),
        functionEntry("repeat", u"$repeat(text,count)"_s, tr("Repeats text"), tr("String")),
        functionEntry("trim", u"$trim(text)"_s, tr("Trims surrounding whitespace"), tr("String")),
        functionEntry("lower", u"$lower(text)"_s, tr("Converts text to lowercase"), tr("String")),
        functionEntry("upper", u"$upper(text)"_s, tr("Converts text to uppercase"), tr("String")),
        functionEntry("abbr", u"$abbr(text)"_s, tr("Builds an abbreviation"), tr("String")),
        functionEntry("caps", u"$caps(text)"_s, tr("Capitalises words"), tr("String")),
        functionEntry("directory", u"$directory(path,level)"_s, tr("Returns a directory name from a path"), tr("Path")),
        functionEntry("directory_path", u"$directory_path(path,level)"_s, tr("Returns a parent directory path"),
                      tr("Path")),
        functionEntry("elide_end", u"$elide_end(text,width)"_s, tr("Elides text at the end"), tr("String")),
        functionEntry("elide_mid", u"$elide_mid(text,width)"_s, tr("Elides text in the middle"), tr("String")),
        functionEntry("ext", u"$ext(path)"_s, tr("Returns a file extension"), tr("Path")),
        functionEntry("filename", u"$filename(path)"_s, tr("Returns a filename without extension"), tr("Path")),
        functionEntry("progress", u"$progress(pos,total,length)"_s, tr("Builds a text progress bar"),
                      tr("Utility Functions")),
        functionEntry("progress2", u"$progress2(pos,total,length)"_s, tr("Builds an alternate text progress bar"),
                      tr("Utility Functions")),
        functionEntry("doclink", u"$doclink(label,url)"_s, tr("Builds a clickable document or web link"),
                      tr("Utility Functions")),
        functionEntry("cmdlink", u"$cmdlink(label,id)"_s, tr("Builds a clickable link to a fooyin command"),
                      tr("Utility Functions")),
        functionEntry("urlencode", u"$urlencode(text)"_s, tr("Percent-encodes text for use in URLs"), tr("String")),
        functionEntry("timems", u"$timems(milliseconds)"_s, tr("Formats milliseconds as time"), tr("Numeric")),
        functionEntry("get", u"$get(name)"_s, tr("Returns the value stored in a script variable"), tr("Variable")),
        functionEntry("put", u"$put(name,value)"_s, tr("Stores a script variable and returns the value"),
                      tr("Variable")),
        functionEntry("puts", u"$puts(name,value)"_s, tr("Stores a script variable and returns nothing"),
                      tr("Variable")),
        functionEntry("if", u"$if(condition,then[,else])"_s, tr("Returns then when condition is true"),
                      tr("Conditional")),
        functionEntry("if2", u"$if2(value,fallback)"_s, tr("Returns the first non-empty value"), tr("Conditional")),
        functionEntry("if3", u"$if3(a1,a2,...,aN,else)"_s,
                      tr("Returns the first true value from a list, or else when none match"), tr("Conditional")),
        functionEntry("ifgreater", u"$ifgreater(x,y,then,else)"_s, tr("Compares numeric values"), tr("Conditional")),
        functionEntry("iflonger", u"$iflonger(text,length,then,else)"_s,
                      tr("Checks whether text is longer than a limit"), tr("Conditional")),
        functionEntry("ifequal", u"$ifequal(x,y,then,else)"_s, tr("Checks whether two numeric values are equal"),
                      tr("Conditional")),
        functionEntry("meta", u"$meta(field)"_s, tr("Looks up a raw tag field by name"), tr("Lookup")),
        functionEntry("info", u"$info(field)"_s, tr("Looks up technical track information"), tr("Lookup")),
    };

    static const auto CommandEntries = [] {
        const auto& aliases = ScriptCommandHandler::scriptCommandAliases();

        std::vector<ScriptReferenceEntry> entries;
        entries.reserve(aliases.size());

        for(const auto& alias : aliases) {
            entries.emplace_back(commandAliasEntry(alias));
        }

        return entries;
    }();

    static const auto AllEntries = [] {
        std::vector<ScriptReferenceEntry> entries;
        entries.reserve(Entries.size() + CommandEntries.size());
        entries.insert(entries.end(), Entries.cbegin(), Entries.cend());
        entries.insert(entries.end(), CommandEntries.cbegin(), CommandEntries.cend());
        return entries;
    }();

    return AllEntries;
}
} // namespace Fooyin
