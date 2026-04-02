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

#include "notifyplugin.h"

#include "notificationsinterface.h"
#include "settings/notifypage.h"
#include "settings/notifysettings.h"

#include <core/coresettings.h>
#include <core/player/playercontroller.h>
#include <utils/settings/settingsmanager.h>

#include <QApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QImage>
#include <QLoggingCategory>
#include <QPixmap>

Q_LOGGING_CATEGORY(NOTIFY, "fy.notify")

using namespace Qt::StringLiterals;

constexpr auto FreedesktopDbusService = "org.freedesktop.Notifications";
constexpr auto FreedesktopDbusPath    = "/org/freedesktop/Notifications";
constexpr auto PortalDbusService      = "org.freedesktop.portal.Desktop";
constexpr auto PortalDbusPath         = "/org/freedesktop/portal/desktop";
constexpr auto PortalDbusInterface    = "org.freedesktop.portal.Notification";
constexpr auto PortalNotificationId   = "track-change";
constexpr auto NotificationIconName   = "org.fooyin.fooyin";
constexpr auto PreviousActionId       = "previous";
constexpr auto PlayPauseActionId      = "play-pause";
constexpr auto NextActionId           = "next";

namespace {
struct ImageData
{
    int width{0};
    int height{0};
    int rowstride{0};
    bool hasAlpha{false};
    int bitsPerSample{0};
    int channels{0};
    QByteArray data;
};

using PortalButtons = QList<QVariantMap>;

QDBusArgument& operator<<(QDBusArgument& argument, const ImageData& image)
{
    argument.beginStructure();
    argument << image.width;
    argument << image.height;
    argument << image.rowstride;
    argument << image.hasAlpha;
    argument << image.bitsPerSample;
    argument << image.channels;
    argument << image.data;
    argument.endStructure();

    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, ImageData& image)
{
    argument.beginStructure();
    argument >> image.width;
    argument >> image.height;
    argument >> image.rowstride;
    argument >> image.hasAlpha;
    argument >> image.bitsPerSample;
    argument >> image.channels;
    argument >> image.data;
    argument.endStructure();

    return argument;
}

QVariantMap makePortalNotification(const QString& title, const QString& body)
{
    QVariantMap notification{
        {u"title"_s, title},
        {u"body"_s, body},
        {u"icon"_s, QString::fromLatin1(NotificationIconName)},
    };

    return notification;
}
} // namespace

Q_DECLARE_METATYPE(ImageData)
Q_DECLARE_METATYPE(PortalButtons)

namespace Fooyin::Notify {
NotifyPlugin::NotifyPlugin()
    : m_playerController{nullptr}
    , m_settings{nullptr}
    , m_coverProvider{nullptr}
    , m_notifications{nullptr}
    , m_notificationPortal{nullptr}
    , m_notifyPage{nullptr}
    , m_backend{NotificationBackend::None}
    , m_notificationGeneration{0}
    , m_portalNotificationGeneration{0}
    , m_lastNotificationId{0}
    , m_notificationRequestInFlight{false}
{ }

void NotifyPlugin::initialise(const CorePluginContext& context)
{
    m_playerController = context.playerController;
    m_audioLoader      = context.audioLoader;
    m_settings         = context.settingsManager;
}

void NotifyPlugin::initialise(const GuiPluginContext& /*context*/)
{
    m_coverProvider = new CoverProvider(m_audioLoader, m_settings, this);
    m_coverProvider->setUsePlaceholder(false);

    m_notifySettings = std::make_unique<NotifySettings>(m_settings);
    m_notifyPage     = new NotifyPage(m_settings, this, this);

    qDBusRegisterMetaType<ImageData>();
    qDBusRegisterMetaType<PortalButtons>();
    initialiseNotificationBackend();

    QObject::connect(m_playerController, &PlayerController::currentTrackChanged, this, &NotifyPlugin::trackChanged);
    QObject::connect(m_playerController, &PlayerController::playStateChanged, this, &NotifyPlugin::playStateChanged);
}

bool NotifyPlugin::supportsAlbumArt()
{
    initialiseNotificationBackend();
    return m_backend == NotificationBackend::Freedesktop;
}

bool NotifyPlugin::supportsPlaybackControls()
{
    initialiseNotificationBackend();
    if(m_backend == NotificationBackend::Freedesktop) {
        return freedesktopSupportsActions();
    }

    return m_backend == NotificationBackend::Portal;
}

bool NotifyPlugin::supportsTimeout()
{
    initialiseNotificationBackend();
    return m_backend == NotificationBackend::Freedesktop;
}

void NotifyPlugin::notificationClosed(uint /*id*/, uint /*reason*/) { }

void NotifyPlugin::notificationActionInvoked(uint id, const QString& actionKey)
{
    if(id != m_lastNotificationId) {
        return;
    }

    m_lastNotificationId = 0;
    handleNotificationAction(actionKey);
}

void NotifyPlugin::portalActionInvoked(const QString& id, const QString& action, const QVariantList& /*parameter*/)
{
    if(id != currentPortalNotificationId()) {
        return;
    }

    ++m_portalNotificationGeneration;
    handleNotificationAction(action);
}

void NotifyPlugin::playStateChanged(Player::PlayState state)
{
    if(state == Player::PlayState::Stopped) {
        resetNotificationIdentities();
    }
}

void NotifyPlugin::trackChanged(const Track& track)
{
    ++m_notificationGeneration;

    if(!m_settings->value<Settings::Notify::Enabled>()) {
        return;
    }

    if(!track.isValid()) {
        resetNotificationIdentities();
        return;
    }

    initialiseNotificationBackend();

    if(m_backend != NotificationBackend::Freedesktop || !m_settings->value<Settings::Notify::ShowAlbumArt>()) {
        showNotification(track, QPixmap{});
        return;
    }

    m_coverProvider->trackCoverFull(track, Track::Cover::Front)
        .then(this, [this, loadGeneration = m_notificationGeneration, track](const QPixmap& cover) {
            if(loadGeneration == m_notificationGeneration && m_settings->value<Settings::Notify::Enabled>()) {
                showNotification(track, cover);
            }
        });
}

void NotifyPlugin::showNotification(const Track& track, const QPixmap& cover)
{
    const QString titleField = m_settings->value<Settings::Notify::TitleField>();
    const QString bodyField  = m_settings->value<Settings::Notify::BodyField>();

    const QString title = m_scriptParser.evaluate(titleField, track);
    const QString body  = m_scriptParser.evaluate(bodyField, track);

    sendNotification(title, body, cover);
}

void NotifyPlugin::sendNotification(const QString& title, const QString& body, const QPixmap& cover)
{
    m_pendingNotification = PendingNotification{
        .title = title,
        .body  = body,
        .cover = cover,
    };

    if(m_notificationRequestInFlight) {
        return;
    }

    sendPendingNotification();
}

void NotifyPlugin::sendPendingNotification()
{
    if(!m_pendingNotification.has_value()) {
        return;
    }

    initialiseNotificationBackend();

    const PendingNotification notification = std::move(*m_pendingNotification);
    m_pendingNotification.reset();
    m_notificationRequestInFlight = true;

    switch(m_backend) {
        case NotificationBackend::Freedesktop:
            sendFreedesktopNotification(notification);
            break;
        case NotificationBackend::Portal:
            sendPortalNotification(notification);
            break;
        case NotificationBackend::None:
            qCWarning(NOTIFY) << "Failed to connect to any D-Bus notification service";
            m_notificationRequestInFlight = false;
            break;
    }
}

void NotifyPlugin::initialiseNotificationBackend()
{
    if(m_backend == NotificationBackend::Freedesktop && m_notifications && m_notifications->isValid()) {
        return;
    }

    if(m_backend == NotificationBackend::Portal && m_notificationPortal && m_notificationPortal->isValid()) {
        return;
    }

    if(!m_notifications) {
        m_notifications = new OrgFreedesktopNotificationsInterface(QString::fromLatin1(FreedesktopDbusService),
                                                                   QString::fromLatin1(FreedesktopDbusPath),
                                                                   QDBusConnection::sessionBus(), this);

        QObject::connect(m_notifications, &OrgFreedesktopNotificationsInterface::NotificationClosed, this,
                         &NotifyPlugin::notificationClosed);
        // clang-format off
        QDBusConnection::sessionBus().connect(
            QString::fromLatin1(FreedesktopDbusService), QString::fromLatin1(FreedesktopDbusPath),
            QString::fromLatin1(FreedesktopDbusService), u"ActionInvoked"_s, this,
            SLOT(notificationActionInvoked(uint,QString)));
        // clang-format om
    }

    if(m_notifications->isValid()) {
        m_backend = NotificationBackend::Freedesktop;
        return;
    }

    if(!m_notificationPortal) {
        m_notificationPortal
            = new QDBusInterface(QString::fromLatin1(PortalDbusService), QString::fromLatin1(PortalDbusPath),
                                 QString::fromLatin1(PortalDbusInterface), QDBusConnection::sessionBus(), this);

        // clang-format off
       QDBusConnection::sessionBus().connect(
            QString::fromLatin1(PortalDbusService), QString::fromLatin1(PortalDbusPath),
            QString::fromLatin1(PortalDbusInterface), u"ActionInvoked"_s, this,
            SLOT(portalActionInvoked(QString,QString,QVariantList)));
        // clang-format om
    }

    if(m_notificationPortal->isValid()) {
        m_backend = NotificationBackend::Portal;
        return;
    }

    m_backend = NotificationBackend::None;
}

bool NotifyPlugin::freedesktopSupportsActions() const
{
    if(!m_notifications || !m_notifications->isValid()) {
        return false;
    }

    const QDBusReply<QStringList> reply = m_notifications->call(u"GetCapabilities"_s);
    if(!reply.isValid()) {
        return false;
    }

    const QStringList capabilities = reply.value();
    return capabilities.contains(u"actions"_s);
}

PlaybackControls NotifyPlugin::playbackControls() const
{
    return PlaybackControls::fromInt(m_settings->value<Settings::Notify::Controls>());
}

QString NotifyPlugin::currentPortalNotificationId() const
{
    return QStringLiteral("%1-%2").arg(QString::fromLatin1(PortalNotificationId)).arg(m_portalNotificationGeneration);
}

void NotifyPlugin::resetNotificationIdentities()
{
    m_lastNotificationId = 0;
    ++m_portalNotificationGeneration;
}

QStringList NotifyPlugin::notificationActions() const
{
    const PlaybackControls controls = playbackControls();
    if(controls == PlaybackControls{} || !freedesktopSupportsActions()) {
        return {};
    }

    const QString playPauseLabel
        = m_playerController->playState() == Player::PlayState::Playing ? tr("Pause") : tr("Play");

    QStringList actions;

    if(controls.testFlag(PlaybackControlFlag::Previous) && m_playerController->hasPreviousTrack()) {
        actions << QString::fromLatin1(PreviousActionId) << tr("Previous");
    }

    if(controls.testFlag(PlaybackControlFlag::PlayPause)) {
        actions << QString::fromLatin1(PlayPauseActionId) << playPauseLabel;
    }

    if(controls.testFlag(PlaybackControlFlag::Next) && m_playerController->hasNextTrack()) {
        actions << QString::fromLatin1(NextActionId) << tr("Next");
    }

    return actions;
}

QList<QVariantMap> NotifyPlugin::notificationButtons() const
{
    QList<QVariantMap> buttons;

    const PlaybackControls controls = playbackControls();
    if(controls == PlaybackControls{}) {
        return buttons;
    }

    if(controls.testFlag(PlaybackControlFlag::Previous) && m_playerController->hasPreviousTrack()) {
        buttons << QVariantMap{
            {u"action"_s, QString::fromLatin1(PreviousActionId)},
            {u"label"_s, tr("Previous")},
        };
    }

    if(controls.testFlag(PlaybackControlFlag::PlayPause)) {
        buttons << QVariantMap{
            {u"action"_s, QString::fromLatin1(PlayPauseActionId)},
            {u"label"_s, m_playerController->playState() == Player::PlayState::Playing ? tr("Pause") : tr("Play")},
        };
    }

    if(controls.testFlag(PlaybackControlFlag::Next) && m_playerController->hasNextTrack()) {
        buttons << QVariantMap{
            {u"action"_s, QString::fromLatin1(NextActionId)},
            {u"label"_s, tr("Next")},
        };
    }

    return buttons;
}

void NotifyPlugin::handleNotificationAction(const QString& actionKey)
{
    if(m_settings->value<Settings::Core::Shutdown>()) {
        return;
    }

    if(actionKey == QString::fromLatin1(PreviousActionId)) {
        m_playerController->previous();
    }
    else if(actionKey == QString::fromLatin1(PlayPauseActionId)) {
        m_playerController->playPause();
    }
    else if(actionKey == QString::fromLatin1(NextActionId)) {
        m_playerController->next();
    }
}

void NotifyPlugin::sendFreedesktopNotification(const PendingNotification& notification)
{
    const QString appName = QApplication::applicationDisplayName();
    const QString appIcon = QString::fromLatin1(NotificationIconName);
    const int timeout     = m_settings->value<Settings::Notify::Timeout>();

    const QStringList actions = notificationActions();
    QVariantMap hints;

    if(!notification.cover.isNull()) {
        const QImage image = notification.cover.toImage()
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

    auto* watcher = new QDBusPendingCallWatcher(m_notifications->Notify(appName, m_lastNotificationId, appIcon,
                                                                        notification.title, notification.body, actions,
                                                                        hints, timeout),
                                                this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this,
                     [this, notification](QDBusPendingCallWatcher* finishedWatcher) {
                         notificationCallFinished(NotificationBackend::Freedesktop, notification, finishedWatcher);
                     });
}

void NotifyPlugin::sendPortalNotification(const PendingNotification& notification)
{
    QVariantMap portalNotification = makePortalNotification(notification.title, notification.body);
    if(const PortalButtons buttons = notificationButtons(); !buttons.isEmpty()) {
        portalNotification[u"buttons"_s] = QVariant::fromValue(buttons);
    }

    auto* watcher = new QDBusPendingCallWatcher(
        m_notificationPortal->asyncCall(u"AddNotification"_s, currentPortalNotificationId(),
                                        portalNotification),
        this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this,
                     [this, notification](QDBusPendingCallWatcher* finishedWatcher) {
                         notificationCallFinished(NotificationBackend::Portal, notification, finishedWatcher);
                     });
}

void NotifyPlugin::notificationCallFinished(NotificationBackend backend, const PendingNotification& notification,
                                            QDBusPendingCallWatcher* watcher)
{
    bool shouldRetryWithPortal = false;

    if(backend == NotificationBackend::Freedesktop) {
        const QDBusPendingReply<uint> reply = *watcher;
        if(!reply.isError()) {
            m_lastNotificationId = reply.value();
        }
        else {
            shouldRetryWithPortal = true;
            qCWarning(NOTIFY) << "Failed to send freedesktop notification:" << reply.error().message();
        }
    }
    else {
        const QDBusPendingReply<> reply = *watcher;
        if(reply.isError()) {
            qCWarning(NOTIFY) << "Failed to send portal notification:" << reply.error().message();
        }
    }

    watcher->deleteLater();
    m_notificationRequestInFlight = false;

    if(shouldRetryWithPortal) {
        m_backend = NotificationBackend::Portal;
        m_pendingNotification = notification;
    }

    if(m_pendingNotification.has_value()) {
        sendPendingNotification();
    }
}
} // namespace Fooyin::Notify

#include "moc_notifyplugin.cpp"
