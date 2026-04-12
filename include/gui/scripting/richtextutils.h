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

#pragma once

#include "fygui_export.h"

#include <gui/scripting/richtext.h>

#include <QColor>
#include <QFont>
#include <QString>

namespace Fooyin {
struct RichTextMetrics
{
    int width{0};
    int height{0};
    int firstLineWidth{0};
    int firstLineHeight{0};
    int extraLineHeight{0};
};

FYGUI_EXPORT QColor resolvedRichTextColour(const RichFormatting& formatting, const QColor& baseColour,
                                           const QColor& linkColour = {});
FYGUI_EXPORT QFont resolvedRichTextFont(const RichFormatting& formatting, const QFont& baseFont);
FYGUI_EXPORT QString richTextToHtml(const RichText& richText, const QColor& linkColour = {});
FYGUI_EXPORT bool richTextHasLineBreaks(const RichText& richText);
FYGUI_EXPORT int richTextExtraLineHeight(const RichText& richText, const QFont& baseFont = {});
FYGUI_EXPORT std::vector<RichText> splitRichTextLines(const RichText& richText);
FYGUI_EXPORT RichTextMetrics measureRichText(const RichText& richText, const QFont& baseFont = {});
FYGUI_EXPORT RichText trimRichText(RichText richText);
} // namespace Fooyin
