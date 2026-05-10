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

#include "advancedpage.h"

#include "advanceditemdelegate.h"
#include "advancedsettingsmodel.h"

#include <gui/guiconstants.h>
#include <utils/modelutils.h>
#include <utils/settings/advancedsettingsregistry.h>

#include <QDataStream>
#include <QGridLayout>
#include <QHeaderView>
#include <QIODevice>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QTreeView>

using namespace Qt::StringLiterals;

namespace Fooyin {
class AdvancedPageWidget : public SettingsPageWidget
{
    Q_OBJECT

public:
    explicit AdvancedPageWidget(AdvancedSettingsRegistry* registry);

    void load() override;
    void apply() override;
    void finish() override;
    void reset() override;

    [[nodiscard]] QByteArray saveState() const override;
    void restoreState(const QByteArray& state) override;

    [[nodiscard]] QString validationError() const override;

private:
    void applyFilter(const QString& filter);
    [[nodiscard]] QString sourceIndexPath(const QModelIndex& index) const;

    QLineEdit* m_filter;
    AdvancedSettingsModel* m_model;
    QSortFilterProxyModel* m_proxyModel;
    QTreeView* m_tree;
    QStringList m_expandedState;
    bool m_hasExpandedState{false};
};

AdvancedPageWidget::AdvancedPageWidget(AdvancedSettingsRegistry* registry)
    : m_filter{new QLineEdit(this)}
    , m_model{new AdvancedSettingsModel(registry, this)}
    , m_proxyModel{new QSortFilterProxyModel(this)}
    , m_tree{new QTreeView(this)}
{
    m_filter->setPlaceholderText(tr("Filter"));

    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setRecursiveFilteringEnabled(true);
    m_proxyModel->setAutoAcceptChildRows(true);

    m_tree->setModel(m_proxyModel);
    m_tree->setItemDelegate(new AdvancedItemDelegate(m_tree));
    m_tree->setAlternatingRowColors(true);
    m_tree->setRootIsDecorated(true);
    m_tree->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
                            | QAbstractItemView::SelectedClicked);
    m_tree->header()->hide();

    auto* layout = new QGridLayout(this);
    layout->addWidget(m_filter, 0, 0);
    layout->addWidget(m_tree, 1, 0);

    QObject::connect(m_filter, &QLineEdit::textChanged, this, &AdvancedPageWidget::applyFilter);
}

void AdvancedPageWidget::load()
{
    m_model->load();

    const auto keyForIndex = [this](const QModelIndex& index) {
        return sourceIndexPath(index);
    };

    if(!m_hasExpandedState) {
        m_tree->expandAll();
        m_expandedState    = Utils::saveExpansionState(m_tree, keyForIndex);
        m_hasExpandedState = true;
    }
    else {
        Utils::restoreExpansionState(m_tree, m_expandedState, keyForIndex);
    }
}

void AdvancedPageWidget::apply()
{
    m_model->apply();
}

void AdvancedPageWidget::finish()
{
    m_expandedState = Utils::updateExpansionState(m_tree, m_expandedState,
                                                  [this](const QModelIndex& index) { return sourceIndexPath(index); });
    m_hasExpandedState = true;
}

void AdvancedPageWidget::reset()
{
    m_model->reset();
}

QByteArray AdvancedPageWidget::saveState() const
{
    QByteArray stateData;
    QDataStream stream{&stateData, QIODeviceBase::WriteOnly};
    stream.setVersion(QDataStream::Qt_6_0);

    stream << m_hasExpandedState;
    stream << m_expandedState;

    return qCompress(stateData, 9);
}

void AdvancedPageWidget::restoreState(const QByteArray& state)
{
    if(state.isEmpty()) {
        return;
    }

    QByteArray stateData = qUncompress(state);
    QDataStream stream{&stateData, QIODeviceBase::ReadOnly};
    stream.setVersion(QDataStream::Qt_6_0);

    stream >> m_hasExpandedState;
    stream >> m_expandedState;

    if(m_hasExpandedState && m_model->rowCount({}) > 0) {
        Utils::restoreExpansionState(m_tree, m_expandedState,
                                     [this](const QModelIndex& index) { return sourceIndexPath(index); });
    }
}

QString AdvancedPageWidget::validationError() const
{
    return m_model->validationError();
}

void AdvancedPageWidget::applyFilter(const QString& filter)
{
    const auto keyForIndex = [this](const QModelIndex& index) {
        return sourceIndexPath(index);
    };

    m_expandedState    = Utils::updateExpansionState(m_tree, m_expandedState, keyForIndex);
    m_hasExpandedState = true;
    m_proxyModel->setFilterFixedString(filter);
    Utils::restoreExpansionState(m_tree, m_expandedState, keyForIndex);
}

QString AdvancedPageWidget::sourceIndexPath(const QModelIndex& index) const
{
    return Utils::modelIndexPath(m_proxyModel->mapToSource(index));
}

AdvancedPage::AdvancedPage(AdvancedSettingsRegistry* registry, SettingsDialogController* controller, QObject* parent)
    : SettingsPage{controller, parent}
{
    setId(Constants::Page::Advanced);
    setName(tr("Advanced"));
    setCategory({tr("Advanced")});
    setWidgetCreator([registry] { return new AdvancedPageWidget(registry); });
}
} // namespace Fooyin

#include "advancedpage.moc"
#include "moc_advancedpage.cpp"
