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

#include <QListView>
#include <QMargins>
#include <QPersistentModelIndex>

namespace Fooyin {
class SettingsManager;

namespace Lyrics {
class LyricsDelegate;
class LyricsModel;

class LyricsView : public QListView
{
    Q_OBJECT

public:
    explicit LyricsView(QWidget* parent = nullptr);

    void setDisplayAlignment(Qt::Alignment alignment);
    void setDisplayMargins(const QMargins& margins);
    void setDisplayString(const QString& string);

    [[nodiscard]] bool isDragSeeking() const;

signals:
    void dragSeekingChanged(bool active);
    void lineClicked(const QModelIndex& index, const QPoint& pos);
    void lineDragSeekRequested(const QModelIndex& index, const QPoint& pos);
    void userScrolling();
    void viewportResized();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void updateGeometries() override;
    void wheelEvent(QWheelEvent* event) override;

private:
    [[nodiscard]] QModelIndex seekableIndexAt(const QPoint& pos) const;
    [[nodiscard]] bool isSeekableIndex(const QModelIndex& index) const;

    void updateScrollSingleStep();
    void updateDragPreview(const QPoint& pos);
    void clearDragPreview();

    QString m_displayString;
    Qt::Alignment m_displayAlignment;
    QMargins m_displayMargins;

    QPoint m_pressPos;
    QPoint m_dragPos;
    QPersistentModelIndex m_dragIndex;
    bool m_leftButtonDown;
    bool m_dragSeeking;
    bool m_suppressContextMenu;
    bool m_suppressNextLeftRelease;
};
} // namespace Lyrics
} // namespace Fooyin
