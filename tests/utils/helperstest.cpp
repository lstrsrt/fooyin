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

#include <utils/helpers.h>

#include <gtest/gtest.h>

#include <QStringList>

using namespace Qt::StringLiterals;

namespace Fooyin::Testing {
TEST(UtilsHelpersTest, FindUniqueStringUsesFirstAvailableSuffix)
{
    const QStringList names{u"Playlist"_s, u"Playlist (2)"_s};

    const QString uniqueName = Utils::findUniqueString(u"Playlist"_s, names, [](const auto& name) { return name; });

    EXPECT_EQ(uniqueName, u"Playlist (1)"_s);
}

TEST(UtilsHelpersTest, FindUniqueStringTreatsSuffixSpacingVariantsAsExisting)
{
    const QStringList names{u"Playlist"_s, u"Playlist(1)"_s, u"Playlist   (2)  "_s};

    const QString uniqueName = Utils::findUniqueString(u"Playlist"_s, names, [](const auto& name) { return name; });

    EXPECT_EQ(uniqueName, u"Playlist (3)"_s);
}
} // namespace Fooyin::Testing
