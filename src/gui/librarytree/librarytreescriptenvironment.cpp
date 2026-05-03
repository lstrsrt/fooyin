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

#include "librarytreescriptenvironment.h"

#include <core/scripting/scriptproviders.h>

#include <QCoreApplication>

using namespace Qt::StringLiterals;

namespace {
Fooyin::ScriptResult nodeVariable(const Fooyin::ScriptContext& context, const QString& name)
{
    const auto* environment = static_cast<const Fooyin::LibraryTreeScriptEnvironment*>(context.environment);
    if(environment == nullptr) {
        return {};
    }

    switch(environment->nodeVariablePolicy()) {
        case Fooyin::LibraryTreeNodePolicy::Placeholder:
            return {.value = {}, .cond = true};
        case Fooyin::LibraryTreeNodePolicy::Value: {
            const int value = name.compare(u"TRACKCOUNT"_s, Qt::CaseInsensitive) == 0 ? environment->nodeTrackCount()
                                                                                      : environment->nodeChildCount();
            return {.value = QString::number(value), .cond = true};
        }
        case Fooyin::LibraryTreeNodePolicy::Unresolved:
            break;
    }

    return {.value = u"%%1%"_s.arg(name.toLower()), .cond = true};
}
} // namespace

namespace Fooyin {
LibraryTreeScriptEnvironment::LibraryTreeScriptEnvironment(const LibraryManager* libraryManager)
    : LibraryScriptEnvironment{libraryManager}
{ }

void LibraryTreeScriptEnvironment::setNodeVariablePolicy(LibraryTreeNodePolicy policy)
{
    m_nodeVariablePolicy = policy;
}

void LibraryTreeScriptEnvironment::setNodeCounts(int trackCount, int childCount)
{
    m_nodeTrackCount = trackCount;
    m_nodeChildCount = childCount;
}

LibraryTreeNodePolicy LibraryTreeScriptEnvironment::nodeVariablePolicy() const
{
    return m_nodeVariablePolicy;
}

int LibraryTreeScriptEnvironment::nodeTrackCount() const
{
    return m_nodeTrackCount;
}

int LibraryTreeScriptEnvironment::nodeChildCount() const
{
    return m_nodeChildCount;
}

const ScriptVariableProvider& libraryTreeNodeVariableProvider()
{
    static const StaticScriptVariableProvider Provider{
        makeScriptVariableDescriptor<nodeVariable>(VariableKind::Generic, u"TRACKCOUNT"_s),
        makeScriptVariableDescriptor<nodeVariable>(VariableKind::Generic, u"CHILDCOUNT"_s)};
    return Provider;
}

QString defaultLibraryTreeSummaryTitle()
{
    //: Default Library Tree summary node title. Keep %trackcount% unchanged; it is a script variable.
    return QCoreApplication::translate("Fooyin::LibraryTree", "All Music (%trackcount%)");
}

bool usesLibraryTreeNodeVariables(QStringView text)
{
    return text.contains(u"%trackcount%"_s, Qt::CaseInsensitive)
        || text.contains(u"%childcount%"_s, Qt::CaseInsensitive);
}

bool usesLibraryTreeChildCount(QStringView text)
{
    return text.contains(u"%childcount%"_s, Qt::CaseInsensitive);
}
} // namespace Fooyin
