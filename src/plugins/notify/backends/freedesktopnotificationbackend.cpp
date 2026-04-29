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

#include "freedesktopnotificationbackend.h"

#include <QApplication>
#include <QDBusReply>

using namespace Qt::StringLiterals;

constexpr auto FreedesktopDbusService = "org.freedesktop.Notifications";
constexpr auto FreedesktopDbusPath    = "/org/freedesktop/Notifications";
constexpr auto NotificationIconName   = "org.fooyin.fooyin";
constexpr auto DefaultTimeoutMs       = 5000;

namespace Fooyin::Notify {
FreedesktopNotificationBackend::FreedesktopNotificationBackend(QObject* parent)
    : NotificationBackend{parent}
    , m_notifications{new OrgFreedesktopNotificationsInterface(QString::fromLatin1(FreedesktopDbusService),
                                                               QString::fromLatin1(FreedesktopDbusPath),
                                                               QDBusConnection::sessionBus(), this)}
    , m_lastNotificationId{0}
{
    QObject::connect(m_notifications, &OrgFreedesktopNotificationsInterface::NotificationClosed, this,
                     &FreedesktopNotificationBackend::notificationClosed);
    QDBusConnection::sessionBus().connect(QString::fromLatin1(FreedesktopDbusService),
                                          QString::fromLatin1(FreedesktopDbusPath),
                                          QString::fromLatin1(FreedesktopDbusService), u"ActionInvoked"_s, this,
                                          // clang-format off
                                          SLOT(notificationActionInvoked(uint,QString)));
    // clang-format on

    if(m_notifications->isValid()) {
        m_capabilities.albumArt         = true;
        m_capabilities.timeout          = true;
        m_capabilities.playbackControls = supportsActions();
    }
}

bool FreedesktopNotificationBackend::isValid() const
{
    return m_notifications->isValid();
}

NotificationCapabilities FreedesktopNotificationBackend::capabilities() const
{
    return m_capabilities;
}

void FreedesktopNotificationBackend::sendNotification(const NotificationRequest& request)
{
    QStringList actions;
    actions.reserve(request.actions.size() * 2);

    for(const auto& action : request.actions) {
        actions << action.id << action.label;
    }

    QVariantMap hints;
    if(!request.cover.isNull()) {
        const QImage image = request.cover.toImage()
                                 .scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                                 .convertToFormat(QImage::Format_RGBA8888);

        const QByteArray imageBytes{reinterpret_cast<const char*>(image.constBits()),
                                    static_cast<int>(image.sizeInBytes())};

        const ImageData imageData{
            .width         = image.width(),
            .height        = image.height(),
            .rowstride     = static_cast<int>(image.bytesPerLine()),
            .hasAlpha      = true,
            .bitsPerSample = 8,
            .channels      = 4,
            .data          = imageBytes,
        };

        hints[u"image-data"_s] = QVariant::fromValue(imageData);
    }

    const uint32_t replacesId = m_notificationExpiryTimer.isActive() ? m_lastNotificationId : 0;

    auto* watcher
        = new QDBusPendingCallWatcher(m_notifications->Notify(QApplication::applicationDisplayName(), replacesId,
                                                              QString::fromLatin1(NotificationIconName), request.title,
                                                              request.body, actions, hints, request.timeoutMs),
                                      this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this,
                     &FreedesktopNotificationBackend::notificationCallFinished);

    const int timeoutMs = request.timeoutMs >= 0 ? request.timeoutMs : DefaultTimeoutMs;
    if(timeoutMs > 0) {
        m_notificationExpiryTimer.start(timeoutMs, this);
    }
    else {
        clearActiveNotification();
    }
}

void FreedesktopNotificationBackend::resetIdentities()
{
    clearActiveNotification();
}

void FreedesktopNotificationBackend::timerEvent(QTimerEvent* event)
{
    if(event->timerId() == m_notificationExpiryTimer.timerId()) {
        clearActiveNotification();
        return;
    }

    NotificationBackend::timerEvent(event);
}

void FreedesktopNotificationBackend::clearActiveNotification()
{
    m_notificationExpiryTimer.stop();
    m_lastNotificationId = 0;
}

bool FreedesktopNotificationBackend::supportsActions() const
{
    const QDBusReply<QStringList> reply = m_notifications->call(u"GetCapabilities"_s);
    if(!reply.isValid()) {
        return false;
    }

    return reply.value().contains(u"actions"_s);
}

void FreedesktopNotificationBackend::notificationClosed(uint id, uint /*reason*/)
{
    if(id == m_lastNotificationId) {
        clearActiveNotification();
    }
}

void FreedesktopNotificationBackend::notificationActionInvoked(uint id, const QString& actionKey)
{
    if(id != m_lastNotificationId) {
        return;
    }

    clearActiveNotification();
    Q_EMIT actionInvoked(actionKey);
}

void FreedesktopNotificationBackend::notificationCallFinished(QDBusPendingCallWatcher* watcher)
{
    const QDBusPendingReply<uint> reply = *watcher;
    if(reply.isError()) {
        qCWarning(NOTIFY) << "Failed to send freedesktop notification:" << reply.error().message();
        Q_EMIT notificationSent(false);
    }
    else {
        m_lastNotificationId = reply.value();
        Q_EMIT notificationSent(true);
    }

    watcher->deleteLater();
}
} // namespace Fooyin::Notify