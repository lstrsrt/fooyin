/*
 * Fooyin
 * Copyright 2025, ripdog <https://github.com/ripdog>
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

#include "settings/notifysettings.h"

#include <core/plugins/coreplugin.h>
#include <core/plugins/plugin.h>
#include <core/scripting/scriptparser.h>
#include <gui/coverprovider.h>
#include <gui/plugins/guiplugin.h>

#include <QDBusPendingCallWatcher>
#include <QPixmap>
#include <QVariant>

#include <optional>

class OrgFreedesktopNotificationsInterface;
class QDBusInterface;

namespace Fooyin::Notify {
class NotifyPage;
class NotifySettings;

class NotifyPlugin : public QObject,
                     public Plugin,
                     public CorePlugin,
                     public GuiPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.fooyin.fooyin.plugin/1.0" FILE "notify.json")
    Q_INTERFACES(Fooyin::Plugin Fooyin::CorePlugin Fooyin::GuiPlugin)

public:
    NotifyPlugin();

    void initialise(const CorePluginContext& context) override;
    void initialise(const GuiPluginContext& context) override;

    bool supportsAlbumArt();
    bool supportsPlaybackControls();
    bool supportsTimeout();

private slots:
    void notificationClosed(uint id, uint reason);
    void notificationActionInvoked(uint id, const QString& actionKey);
    void portalActionInvoked(const QString& id, const QString& action, const QVariantList& parameter);
    void playStateChanged(Player::PlayState state);

private:
    enum class NotificationBackend : uint8_t
    {
        None = 0,
        Freedesktop,
        Portal,
    };

    struct PendingNotification
    {
        QString title;
        QString body;
        QPixmap cover;
    };

    void trackChanged(const Track& track);
    void showNotification(const Track& track, const QPixmap& cover);
    void sendNotification(const QString& title, const QString& body, const QPixmap& cover);
    void sendPendingNotification();

    void initialiseNotificationBackend();
    [[nodiscard]] PlaybackControls playbackControls() const;
    [[nodiscard]] QString currentPortalNotificationId() const;
    void resetNotificationIdentities();
    [[nodiscard]] bool freedesktopSupportsActions() const;
    [[nodiscard]] QStringList notificationActions() const;
    [[nodiscard]] QList<QVariantMap> notificationButtons() const;
    void handleNotificationAction(const QString& actionKey);
    void sendFreedesktopNotification(const PendingNotification& notification);
    void sendPortalNotification(const PendingNotification& notification);
    void notificationCallFinished(NotificationBackend backend, const PendingNotification& notification,
                                  QDBusPendingCallWatcher* watcher);

    PlayerController* m_playerController;
    std::shared_ptr<AudioLoader> m_audioLoader;
    SettingsManager* m_settings;
    CoverProvider* m_coverProvider;

    OrgFreedesktopNotificationsInterface* m_notifications;
    QDBusInterface* m_notificationPortal;
    std::unique_ptr<NotifySettings> m_notifySettings;
    NotifyPage* m_notifyPage;
    std::optional<PendingNotification> m_pendingNotification;

    ScriptParser m_scriptParser;
    NotificationBackend m_backend;
    uint64_t m_notificationGeneration;
    uint64_t m_portalNotificationGeneration;
    uint32_t m_lastNotificationId;
    bool m_notificationRequestInFlight;
};
} // namespace Fooyin::Notify
