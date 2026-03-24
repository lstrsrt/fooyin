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

#include <QFile>
#include <QIcon>
#include <QPixmap>
#include <QPixmapCache>
#include <gui/iconloader.h>

using namespace Qt::StringLiterals;

constexpr auto DefaultIconSize = 20;

namespace {
struct IconThemeState
{
    QString primaryTheme;
    QString fallbackTheme;
};

IconThemeState& iconThemeState()
{
    static IconThemeState state;
    return state;
}

std::pair<QString, QString> currentThemes()
{
    auto& state = iconThemeState();
    return {state.primaryTheme, state.fallbackTheme};
}

QIcon iconFromResourceTheme(const QString& theme, const QString& iconName)
{
    if(theme.isEmpty()) {
        return {};
    }

    const QString iconPath = u":/icons/%1/scalable/actions/%2.svg"_s.arg(theme, iconName);
    const QIcon icon(iconPath);
    return icon.isNull() ? QIcon{} : icon;
}

class IconLoader
{
public:
    IconLoader() = delete;

    static bool setThemeOverrides(const QString& primaryTheme, const QString& fallbackTheme)
    {
        auto& state = iconThemeState();

        if(state.primaryTheme == primaryTheme && state.fallbackTheme == fallbackTheme) {
            return false;
        }

        state.primaryTheme  = primaryTheme;
        state.fallbackTheme = fallbackTheme;
        return true;
    }

    [[nodiscard]] static QIcon icon(const QString& iconName)
    {
        const auto [primaryTheme, fallbackTheme] = currentThemes();

        if(const QIcon primaryIcon = iconFromResourceTheme(primaryTheme, iconName); !primaryIcon.isNull()) {
            return primaryIcon;
        }

        if(const QIcon systemIcon = QIcon::fromTheme(iconName); !systemIcon.isNull()) {
            return systemIcon;
        }

        return iconFromResourceTheme(fallbackTheme, iconName);
    }

    [[nodiscard]] static QPixmap pixmap(const QString& iconName, const QSize& size)
    {
        const auto [primaryTheme, fallbackTheme] = currentThemes();
        const QString key
            = u"ThemeIcon|%1|%2|%3|%4x%5"_s.arg(iconName, primaryTheme, fallbackTheme).arg(size.width(), size.height());

        QPixmap pixmap;
        if(QPixmapCache::find(key, &pixmap)) {
            return pixmap;
        }

        pixmap = icon(iconName).pixmap(size);
        QPixmapCache::insert(key, pixmap);
        return pixmap;
    }
};
} // namespace

namespace Fooyin::Gui {
bool setThemeIconOverrides(const QString& primaryTheme, const QString& fallbackTheme)
{
    return IconLoader::setThemeOverrides(primaryTheme, fallbackTheme);
}

QIcon iconFromTheme(const QString& icon)
{
    return IconLoader::icon(icon);
}

QIcon iconFromTheme(const char* icon)
{
    return iconFromTheme(QString::fromLatin1(icon));
}

QPixmap pixmapFromTheme(const char* icon)
{
    return pixmapFromTheme(icon, {DefaultIconSize, DefaultIconSize});
}

QPixmap pixmapFromTheme(const char* icon, const QSize& size)
{
    return IconLoader::pixmap(QString::fromLatin1(icon), size);
}
} // namespace Fooyin::Gui
