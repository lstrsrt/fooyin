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

#include "portalnotificationbackend.h"

#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QVariant>

using namespace Qt::StringLiterals;

constexpr auto PortalDbusService    = "org.freedesktop.portal.Desktop";
constexpr auto PortalDbusPath       = "/org/freedesktop/portal/desktop";
constexpr auto PortalDbusInterface  = "org.freedesktop.portal.Notification";
constexpr auto PortalNotificationId = "track-change";
constexpr auto NotificationIconName = "org.fooyin.fooyin";

namespace {
QVariantMap makePortalNotification(const Fooyin::Notify::NotificationRequest& request)
{
    QVariantMap notification{
        {u"title"_s, request.title},
        {u"body"_s, request.body},
        {u"icon"_s, QString::fromLatin1(NotificationIconName)},
    };

    return notification;
}
} // namespace

namespace Fooyin::Notify {
PortalNotificationBackend::PortalNotificationBackend(QObject* parent)
    : NotificationBackend{parent}
    , m_notificationPortal{new QDBusInterface(
          QString::fromLatin1(PortalDbusService), QString::fromLatin1(PortalDbusPath),
          QString::fromLatin1(PortalDbusInterface), QDBusConnection::sessionBus(), this)}
    , m_notificationGen{0}
{
    QDBusConnection::sessionBus().connect(QString::fromLatin1(PortalDbusService), QString::fromLatin1(PortalDbusPath),
                                          QString::fromLatin1(PortalDbusInterface), u"ActionInvoked"_s, this,
                                          // clang-format off
                                          SLOT(portalActionInvoked(QString,QString,QVariantList)));
    // clang-format on

    if(m_notificationPortal->isValid()) {
        m_capabilities.playbackControls = true;
    }
}

bool PortalNotificationBackend::isValid() const
{
    return m_notificationPortal->isValid();
}

NotificationCapabilities PortalNotificationBackend::capabilities() const
{
    return m_capabilities;
}

void PortalNotificationBackend::sendNotification(const NotificationRequest& request)
{
    QVariantMap portalNotification = makePortalNotification(request);

    PortalButtons buttons;
    buttons.reserve(request.actions.size());

    for(const auto& action : request.actions) {
        buttons << QVariantMap{
            {u"action"_s, action.id},
            {u"label"_s, action.label},
        };
    }

    if(!buttons.isEmpty()) {
        portalNotification[u"buttons"_s] = QVariant::fromValue(buttons);
    }

    auto* watcher = new QDBusPendingCallWatcher(
        m_notificationPortal->asyncCall(u"AddNotification"_s, currentNotificationId(), portalNotification), this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this,
                     &PortalNotificationBackend::notificationCallFinished);
}

void PortalNotificationBackend::resetIdentities()
{
    ++m_notificationGen;
}

QString PortalNotificationBackend::currentNotificationId() const
{
    return QStringLiteral("%1-%2").arg(QString::fromLatin1(PortalNotificationId)).arg(m_notificationGen);
}

void PortalNotificationBackend::portalActionInvoked(const QString& id, const QString& action,
                                                    const QVariantList& /*parameter*/)
{
    if(id != currentNotificationId()) {
        return;
    }

    ++m_notificationGen;
    emit actionInvoked(action);
}

void PortalNotificationBackend::notificationCallFinished(QDBusPendingCallWatcher* watcher)
{
    const QDBusPendingReply<> reply = *watcher;
    if(reply.isError()) {
        qCWarning(NOTIFY) << "Failed to send portal notification:" << reply.error().message();
        emit notificationSent(false);
    }
    else {
        emit notificationSent(true);
    }

    watcher->deleteLater();
}
} // namespace Fooyin::Notify