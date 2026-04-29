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

#include "shortcutspage.h"

#include "shortcutsmodel.h"

#include <gui/guiconstants.h>
#include <gui/widgets/expandableinputbox.h>
#include <utils/actions/actionmanager.h>
#include <utils/settings/settingsmanager.h>

#include <QApplication>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>
#include <QTreeView>

using namespace Qt::StringLiterals;

namespace Fooyin {
class ShortcutInput : public ExpandableInput
{
    Q_OBJECT

public:
    explicit ShortcutInput(QWidget* parent = nullptr)
        : ExpandableInput{ExpandableInput::ClearButton | ExpandableInput::CustomWidget, parent}
        , m_shortcut{new QKeySequenceEdit(this)}
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_shortcut);

        m_shortcut->setClearButtonEnabled(true);

        QObject::connect(m_shortcut, &QKeySequenceEdit::keySequenceChanged, this,
                         [this](const QKeySequence& shortcut) { Q_EMIT textChanged(shortcut.toString()); });
    }

    [[nodiscard]] QString text() const override
    {
        return m_shortcut->keySequence().toString();
    }

    void setShortcut(const QKeySequence& shortcut)
    {
        m_shortcut->setKeySequence(shortcut);
    }

private:
    QKeySequenceEdit* m_shortcut;
};

class ShortcutsPageWidget : public SettingsPageWidget
{
    Q_OBJECT

public:
    explicit ShortcutsPageWidget(ActionManager* actionManager);

    void load() override;
    void apply() override;
    void reset() override;

    [[nodiscard]] QString validationError() const override;

private:
    [[nodiscard]] Command* selectedCommand() const;
    void updateCurrentShortcuts(const ShortcutList& shortcuts);
    void updateConflictState();
    void selectionChanged();
    void shortcutChanged();
    void shortcutDeleted(const QString& text);
    void resetCurrentShortcut();
    void reassignConflicts();

    ActionManager* m_actionManager;

    QTreeView* m_shortcutTable;
    ShortcutsModel* m_model;
    QGroupBox* m_shortcutBox;
    ExpandableInputBox* m_inputBox;
    QLabel* m_conflictLabel;
    QPushButton* m_reassignButton;
};

ShortcutsPageWidget::ShortcutsPageWidget(ActionManager* actionManager)
    : m_actionManager{actionManager}
    , m_shortcutTable{new QTreeView(this)}
    , m_model{new ShortcutsModel(this)}
    , m_shortcutBox{new QGroupBox(this)}
    , m_inputBox{new ExpandableInputBox(tr("Shortcuts"), ExpandableInput::ClearButton | ExpandableInput::CustomWidget,
                                        this)}
    , m_conflictLabel{new QLabel(this)}
    , m_reassignButton{new QPushButton(tr("Overwrite Shortcut"), this)}
{
    m_shortcutTable->setModel(m_model);
    m_shortcutTable->hideColumn(1);
    m_shortcutTable->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(m_shortcutTable, 0, 0);

    QObject::connect(m_model, &QAbstractItemModel::modelReset, m_shortcutTable, &QTreeView::expandAll);

    auto* groupLayout = new QVBoxLayout(m_shortcutBox);

    m_inputBox->setMaximum(3);
    auto* resetShortcut = new QPushButton(tr("Reset"), this);
    m_inputBox->addBoxWidget(resetShortcut);
    m_inputBox->setInputWidget([this](QWidget* parent) {
        auto* input = new ShortcutInput(parent);
        QObject::connect(input, &ExpandableInput::textChanged, this, &ShortcutsPageWidget::shortcutChanged);
        return input;
    });

    groupLayout->addWidget(m_inputBox);

    m_conflictLabel->setWordWrap(true);
    m_conflictLabel->hide();
    groupLayout->addWidget(m_conflictLabel);

    m_reassignButton->hide();
    groupLayout->addWidget(m_reassignButton, 0, Qt::AlignLeft);

    layout->addWidget(m_shortcutBox, 1, 0);

    QObject::connect(resetShortcut, &QAbstractButton::clicked, this, &ShortcutsPageWidget::resetCurrentShortcut);
    QObject::connect(m_reassignButton, &QAbstractButton::clicked, this, &ShortcutsPageWidget::reassignConflicts);
    QObject::connect(m_shortcutTable->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                     &ShortcutsPageWidget::selectionChanged);
    QObject::connect(m_inputBox, &ExpandableInputBox::blockDeleted, this, &ShortcutsPageWidget::shortcutDeleted);

    m_shortcutBox->setDisabled(true);
}

void ShortcutsPageWidget::load()
{
    m_model->populate(m_actionManager);
    updateConflictState();
}

void ShortcutsPageWidget::apply()
{
    m_model->processQueue();
}

void ShortcutsPageWidget::reset()
{
    const auto commands = m_actionManager->commands();
    for(Command* command : commands) {
        command->setShortcut(command->defaultShortcuts());
    }
}

QString ShortcutsPageWidget::validationError() const
{
    return m_model->firstConflictError();
}

Command* ShortcutsPageWidget::selectedCommand() const
{
    const auto selected = m_shortcutTable->selectionModel()->selectedIndexes();
    if(selected.empty()) {
        return nullptr;
    }

    const QModelIndex index = selected.front();
    if(index.data(ShortcutItem::IsCategory).toBool()) {
        return nullptr;
    }

    return index.data(ShortcutItem::ActionCommand).value<Command*>();
}

void ShortcutsPageWidget::updateCurrentShortcuts(const ShortcutList& shortcuts)
{
    m_inputBox->clearBlocks();

    for(const auto& shortcut : shortcuts) {
        auto* input = new ShortcutInput(this);
        input->setShortcut(shortcut);
        QObject::connect(input, &ExpandableInput::textChanged, this, &ShortcutsPageWidget::shortcutChanged);
        m_inputBox->addInput(input);
    }

    if(shortcuts.empty()) {
        m_inputBox->addEmptyBlock();
    }
}

void ShortcutsPageWidget::updateConflictState()
{
    Command* command = selectedCommand();
    if(!command) {
        m_conflictLabel->hide();
        m_reassignButton->hide();
        return;
    }

    const QString conflicts = m_model->conflictDescription(command);
    if(conflicts.isEmpty()) {
        m_conflictLabel->hide();
        m_reassignButton->hide();
        return;
    }

    m_conflictLabel->setText(tr("Duplicate shortcuts") + u":\n%1"_s.arg(conflicts));
    m_conflictLabel->show();
    m_reassignButton->show();
}

void ShortcutsPageWidget::selectionChanged()
{
    Command* command = selectedCommand();
    if(!command) {
        m_shortcutBox->setDisabled(true);
        updateConflictState();
        return;
    }

    const auto shortcuts = m_model->shortcuts(command);

    updateCurrentShortcuts(shortcuts);
    m_shortcutBox->setDisabled(false);
    updateConflictState();
}

void ShortcutsPageWidget::shortcutChanged()
{
    Command* command = selectedCommand();
    if(!command) {
        return;
    }

    ShortcutList shortcuts;

    const auto inputs = m_inputBox->blocks();
    for(ExpandableInput* input : inputs) {
        const QString text = input->text();
        if(!text.isEmpty() && !shortcuts.contains(text)) {
            shortcuts.append(text);
        }
    }

    m_model->shortcutChanged(command, shortcuts);
    updateConflictState();
}

void ShortcutsPageWidget::shortcutDeleted(const QString& text)
{
    Command* command = selectedCommand();
    if(!command) {
        return;
    }

    m_model->shortcutDeleted(command, text);
    updateConflictState();
}

void ShortcutsPageWidget::resetCurrentShortcut()
{
    Command* command = selectedCommand();
    if(!command) {
        return;
    }

    m_model->shortcutChanged(command, command->defaultShortcuts());
    updateCurrentShortcuts(command->defaultShortcuts());
    updateConflictState();
}

void ShortcutsPageWidget::reassignConflicts()
{
    Command* command = selectedCommand();
    if(!command) {
        return;
    }

    m_model->reassignConflicts(command);
    updateConflictState();
}

ShortcutsPage::ShortcutsPage(ActionManager* actionManager, SettingsManager* settings, QObject* parent)
    : SettingsPage{settings->settingsDialog(), parent}
{
    setId(Constants::Page::Shortcuts);
    setName(tr("Shortcuts"));
    setCategory({tr("Shortcuts")});
    setWidgetCreator([actionManager] { return new ShortcutsPageWidget(actionManager); });
}
} // namespace Fooyin

#include "moc_shortcutspage.cpp"
#include "shortcutspage.moc"
