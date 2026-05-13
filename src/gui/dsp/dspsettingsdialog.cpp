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

#include <gui/dsp/dspsettingsdialog.h>

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace Fooyin {
DspSettingsDialog::DspSettingsDialog(QWidget* parent)
    : QDialog{parent}
    , m_mainLayout{new QVBoxLayout(this)}
    , m_contentLayout{new QVBoxLayout()}
    , m_buttonLayout{new QHBoxLayout()}
    , m_restoreButtonBox{new QDialogButtonBox(QDialogButtonBox::RestoreDefaults, this)}
    , m_buttonBox{new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this)}
{
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(10);

    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(10);
    m_mainLayout->addLayout(m_contentLayout, 1);

    m_buttonLayout->setContentsMargins(0, 0, 0, 0);
    m_buttonLayout->setSpacing(6);
    m_buttonLayout->addWidget(m_restoreButtonBox);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_buttonBox);
    m_mainLayout->addLayout(m_buttonLayout);

    QObject::connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    QObject::connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    QObject::connect(m_restoreButtonBox, &QDialogButtonBox::clicked, this, [this]() { restoreDefaults(); });
}

DspSettingsDialog::~DspSettingsDialog() = default;

QVBoxLayout* DspSettingsDialog::contentLayout() const
{
    return m_contentLayout;
}

void DspSettingsDialog::setRestoreDefaultsVisible(bool visible)
{
    m_restoreButtonBox->setVisible(visible);
}

void DspSettingsDialog::addButtonRowWidget(QWidget* widget)
{
    if(widget) {
        m_buttonLayout->insertWidget(0, widget);
    }
}

void DspSettingsDialog::setButtonsVisible(bool visible)
{
    m_restoreButtonBox->setVisible(visible);
    m_buttonBox->setVisible(visible);
}

void DspSettingsDialog::publishPreviewSettings()
{
    Q_EMIT previewSettingsChanged(saveSettings());
}
} // namespace Fooyin
