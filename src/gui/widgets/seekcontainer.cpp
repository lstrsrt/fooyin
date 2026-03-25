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

#include <gui/widgets/seekcontainer.h>

#include <core/player/playercontroller.h>
#include <gui/widgets/clickablelabel.h>
#include <utils/stringutils.h>

#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>

using namespace Qt::StringLiterals;

namespace Fooyin {
class SeekContainerPrivate
{
public:
    SeekContainerPrivate(SeekContainer* self, PlayerController* playerController);

    void reset();
    void updateLabelWidth(ClickableLabel* label) const;
    void updateLabelWidths() const;

    void trackChanged(const Track& track);
    void stateChanged(Player::PlayState state);
    void updateLabels(uint64_t time) const;

    SeekContainer* m_self;

    PlayerController* m_playerController;

    QHBoxLayout* m_layout;
    ClickableLabel* m_elapsed;
    ClickableLabel* m_total;
    uint64_t m_max{0};
    bool m_elapsedTotal{false};
};

SeekContainerPrivate::SeekContainerPrivate(SeekContainer* self, PlayerController* playerController)
    : m_self{self}
    , m_playerController{playerController}
    , m_layout{new QHBoxLayout(m_self)}
    , m_elapsed{new ClickableLabel(Utils::msToString(0), m_self)}
    , m_total{new ClickableLabel(Utils::msToString(0), m_self)}
{
    m_layout->setContentsMargins({});

    m_layout->addWidget(m_elapsed, 0, Qt::AlignVCenter | Qt::AlignLeft);
    m_layout->addWidget(m_total, 0, Qt::AlignVCenter | Qt::AlignLeft);

    trackChanged(m_playerController->currentTrack());
}

void SeekContainerPrivate::reset()
{
    m_max = 0;
    updateLabels(m_max);
}

void SeekContainerPrivate::updateLabelWidth(ClickableLabel* label) const
{
    if(!label) {
        return;
    }

    const QFontMetrics fm{label->fontMetrics()};
    const auto margins = label->contentsMargins();
    const int width
        = fm.horizontalAdvance(label->text()) + margins.left() + margins.right() + (2 * label->margin()) + 2;
    label->setFixedWidth(width);
}

void SeekContainerPrivate::updateLabelWidths() const
{
    updateLabelWidth(m_elapsed);
    updateLabelWidth(m_total);
}

void SeekContainerPrivate::trackChanged(const Track& track)
{
    if(track.isValid()) {
        m_max = track.duration();
        updateLabels(0);
    }
}

void SeekContainerPrivate::stateChanged(Player::PlayState state)
{
    switch(state) {
        case(Player::PlayState::Paused):
            break;
        case(Player::PlayState::Stopped):
            reset();
            break;
        case(Player::PlayState::Playing): {
            if(m_max == 0) {
                trackChanged(m_playerController->currentTrack());
            }
            break;
        }
    }
}

void SeekContainerPrivate::updateLabels(uint64_t time) const
{
    const auto prevElapsedSize = m_elapsed->text().size();
    const auto prevTotalSize   = m_total->text().size();

    const auto elapsed = Utils::msToString(time);
    m_elapsed->setText(elapsed);

    QString total;
    if(m_elapsedTotal) {
        const int remaining = std::max(0, static_cast<int>(m_max - time));
        total               = u"-"_s + Utils::msToString(remaining);
    }
    else {
        total = Utils::msToString(m_max);
    }
    m_total->setText(total);

    if(elapsed.size() != prevElapsedSize || total.size() != prevTotalSize) {
        updateLabelWidths();
    }
}

SeekContainer::SeekContainer(PlayerController* playerController, QWidget* parent)
    : QWidget{parent}
    , p{std::make_unique<SeekContainerPrivate>(this, playerController)}
{
    QObject::connect(p->m_elapsed, &ClickableLabel::clicked, this, &SeekContainer::elapsedClicked);
    QObject::connect(p->m_total, &ClickableLabel::clicked, this, &SeekContainer::totalClicked);

    QObject::connect(p->m_playerController, &PlayerController::playStateChanged, this,
                     [this](Player::PlayState state) { p->stateChanged(state); });
    QObject::connect(p->m_playerController, &PlayerController::currentTrackChanged, this,
                     [this](const Track& track) { p->trackChanged(track); });
    QObject::connect(p->m_playerController, &PlayerController::positionChanged, this,
                     [this](uint64_t pos) { p->updateLabels(pos); });

    QObject::connect(this, &SeekContainer::totalClicked, this, [this]() { setElapsedTotal(!elapsedTotal()); });
}

SeekContainer::~SeekContainer() = default;

void SeekContainer::insertWidget(int index, QWidget* widget)
{
    p->m_layout->insertWidget(index, widget, Qt::AlignVCenter);
}

bool SeekContainer::labelsEnabled() const
{
    return !p->m_elapsed->isHidden() && !p->m_total->isHidden();
}

bool SeekContainer::elapsedTotal() const
{
    return p->m_elapsedTotal;
}

void SeekContainer::setLabelsEnabled(bool enabled)
{
    p->m_elapsed->setHidden(!enabled);
    p->m_total->setHidden(!enabled);
}

void SeekContainer::setElapsedTotal(bool enabled)
{
    p->m_elapsedTotal = enabled;
    p->updateLabels(p->m_playerController->currentPosition());
}

void SeekContainer::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);

    switch(event->type()) {
        case QEvent::FontChange:
        case QEvent::StyleChange:
        case QEvent::PaletteChange:
            p->updateLabelWidths();
            break;
        default:
            break;
    }
}
} // namespace Fooyin
