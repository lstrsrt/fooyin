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

#include "titletooltipgroupbox.h"

#include <QEvent>
#include <QHelpEvent>
#include <QStyleOptionGroupBox>
#include <QToolTip>

namespace Fooyin {
void TitleToolTipGroupBox::setTitleToolTip(QString toolTip)
{
    m_titleToolTip = std::move(toolTip);
}

bool TitleToolTipGroupBox::event(QEvent* event)
{
    if(event->type() == QEvent::ToolTip && !m_titleToolTip.isEmpty()) {
        auto* helpEvent = static_cast<QHelpEvent*>(event);

        QStyleOptionGroupBox option;
        initStyleOption(&option);

        const auto control = style()->hitTestComplexControl(QStyle::CC_GroupBox, &option, helpEvent->pos(), this);

        if(control == QStyle::SC_GroupBoxLabel || control == QStyle::SC_GroupBoxCheckBox) {
            QToolTip::showText(helpEvent->globalPos(), m_titleToolTip, this);
        }
        else {
            QToolTip::hideText();
        }

        return true;
    }

    return QGroupBox::event(event);
}
} // namespace Fooyin
