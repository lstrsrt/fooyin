/*
 * Fooyin
 * Copyright © 2024, Luke Taylor <LukeT1@proton.me>
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

#include <core/engine/audioinput.h>
#include <core/engine/inputplugin.h>
#include <core/plugins/plugin.h>

namespace Fooyin::LibArchive {
class LibArchivePlugin : public QObject,
                         public Plugin,
                         public InputPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.fooyin.fooyin.plugin" FILE "libarchive.json")
    Q_INTERFACES(Fooyin::Plugin Fooyin::InputPlugin)

public:
    [[nodiscard]] QString inputName() const override;
    [[nodiscard]] InputCreator inputCreator() const override;
};
} // namespace Fooyin::LibArchive
