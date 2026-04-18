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

#include <core/scripting/scripttypes.h>

#include <optional>
#include <vector>

namespace Fooyin::TagEditor {
struct FillPatternError
{
    int position{-1};
    QString message;
};

class FillPattern
{
public:
    struct Token
    {
        enum class Type : uint8_t
        {
            Literal = 0,
            Capture,
            Ignore,
        };

        Type type{Type::Literal};
        int position{-1};
        QString value;
    };

    struct Match
    {
        std::vector<std::pair<QString, QString>> values;
    };

    [[nodiscard]] static FillPattern parse(const QString& pattern);

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] const std::vector<FillPatternError>& errors() const;
    [[nodiscard]] const std::vector<QString>& captureFields() const;
    [[nodiscard]] std::optional<Match> match(const QString& source) const;

private:
    QString m_pattern;
    std::vector<Token> m_tokens;
    std::vector<FillPatternError> m_errors;
    std::vector<QString> m_captureFields;
};

[[nodiscard]] bool isWritableFillField(const QString& field);
[[nodiscard]] bool isMultiValueFillField(const QString& field);
[[nodiscard]] ScriptFieldValue fillFieldValue(const QString& field, const QString& value);
} // namespace Fooyin::TagEditor
