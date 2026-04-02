/*
 * Fooyin
 * Copyright 2026, Luke Taylor <luket@pm.me>
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

#include "notificationbackend.h"

#include <QApplication>
#include <QDBusArgument>
#include <QVariant>

using namespace Qt::StringLiterals;

namespace Fooyin::Notify {
Q_LOGGING_CATEGORY(NOTIFY, "fy.notify")

NotificationBackend::NotificationBackend(QObject* parent)
    : QObject{parent}
{ }

NotificationBackend::~NotificationBackend() = default;
} // namespace Fooyin::Notify

QDBusArgument& operator<<(QDBusArgument& argument, const Fooyin::Notify::ImageData& image)
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

const QDBusArgument& operator>>(const QDBusArgument& argument, Fooyin::Notify::ImageData& image)
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
