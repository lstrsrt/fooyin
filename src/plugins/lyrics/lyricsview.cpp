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

#include "lyricsview.h"

#include "lyricsmodel.h"

#include <QApplication>
#include <QBrush>
#include <QContextMenuEvent>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QScrollBar>
#include <QWheelEvent>

using namespace Qt::StringLiterals;

namespace Fooyin::Lyrics {
LyricsView::LyricsView(QWidget* parent)
    : QListView{parent}
    , m_displayAlignment{Qt::AlignCenter}
    , m_edgeFadeEnabled{false}
    , m_edgeFadeSizePercent{10}
    , m_leftButtonDown{false}
    , m_dragSeeking{false}
    , m_suppressContextMenu{false}
    , m_suppressNextLeftRelease{false}
{
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionMode(QAbstractItemView::NoSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setFocusPolicy(Qt::NoFocus);
    setUniformItemSizes(false);
    setResizeMode(QListView::Adjust);
    setWrapping(false);
    setFlow(QListView::TopToBottom);
    setFrameShape(QFrame::NoFrame);
    viewport()->setAutoFillBackground(true);

    updateScrollSingleStep();
}

void LyricsView::setDisplayAlignment(Qt::Alignment alignment)
{
    if(std::exchange(m_displayAlignment, alignment) == alignment) {
        return;
    }

    viewport()->update();
}

void LyricsView::setEdgeFadeEnabled(bool enabled)
{
    if(std::exchange(m_edgeFadeEnabled, enabled) == enabled) {
        return;
    }

    viewport()->update();
}

void LyricsView::setEdgeFadeSizePercent(int percent)
{
    percent = std::clamp(percent, 1, 50);
    if(std::exchange(m_edgeFadeSizePercent, percent) == percent) {
        return;
    }

    if(m_edgeFadeEnabled) {
        viewport()->update();
    }
}

void LyricsView::setDisplayMargins(const QMargins& margins)
{
    if(std::exchange(m_displayMargins, margins) == margins) {
        return;
    }

    viewport()->update();
}

void LyricsView::setDisplayString(const QString& string)
{
    m_displayString = string;
    viewport()->update();
}

bool LyricsView::isDragSeeking() const
{
    return m_dragSeeking;
}

void LyricsView::contextMenuEvent(QContextMenuEvent* event)
{
    if(m_suppressContextMenu) {
        m_suppressContextMenu = false;
        event->accept();
        return;
    }

    QListView::contextMenuEvent(event);
}

void LyricsView::paintEvent(QPaintEvent* event)
{
    if(!m_displayString.isEmpty()) {
        QPainter painter{viewport()};
        const QRect textRect = viewport()->rect().adjusted(m_displayMargins.left(), m_displayMargins.top(),
                                                           -m_displayMargins.right(), -m_displayMargins.bottom());
        const auto alignment = (m_displayAlignment & Qt::AlignHorizontal_Mask) | Qt::AlignVCenter | Qt::TextWordWrap;
        painter.drawText(textRect, alignment, m_displayString);
        return;
    }

    QListView::paintEvent(event);

    QPainter painter{viewport()};

    const auto paintSeekLine = [this, &painter]() {
        // TODO: Make configurable
        QPen pen{palette().color(QPalette::Text)};
        pen.setStyle(Qt::DotLine);
        pen.setDashPattern({2.0, 4.0});
        pen.setCosmetic(true);
        pen.setWidth(1);

        painter.setOpacity(0.6);
        painter.setPen(pen);

        const int guideY = seekPosition(m_dragPos).y();
        painter.drawLine(0, guideY, viewport()->width(), guideY);
    };

    if(m_edgeFadeEnabled) {
        paintEdgeFade(painter);
    }

    if(m_dragSeeking && isSeekableIndex(m_dragIndex)) {
        paintSeekLine();
    }
}

void LyricsView::mousePressEvent(QMouseEvent* event)
{
    if(event->button() == Qt::RightButton && m_dragSeeking) {
        clearDragPreview();
        m_leftButtonDown          = false;
        m_suppressContextMenu     = true;
        m_suppressNextLeftRelease = true;
        event->accept();
        return;
    }

    QListView::mousePressEvent(event);

    if(event->button() != Qt::LeftButton || !m_displayString.isEmpty()) {
        event->ignore();
        return;
    }

    const QModelIndex index = seekableIndexAt(event->position().toPoint());
    if(!isSeekableIndex(index)) {
        return;
    }

    m_leftButtonDown = true;
    m_dragSeeking    = false;
    m_pressPos       = event->position().toPoint();
    m_dragPos        = m_pressPos;
    m_dragIndex      = index;
}

void LyricsView::mouseMoveEvent(QMouseEvent* event)
{
    QListView::mouseMoveEvent(event);

    if(!m_leftButtonDown || !(event->buttons() & Qt::LeftButton) || !m_displayString.isEmpty()) {
        return;
    }

    const QPoint pos = event->position().toPoint();
    if(!m_dragSeeking && (m_pressPos - pos).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }

    if(!m_dragSeeking) {
        m_dragSeeking = true;
        emit dragSeekingChanged(true);
    }

    emit userScrolling();
    updateDragPreview(pos);
}

void LyricsView::mouseReleaseEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton && m_suppressNextLeftRelease) {
        m_suppressNextLeftRelease = false;
        event->accept();
        return;
    }

    QListView::mouseReleaseEvent(event);

    if(event->button() != Qt::LeftButton) {
        return;
    }

    const QPoint pos = seekPosition(event->position().toPoint());
    if(m_dragSeeking) {
        updateDragPreview(pos);
        emit lineDragSeekRequested(m_dragIndex, pos);
        clearDragPreview();
        m_leftButtonDown = false;
        return;
    }

    const QModelIndex index = seekableIndexAt(pos);
    emit lineClicked(index, pos);

    m_leftButtonDown = false;
    m_dragIndex      = QPersistentModelIndex{};
}

void LyricsView::resizeEvent(QResizeEvent* event)
{
    QListView::resizeEvent(event);
    emit viewportResized();
}

void LyricsView::scrollContentsBy(const int dx, const int dy)
{
    QListView::scrollContentsBy(dx, dy);

    if(m_edgeFadeEnabled && dy != 0) {
        viewport()->update();
    }
}

void LyricsView::updateGeometries()
{
    QListView::updateGeometries();
    updateScrollSingleStep();
}

void LyricsView::wheelEvent(QWheelEvent* event)
{
    emit userScrolling();
    QListView::wheelEvent(event);
}

void LyricsView::updateScrollSingleStep()
{
    int singleStep = std::max(1, fontMetrics().height());

    if(model()) {
        if(model()->rowCount({}) > 2) {
            const int firstRowHeight = sizeHintForIndex(model()->index(1, 0)).height();
            if(firstRowHeight > 0) {
                singleStep = firstRowHeight;
            }
        }
    }

    // QAbstractItemView derives the scrollbar step from item sizes in ScrollPerPixel mode.
    // The lyrics model's row 0 is synthetic padding for centering, so anchor manual scrolling
    // to the first real lyrics row instead of the padding height
    verticalScrollBar()->setSingleStep(singleStep);
}

QColor LyricsView::backgroundColour() const
{
    if(const auto* model = this->model(); model && model->rowCount({}) > 0) {
        const auto background = model->index(0, 0).data(Qt::BackgroundRole).value<QBrush>();
        if(background.style() != Qt::NoBrush) {
            const QColor colour = background.color();
            if(colour.isValid()) {
                return colour;
            }
        }
    }

    return palette().color(QPalette::Base);
}

int LyricsView::edgeFadeHeight() const
{
    const int viewportHeight = viewport()->height();
    if(viewportHeight <= 1) {
        return 0;
    }

    return std::clamp((viewportHeight * m_edgeFadeSizePercent) / 100, 1, viewportHeight / 2);
}

void LyricsView::paintEdgeFade(QPainter& painter) const
{
    if(!model() || model()->rowCount({}) <= 0) {
        return;
    }

    const int fadeHeight = edgeFadeHeight();
    if(fadeHeight <= 0) {
        return;
    }

    const QRect viewportRect = viewport()->rect();
    if(viewportRect.height() <= fadeHeight) {
        return;
    }

    QColor edgeColour = backgroundColour();
    if(!edgeColour.isValid()) {
        return;
    }

    QColor centreColour{edgeColour};
    centreColour.setAlpha(0);

    const QRect topRect{viewportRect.left(), viewportRect.top(), viewportRect.width(), fadeHeight};
    QLinearGradient topGradient{topRect.topLeft(), topRect.bottomLeft()};
    topGradient.setColorAt(0.0, edgeColour);
    topGradient.setColorAt(1.0, centreColour);
    painter.fillRect(topRect, topGradient);

    const QRect bottomRect{viewportRect.left(), viewportRect.bottom() - fadeHeight + 1, viewportRect.width(),
                           fadeHeight};
    QLinearGradient bottomGradient{bottomRect.topLeft(), bottomRect.bottomLeft()};
    bottomGradient.setColorAt(0.0, centreColour);
    bottomGradient.setColorAt(1.0, edgeColour);
    painter.fillRect(bottomRect, bottomGradient);
}

QModelIndex LyricsView::seekableIndexAt(const QPoint& pos) const
{
    const auto* model = this->model();
    if(!model || model->rowCount() <= 2) {
        return {};
    }

    const QPoint seekPos    = seekPosition(pos);
    const QModelIndex index = indexAt(seekPos);
    if(isSeekableIndex(index)) {
        return index;
    }

    if(index.isValid() && index.data(LyricsModel::IsPaddingRole).toBool()) {
        if(index.row() == 0) {
            return model->index(1, 0);
        }
        if(index.row() == model->rowCount({}) - 1) {
            return model->index(model->rowCount({}) - 2, 0);
        }
    }

    if(seekPos.y() < viewport()->rect().center().y()) {
        return model->index(1, 0);
    }

    return model->index(model->rowCount() - 2, 0);
}

QPoint LyricsView::seekPosition(const QPoint& pos) const
{
    QPoint seekPos{pos};
    const QRect viewportRect = viewport()->rect();
    if(viewportRect.isEmpty()) {
        return seekPos;
    }

    seekPos.setX(std::clamp(seekPos.x(), viewportRect.left(), viewportRect.right()));
    seekPos.setY(std::clamp(seekPos.y(), viewportRect.top(), viewportRect.bottom()));
    return seekPos;
}

bool LyricsView::isSeekableIndex(const QModelIndex& index) const
{
    return index.isValid() && !index.data(LyricsModel::IsPaddingRole).toBool();
}

void LyricsView::updateDragPreview(const QPoint& pos)
{
    const QPoint seekPos    = seekPosition(pos);
    const QModelIndex index = seekableIndexAt(seekPos);
    if(!isSeekableIndex(index)) {
        return;
    }

    m_dragPos   = seekPos;
    m_dragIndex = index;

    viewport()->update();
}

void LyricsView::clearDragPreview()
{
    const bool wasDragSeeking = std::exchange(m_dragSeeking, false);

    m_dragIndex = QPersistentModelIndex{};
    m_dragPos   = {};

    viewport()->update();

    if(wasDragSeeking) {
        emit dragSeekingChanged(false);
    }
}
} // namespace Fooyin::Lyrics
