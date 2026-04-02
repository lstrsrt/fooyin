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

#include <QBasicTimer>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QTimerEvent>

namespace Fooyin::Notify {
class PortalNotificationBackend : public NotificationBackend
{
    Q_OBJECT

public:
    explicit PortalNotificationBackend(QObject* parent = nullptr);

    [[nodiscard]] bool isValid() const override;
    [[nodiscard]] NotificationCapabilities capabilities() const override;

    void sendNotification(const NotificationRequest& request) override;
    void resetIdentities() override;

protected:
    void timerEvent(QTimerEvent* event) override;

private:
    void clearActiveNotification();
    [[nodiscard]] QString currentNotificationId() const;
    void notificationCallFinished(QDBusPendingCallWatcher* watcher);

private slots:
    void portalActionInvoked(const QString& id, const QString& action, const QVariantList& parameter);

private:
    QDBusInterface* m_notificationPortal;
    NotificationCapabilities m_capabilities;
    QBasicTimer m_notificationExpiryTimer;
    uint64_t m_notificationGen;
    bool m_notificationActive;
};
} // namespace Fooyin::Notify