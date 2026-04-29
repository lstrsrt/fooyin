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

#pragma once

#include "notificationbackend.h"
#include "notificationsinterface.h"

#include <QBasicTimer>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QTimerEvent>

namespace Fooyin::Notify {
class FreedesktopNotificationBackend : public NotificationBackend
{
    Q_OBJECT

public:
    explicit FreedesktopNotificationBackend(QObject* parent = nullptr);

    [[nodiscard]] bool isValid() const override;
    [[nodiscard]] NotificationCapabilities capabilities() const override;

    void sendNotification(const NotificationRequest& request) override;
    void resetIdentities() override;

protected:
    void timerEvent(QTimerEvent* event) override;

private:
    void clearActiveNotification();
    [[nodiscard]] bool supportsActions() const;
    void notificationClosed(uint id, uint reason);
    void notificationCallFinished(QDBusPendingCallWatcher* watcher);

private Q_SLOTS:
    void notificationActionInvoked(uint id, const QString& actionKey);

private:
    OrgFreedesktopNotificationsInterface* m_notifications;
    NotificationCapabilities m_capabilities;
    QBasicTimer m_notificationExpiryTimer;
    uint32_t m_lastNotificationId;
};
} // namespace Fooyin::Notify
