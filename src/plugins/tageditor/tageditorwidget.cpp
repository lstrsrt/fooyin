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

#include "tageditorwidget.h"

#include "core/coresettings.h"
#include "tageditoritem.h"
#include "tageditormodel.h"

#include <gui/guiconstants.h>
#include <utils/actions/actionmanager.h>
#include <utils/actions/command.h>
#include <utils/actions/widgetcontext.h>
#include <utils/multilinedelegate.h>
#include <utils/settings/settingsmanager.h>
#include <utils/stardelegate.h>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QTableView>
#include <QToolButton>

namespace Fooyin::TagEditor {
class TagEditorDelegate : public MultiLineEditDelegate
{
    Q_OBJECT

public:
    using MultiLineEditDelegate::MultiLineEditDelegate;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        if(index.data(TagEditorItem::IsDefault).toBool()) {
            // NOLINTNEXTLINE
            return QStyledItemDelegate::createEditor(parent, option, index); // clazy:exclude=skipped-base-method
        }
        return MultiLineEditDelegate::createEditor(parent, option, index);
    }
};

TagEditorView::TagEditorView(ActionManager* actionManager, QWidget* parent)
    : ExtendableTableView{actionManager, parent}
    , m_actionManager{actionManager}
    , m_editTrigger{EditTrigger::AllEditTriggers}
    , m_context{new WidgetContext(this, Context{"Context.TagEditor"}, this)}
    , m_copyAction{new QAction(tr("Copy"), this)}
    , m_pasteAction{new QAction(tr("Paste"), this)}
    , m_pasteFields{new QAction(tr("Paste Fields"), this)}
{
    actionManager->addContextObject(m_context);

    setItemDelegateForColumn(1, new TagEditorDelegate(this));
    setItemDelegateForRow(13, new StarDelegate(this));
    setTextElideMode(Qt::ElideRight);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionsClickable(false);
    verticalHeader()->setVisible(false);
}

void TagEditorView::setTagEditTriggers(EditTrigger triggers)
{
    m_editTrigger = triggers;
    setEditTriggers(triggers);
}

void TagEditorView::setupActions()
{
    m_copyAction->setShortcut(QKeySequence::Copy);
    if(auto* command = m_actionManager->command(Constants::Actions::Copy)) {
        m_copyAction->setShortcuts(command->defaultShortcuts());
    }
    QObject::connect(selectionModel(), &QItemSelectionModel::selectionChanged, this,
                     [this]() { m_copyAction->setEnabled(selectionModel()->hasSelection()); });
    QObject::connect(m_copyAction, &QAction::triggered, this, &TagEditorView::copySelection);
    m_copyAction->setEnabled(selectionModel()->hasSelection());

    m_pasteAction->setShortcut(QKeySequence::Paste);
    if(auto* command = m_actionManager->command(Constants::Actions::Paste)) {
        m_pasteAction->setShortcuts(command->defaultShortcuts());
    }
    QObject::connect(QApplication::clipboard(), &QClipboard::changed, this,
                     [this]() { m_pasteAction->setEnabled(!QApplication::clipboard()->text().isEmpty()); });
    QObject::connect(m_pasteAction, &QAction::triggered, this, [this]() { pasteSelection(false); });
    m_pasteAction->setEnabled(!QApplication::clipboard()->text().isEmpty());

    QObject::connect(QApplication::clipboard(), &QClipboard::changed, this,
                     [this]() { m_pasteFields->setEnabled(!QApplication::clipboard()->text().isEmpty()); });
    QObject::connect(m_pasteFields, &QAction::triggered, this, [this]() { pasteSelection(true); });
    m_pasteFields->setEnabled(!QApplication::clipboard()->text().isEmpty());
}

int TagEditorView::sizeHintForRow(int row) const
{
    if(!model()->hasIndex(row, 0, {})) {
        return 0;
    }

    // Only resize rows based on first column
    return model()->index(row, 0, {}).data(Qt::SizeHintRole).toSize().height();
}

void TagEditorView::setupContextActions(QMenu* menu, const QPoint& /*pos*/)
{
    menu->addAction(m_copyAction);
    menu->addAction(m_pasteAction);
    menu->addSeparator();
    menu->addAction(m_pasteFields);
}

void TagEditorView::mousePressEvent(QMouseEvent* event)
{
    if(event->button() == Qt::RightButton) {
        // Don't start editing on right-click
        setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
    else {
        setEditTriggers(m_editTrigger);
    }

    ExtendableTableView::mousePressEvent(event);
}

void TagEditorView::keyPressEvent(QKeyEvent* event)
{
    if((event == QKeySequence::Copy)) {
        copySelection();
        return;
    }
    if((event == QKeySequence::Paste)) {
        pasteSelection(false);
        return;
    }

    ExtendableTableView::keyPressEvent(event);
}

void TagEditorView::copySelection()
{
    const auto selected = selectionModel()->selectedRows();
    if(selected.empty()) {
        return;
    }

    QStringList fields;
    for(const auto& index : selected) {
        const QString value = index.siblingAtColumn(1).data().toString();
        if(value.isEmpty()) {
            continue;
        }
        const QString field = index.siblingAtColumn(0).data().toString();
        fields.emplace_back(field + u" : " + value);
    }

    QApplication::clipboard()->setText(fields.join(u"\n"));
}

void TagEditorView::pasteSelection(bool match)
{
    const auto selected = selectionModel()->selectedRows();
    if(!match && selected.empty()) {
        return;
    }

    std::map<QString, QString> values;

    const QString text      = QApplication::clipboard()->text();
    const QStringList pairs = text.split(QStringLiteral("\n"), Qt::SkipEmptyParts);

    for(int i{0}; const QString& pair : pairs) {
        if(pair.contains(u" : ")) {
            const auto& parts = pair.split(QStringLiteral(" : "), Qt::SkipEmptyParts);
            if(parts.size() != 2) {
                continue;
            }

            if(!match && std::cmp_less(i, selected.size())) {
                values.emplace(selected.at(i++).data().toString(), parts.at(1));
            }
            else {
                values.emplace(parts.at(0), parts.at(1));
            }
        }
        else if(!match && std::cmp_less(i, selected.size())) {
            values.emplace(selected.at(i++).data().toString(), pair);
        }
    }

    if(auto* tagModel = qobject_cast<TagEditorModel*>(model())) {
        tagModel->updateValues(values);
    }
}

TagEditorWidget::TagEditorWidget(const TrackList& tracks, bool readOnly, ActionManager* actionManager,
                                 SettingsManager* settings, QWidget* parent)
    : PropertiesTabWidget{parent}
    , m_settings{settings}
    , m_view{new TagEditorView(actionManager, this)}
    , m_model{new TagEditorModel(settings, this)}
    , m_toolsButton{new QToolButton(this)}
    , m_toolsMenu{new QMenu(this)}
{
    setObjectName(TagEditorWidget::name());
    setWindowFlags(Qt::Dialog);
    resize(600, 720);
    setMinimumSize(300, 400);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);

    m_view->setExtendableModel(m_model);
    setupToolsMenu();

    m_toolsButton->setText(tr("Tools"));
    m_toolsButton->setMenu(m_toolsMenu);
    m_toolsButton->setPopupMode(QToolButton::InstantPopup);
    m_view->addCustomTool(m_toolsButton);

    QObject::connect(m_model, &QAbstractItemModel::rowsInserted, m_view, &QTableView::resizeRowsToContents);
    QObject::connect(m_model, &QAbstractItemModel::modelReset, this, [this]() {
        m_view->resizeColumnsToContents();
        m_view->resizeRowsToContents();
        restoreState();
    });
    QObject::connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this, readOnly]() {
        if(!readOnly) {
            const QModelIndexList selected = m_view->selectionModel()->selectedIndexes();
            m_view->removeRowAction()->setEnabled(!selected.empty());
        }
    });

    m_view->setTagEditTriggers(readOnly ? QAbstractItemView::NoEditTriggers : QAbstractItemView::AllEditTriggers);
    m_view->addRowAction()->setDisabled(readOnly);
    m_view->removeRowAction()->setDisabled(readOnly);
    m_toolsButton->setDisabled(readOnly);

    m_model->reset(tracks);
    m_view->setupActions();
}

TagEditorWidget::~TagEditorWidget()
{
    saveState();
}

QString TagEditorWidget::name() const
{
    return QStringLiteral("Tag Editor");
}

QString TagEditorWidget::layoutName() const
{
    return QStringLiteral("TagEditor");
}

void TagEditorWidget::apply()
{
    if(!m_model->haveChanges()) {
        return;
    }

    const bool updateStats = m_model->haveOnlyStatChanges()
                          && !m_settings->value<Settings::Core::SaveRatingToMetadata>()
                          && !m_settings->value<Settings::Core::SavePlaycountToMetadata>();

    auto applyChanges = [this, updateStats]() {
        m_model->applyChanges();
        if(updateStats) {
            emit trackStatsChanged(m_model->tracks());
        }
        else {
            emit trackMetadataChanged(m_model->tracks());
        }
    };

    if(m_settings->fileValue(QStringLiteral("TagEditor/DontAskAgain")).toBool()) {
        applyChanges();
        return;
    }

    QMessageBox message;
    message.setIcon(QMessageBox::Warning);
    message.setText(QStringLiteral("Are you sure?"));
    message.setInformativeText(tr("Metadata in the associated files will be overwritten."));

    auto* dontAskAgain = new QCheckBox(tr("Don't ask again"), &message);
    message.setCheckBox(dontAskAgain);

    message.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    message.setDefaultButton(QMessageBox::No);

    const int buttonClicked = message.exec();

    if(buttonClicked == QMessageBox::Yes) {
        if(dontAskAgain->isChecked()) {
            m_settings->fileSet(QStringLiteral("TagEditor/DontAskAgain"), true);
        }
        applyChanges();
    }
}

void TagEditorWidget::setupToolsMenu()
{
    auto* autoTrackNum = new QAction(tr("Auto &track number"), this);
    QObject::connect(autoTrackNum, &QAction::triggered, m_model, &TagEditorModel::autoNumberTracks);
    m_toolsMenu->addAction(autoTrackNum);
}

void TagEditorWidget::saveState() const
{
    const QByteArray state = m_view->horizontalHeader()->saveState();
    m_settings->fileSet(QStringLiteral("TagEditor/State"), state);
}

void TagEditorWidget::restoreState() const
{
    const QByteArray state = m_settings->fileValue(QStringLiteral("TagEditor/State")).toByteArray();

    if(state.isEmpty()) {
        return;
    }

    m_view->horizontalHeader()->restoreState(state);
}
} // namespace Fooyin::TagEditor

#include "moc_tageditorwidget.cpp"
#include "tageditorwidget.moc"
