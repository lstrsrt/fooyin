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

#pragma once

#include <gui/widgetcontainer.h>
#include <gui/widgets/editabletabwidget.h>

namespace Fooyin {
class SettingsManager;
class WidgetProvider;

class TabStackWidget : public WidgetContainer
{
    Q_OBJECT

public:
    TabStackWidget(WidgetProvider* widgetProvider, SettingsManager* settings, QWidget* parent = nullptr);

    [[nodiscard]] QString name() const override;
    [[nodiscard]] QString layoutName() const override;
    void saveLayoutData(QJsonObject& layout) override;
    void loadLayoutData(const QJsonObject& layout) override;

    [[nodiscard]] Qt::Orientation orientation() const override;

    [[nodiscard]] bool canAddWidget() const override;
    [[nodiscard]] bool canMoveWidget(int index, int newIndex) const override;
    [[nodiscard]] int widgetIndex(const Id& id) const override;
    [[nodiscard]] FyWidget* widgetAtId(const Id& id) const override;
    [[nodiscard]] FyWidget* widgetAtIndex(int index) const override;
    [[nodiscard]] int widgetCount() const override;
    [[nodiscard]] WidgetList widgets() const override;

    int addWidget(FyWidget* widget) override;
    void insertWidget(int index, FyWidget* widget) override;
    void removeWidget(int index) override;
    void replaceWidget(int index, FyWidget* newWidget) override;
    void moveWidget(int index, int newIndex) override;

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    [[nodiscard]] int indexOfWidget(FyWidget* widget) const;
    [[nodiscard]] int indexOfWidget(const Id& id) const;
    void changeTabPosition(QTabWidget::TabPosition position) const;

    WidgetList m_widgets;
    EditableTabWidget* m_tabs;
};
} // namespace Fooyin
