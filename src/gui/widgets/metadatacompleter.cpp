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

#include <gui/widgets/metadatacompleter.h>

#include <core/constants.h>
#include <core/track.h>
#include <core/trackmetadatastore.h>

#include <QAbstractItemView>
#include <QCompleter>
#include <QFontMetrics>
#include <QLineEdit>
#include <QScreen>
#include <QStringListModel>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace {
class TokenCompleter : public QCompleter
{
public:
    using QCompleter::QCompleter;

    void setMultiValue(bool multivalue)
    {
        m_multivalue = multivalue;
    }

    [[nodiscard]] QString pathFromIndex(const QModelIndex& index) const override
    {
        QString completion = QCompleter::pathFromIndex(index);

        if(!m_multivalue) {
            return completion;
        }

        const auto* lineEdit = qobject_cast<const QLineEdit*>(widget());
        if(!lineEdit) {
            return completion;
        }

        const QString text       = lineEdit->text();
        const qsizetype splitPos = text.lastIndexOf(';'_L1);
        if(splitPos < 0) {
            return completion;
        }

        QString prefix = text.first(splitPos + 1);
        if(!prefix.endsWith(' '_L1)) {
            prefix += u' ';
        }

        return prefix + completion;
    }

    [[nodiscard]] QStringList splitPath(const QString& path) const override
    {
        if(!m_multivalue) {
            return QCompleter::splitPath(path);
        }

        const qsizetype splitPos = path.lastIndexOf(';'_L1);
        return {path.sliced(splitPos >= 0 ? splitPos + 1 : 0).trimmed()};
    }

private:
    bool m_multivalue{false};
};

using Domain = Fooyin::StringPool::Domain;

int popupWidthForValues(QLineEdit* editor, const QStringList& values)
{
    static constexpr int ContentPadding = 48;
    static constexpr int ScreenMargin   = 40;
    static constexpr int MaxPopupWidth  = 700;

    const QFontMetrics fm{editor->font()};

    int contentWidth{0};
    for(const QString& value : values) {
        contentWidth = std::max(contentWidth, fm.horizontalAdvance(value));
    }

    int maxWidth{MaxPopupWidth};
    if(const QScreen* screen = editor->screen()) {
        maxWidth = std::min(maxWidth, screen->availableGeometry().width() - ScreenMargin);
    }

    const int desiredWidth = std::max(editor->width(), contentWidth + ContentPadding);
    return std::max(editor->width(), std::min(desiredWidth, maxWidth));
}
} // namespace

namespace Fooyin::Gui {
std::optional<StringPool::Domain> metadataCompletionDomain(const QString& field)
{
    const QString normalisedField = field.trimmed().toUpper();

    if(normalisedField == QLatin1String{Constants::MetaData::Artist}) {
        return Domain::Artist;
    }
    if(normalisedField == QLatin1String{Constants::MetaData::AlbumArtist}) {
        return Domain::AlbumArtist;
    }
    if(normalisedField == QLatin1String{Constants::MetaData::Album}) {
        return Domain::Album;
    }
    if(normalisedField == QLatin1String{Constants::MetaData::Genre}) {
        return Domain::Genre;
    }
    if(normalisedField == QLatin1String{Constants::MetaData::Composer}) {
        return Domain::Composer;
    }
    if(normalisedField == QLatin1String{Constants::MetaData::Performer}) {
        return Domain::Performer;
    }
    if(normalisedField == QLatin1String{Constants::MetaData::Codec}) {
        return Domain::Codec;
    }
    if(normalisedField == QLatin1String{Constants::MetaData::Encoding}) {
        return Domain::Encoding;
    }

    return {};
}

void setMetadataCompleter(QLineEdit* editor, const QStringList& values, bool multivalue)
{
    if(!editor || values.empty()) {
        return;
    }

    auto* model = new QStringListModel(values, editor);

    auto* completer = new TokenCompleter(model, editor);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setFilterMode(Qt::MatchContains);
    completer->setMultiValue(multivalue);
    completer->popup()->setMinimumWidth(popupWidthForValues(editor, values));

    editor->setCompleter(completer);
}

bool setMetadataCompleter(QLineEdit* editor, const QString& field, const TrackMetadataStore& store)
{
    if(!editor) {
        return false;
    }

    const auto domain = metadataCompletionDomain(field);
    if(!domain.has_value()) {
        return false;
    }

    QStringList values = store.values(*domain);
    values.sort(Qt::CaseInsensitive);
    setMetadataCompleter(editor, values, Track::isMultiValueTag(field));
    return true;
}
} // namespace Fooyin::Gui
