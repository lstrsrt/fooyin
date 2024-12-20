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

#include "gmeplugin.h"

#include "gmeinput.h"
#include "gmesettings.h"

using namespace Qt::StringLiterals;

namespace Fooyin::Gme {
QString GmePlugin::inputName() const
{
    return u"Game Music Emu"_s;
}

InputCreator GmePlugin::inputCreator() const
{
    InputCreator creator;
    creator.decoder = []() {
        return std::make_unique<GmeDecoder>();
    };
    creator.reader = []() {
        return std::make_unique<GmeReader>();
    };
    return creator;
}

bool GmePlugin::hasSettings() const
{
    return true;
}

void GmePlugin::showSettings(QWidget* parent)
{
    auto* dialog = new GmeSettings(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}
} // namespace Fooyin::Gme

#include "moc_gmeplugin.cpp"
