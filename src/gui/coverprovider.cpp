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

#include <gui/coverprovider.h>

#include "internalguisettings.h"

#include <core/engine/audioloader.h>
#include <core/scripting/scriptparser.h>
#include <core/track.h>
#include <gui/guiconstants.h>
#include <gui/guipaths.h>
#include <gui/guisettings.h>
#include <gui/iconloader.h>
#include <utils/async.h>
#include <utils/crypto.h>
#include <utils/fileutils.h>
#include <utils/settings/settingsmanager.h>
#include <utils/utils.h>

#include <QBuffer>
#include <QByteArray>
#include <QCache>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QImageReader>
#include <QLoggingCategory>
#include <QMimeDatabase>
#include <QPromise>

#include <set>
#include <unordered_map>

Q_LOGGING_CATEGORY(COV_PROV, "fy.coverprovider")

using namespace Qt::StringLiterals;

constexpr auto MaxSize            = 1024;
constexpr int64_t RetryIntervalMs = 5000;

QCache<QString, QPixmap> Fooyin::CoverProvider::m_coverCache;
// Used to keep track of tracks without artwork so we don't query the filesystem more than necessary
std::set<QString> Fooyin::CoverProvider::m_noCoverKeys;

namespace {
using Fooyin::CoverProvider;

QString generateGroupedCoverKey(const QString& group, Fooyin::Track::Cover type,
                                Fooyin::ArtworkSourcePreference sourcePreference)
{
    return Fooyin::Utils::generateHash(
        u"FyCover|%1|%2"_s.arg(static_cast<int>(type)).arg(static_cast<int>(sourcePreference)), group);
}

QString generateTrackCoverKey(const Fooyin::Track& track, Fooyin::Track::Cover type,
                              Fooyin::ArtworkSourcePreference sourcePreference)
{
    return Fooyin::Utils::generateHash(
        u"FyCover|%1|%2"_s.arg(static_cast<int>(type)).arg(static_cast<int>(sourcePreference)), track.hash());
}

QString generateThumbCoverKey(const QString& key, int size)
{
    return Fooyin::Utils::generateHash(u"Thumb|%1|%2"_s.arg(key).arg(size));
}

QString coverThumbnailPath(const QString& key)
{
    return Fooyin::Gui::coverPath() + key + u".jpg"_s;
}

QString noCoverCacheKey(int size)
{
    return size > 0 ? u"|NoCover|%1"_s.arg(size) : u"|NoCover|"_s;
}

template <typename T>
QFuture<T> makeReadyFuture(T value)
{
    QPromise<T> promise;
    promise.start();
    promise.addResult(std::move(value));
    promise.finish();
    return promise.future();
}

bool saveThumbnail(const QImage& cover, const QString& key)
{
    QFile file{coverThumbnailPath(key)};
    if(file.open(QIODevice::WriteOnly)) {
        return cover.save(&file, "JPG", 85);
    }
    return false;
}

QSize calculateScaledSize(const QSize& originalSize, int maxSize)
{
    int newWidth{0};
    int newHeight{0};

    if(originalSize.width() > originalSize.height()) {
        newWidth  = maxSize;
        newHeight = (maxSize * originalSize.height()) / originalSize.width();
    }
    else {
        newHeight = maxSize;
        newWidth  = (maxSize * originalSize.width()) / originalSize.height();
    }

    return {newWidth, newHeight};
}

QString findDirectoryCover(const Fooyin::CoverPaths& paths, const Fooyin::Track& track, Fooyin::Track::Cover type)
{
    if(!track.isValid()) {
        return {};
    }

    static Fooyin::ScriptParser parser;
    static std::mutex parserGuard;
    const std::scoped_lock lock{parserGuard};

    QStringList filters;

    if(type == Fooyin::Track::Cover::Front) {
        for(const auto& path : paths.frontCoverPaths) {
            filters.emplace_back(parser.evaluate(path.trimmed(), track));
        }
    }
    else if(type == Fooyin::Track::Cover::Back) {
        for(const auto& path : paths.backCoverPaths) {
            filters.emplace_back(parser.evaluate(path.trimmed(), track));
        }
    }
    else if(type == Fooyin::Track::Cover::Artist) {
        for(const auto& path : paths.artistPaths) {
            filters.emplace_back(parser.evaluate(path.trimmed(), track));
        }
    }

    for(const auto& filter : filters) {
        const QStringList coverPaths = Fooyin::Utils::File::filesFromWildcardPath(filter);
        if(!coverPaths.empty()) {
            return coverPaths.constFirst();
        }
    }

    return {};
}

QString evaluateThumbnailGroupScript(const QString& script, const Fooyin::Track& track)
{
    static Fooyin::ScriptParser parser;
    static std::mutex parserGuard;
    const std::scoped_lock lock{parserGuard};

    return parser.evaluate(script, track);
}

QImage readImage(const QString& path, int requestedSize, const QString& hintType)
{
    const QMimeDatabase mimeDb;
    const auto mimeType   = mimeDb.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    const auto formatHint = mimeType.preferredSuffix().toLocal8Bit().toLower();

    QImageReader reader{path, formatHint};

    if(!reader.canRead()) {
        qCDebug(COV_PROV) << "Failed to use format hint" << formatHint << "when trying to load" << hintType << "cover";

        reader.setFormat({});
        reader.setFileName(path);
        if(!reader.canRead()) {
            qCDebug(COV_PROV) << "Failed to load" << hintType << "cover";
            return {};
        }
    }

    const auto size    = reader.size();
    const auto maxSize = requestedSize == 0 ? MaxSize : requestedSize;
    const auto dpr     = Fooyin::Utils::windowDpr();

    if(size.width() > maxSize || size.height() > maxSize || dpr > 1.0) {
        const auto scaledSize = calculateScaledSize(size, static_cast<int>(maxSize * dpr));
        reader.setScaledSize(scaledSize);
    }

    QImage image = reader.read();
    image.setDevicePixelRatio(dpr);

    return image;
}

QImage readImageOriginal(const QString& path, const QString& hintType)
{
    const QMimeDatabase mimeDb;
    const auto mimeType   = mimeDb.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    const auto formatHint = mimeType.preferredSuffix().toLocal8Bit().toLower();

    QImageReader reader{path, formatHint};

    if(!reader.canRead()) {
        qCDebug(COV_PROV) << "Failed to use format hint" << formatHint << "when trying to load" << hintType << "cover";

        reader.setFormat({});
        reader.setFileName(path);
        if(!reader.canRead()) {
            qCDebug(COV_PROV) << "Failed to load" << hintType << "cover";
            return {};
        }
    }

    return reader.read();
}

QImage readImage(QByteArray data)
{
    QBuffer buffer{&data};
    const QMimeDatabase mimeDb;
    const auto mimeType   = mimeDb.mimeTypeForData(&buffer);
    const auto formatHint = mimeType.preferredSuffix().toLocal8Bit().toLower();

    QImageReader reader{&buffer, formatHint};

    if(!reader.canRead()) {
        qCDebug(COV_PROV) << "Failed to use format hint" << formatHint << "when trying to load embedded cover";

        reader.setFormat({});
        reader.setDevice(&buffer);
        if(!reader.canRead()) {
            qCDebug(COV_PROV) << "Failed to load embedded cover";
            return {};
        }
    }

    const auto size = reader.size();
    if(size.width() > MaxSize || size.height() > MaxSize) {
        const auto scaledSize = calculateScaledSize(size, MaxSize);
        reader.setScaledSize(scaledSize);
    }

    return reader.read();
}

QImage readImageOriginal(QByteArray data)
{
    QBuffer buffer{&data};
    const QMimeDatabase mimeDb;
    const auto mimeType   = mimeDb.mimeTypeForData(&buffer);
    const auto formatHint = mimeType.preferredSuffix().toLocal8Bit().toLower();

    QImageReader reader{&buffer, formatHint};

    if(!reader.canRead()) {
        qCDebug(COV_PROV) << "Failed to use format hint" << formatHint << "when trying to load embedded cover";

        reader.setFormat({});
        reader.setDevice(&buffer);
        if(!reader.canRead()) {
            qCDebug(COV_PROV) << "Failed to load embedded cover";
            return {};
        }
    }

    return reader.read();
}

struct CoverLoader
{
    QString key;
    Fooyin::Track track;
    Fooyin::Track::Cover type;
    Fooyin::ArtworkSourcePreference sourcePreference{Fooyin::ArtworkSourcePreference::PreferDirectory};
    std::shared_ptr<Fooyin::AudioLoader> audioLoader;
    Fooyin::CoverPaths paths;
    bool isThumb{false};
    CoverProvider::ThumbnailSize size{CoverProvider::None};
    bool originalSize{false};
    QImage cover;
};

bool prefersEmbedded(const CoverLoader& loader)
{
    return loader.sourcePreference == Fooyin::ArtworkSourcePreference::PreferEmbedded;
}

bool hasImageInDirectory(CoverLoader& loader)
{
    const QString dirPath = findDirectoryCover(loader.paths, loader.track, loader.type);
    if(dirPath.isEmpty()) {
        return {};
    }

    const QFile file{dirPath};
    return file.size() > 0;
}

QImage loadImageFromDirectory(CoverLoader& loader)
{
    const QString dirPath = findDirectoryCover(loader.paths, loader.track, loader.type);
    if(dirPath.isEmpty()) {
        return {};
    }

    const QFile file{dirPath};
    if(file.size() == 0) {
        return {};
    }

    return loader.originalSize ? readImageOriginal(dirPath, u"directory"_s)
                               : readImage(dirPath, loader.size, u"directory"_s);
}

bool hasEmbeddedCover(const CoverLoader& loader)
{
    const QByteArray coverData = loader.audioLoader->readTrackCover(loader.track, loader.type);
    return !coverData.isEmpty();
}

QImage loadImageFromEmbedded(const CoverLoader& loader, const QString& cachePath)
{
    const QByteArray coverData = loader.audioLoader->readTrackCover(loader.track, loader.type);
    if(coverData.isEmpty()) {
        return {};
    }

    QImage cover = loader.originalSize ? readImageOriginal(coverData) : readImage(coverData);

    if(loader.isThumb && !cover.isNull() && !QFileInfo::exists(cachePath)) {
        if(!saveThumbnail(cover, loader.key)) {
            qCInfo(COV_PROV) << "Failed to save cover thumbnail for track:" << loader.track.filepath();
        }
        cover = Fooyin::Utils::scaleImage(cover, loader.size, Fooyin::Utils::windowDpr());
    }

    return cover;
}

bool hasCoverImage(CoverLoader loader)
{
    if(prefersEmbedded(loader)) {
        return hasEmbeddedCover(loader) || hasImageInDirectory(loader);
    }

    return hasImageInDirectory(loader) || hasEmbeddedCover(loader);
}

CoverLoader loadCoverImage(CoverLoader loader)
{
    CoverLoader result{loader};

    const QString cachePath = coverThumbnailPath(loader.key);

    // First check disk cache
    if(result.isThumb && QFileInfo::exists(cachePath)) {
        result.cover = readImage(cachePath, loader.size, u"cached"_s);
    }

    if(prefersEmbedded(loader)) {
        if(result.cover.isNull()) {
            result.cover = loadImageFromEmbedded(loader, cachePath);
        }
        if(result.cover.isNull()) {
            result.cover = loadImageFromDirectory(loader);
        }
    }
    else {
        if(result.cover.isNull()) {
            result.cover = loadImageFromDirectory(loader);
        }
        if(result.cover.isNull()) {
            result.cover = loadImageFromEmbedded(loader, cachePath);
        }
    }

    return result;
}
} // namespace

namespace Fooyin {
class FYGUI_NO_EXPORT CoverProvider::CoverProviderPrivate
{
public:
    explicit CoverProviderPrivate(CoverProvider* self, std::shared_ptr<AudioLoader> audioLoader,
                                  SettingsManager* settings);

    [[nodiscard]] QPixmap loadNoCover(ThumbnailSize size = None) const;
    void processCoverResult(const CoverLoader& loader);
    static QPixmap processLoadResult(const CoverLoader& loader);
    static QPixmap processOriginalLoadResult(const CoverLoader& loader);
    static void cachePixmap(const QString& key, const QPixmap& cover, int size = 0);
    void fetchCover(const QString& key, const Track& track, Track::Cover type, bool thumbnail,
                    ThumbnailSize size = None);
    [[nodiscard]] QFuture<QPixmap> loadCover(const Track& track, Track::Cover type) const;
    [[nodiscard]] QFuture<QPixmap> loadOriginalCover(const Track& track, Track::Cover type) const;
    [[nodiscard]] QFuture<QPixmap> loadThumbnail(const QString& key, const Track& track, ThumbnailSize size,
                                                 Track::Cover type) const;
    QPixmap loadCachedCover(const QString& key, int size = 0);
    [[nodiscard]] QFuture<bool> hasCover(const QString& key, const Track& track, Track::Cover type) const;
    [[nodiscard]] QString thumbnailCoverKey(const Track& track, Track::Cover type) const;
    bool shouldRetryNoCover(const QString& key) const;
    void clearNoCoverRetry(const QString& key) const;

    CoverProvider* m_self;
    std::shared_ptr<AudioLoader> m_audioLoader;
    SettingsManager* m_settings;

    bool m_usePlacerholder{true};
    std::set<QString> m_pendingCovers;
    mutable std::unordered_map<QString, int64_t> m_noCoverRetryAfterMs;

    CoverPaths m_paths;
    ArtworkSourcePreference m_sourcePreference;
    QString m_thumbnailGroupScript;
};

CoverProvider::CoverProviderPrivate::CoverProviderPrivate(CoverProvider* self, std::shared_ptr<AudioLoader> audioLoader,
                                                          SettingsManager* settings)
    : m_self{self}
    , m_audioLoader{std::move(audioLoader)}
    , m_settings{settings}
    , m_paths{m_settings->value<Settings::Gui::Internal::TrackCoverPaths>().value<CoverPaths>()}
    , m_sourcePreference{static_cast<ArtworkSourcePreference>(
          m_settings->value<Settings::Gui::Internal::TrackCoverSourcePreference>())}
    , m_thumbnailGroupScript{m_settings->value<Settings::Gui::Internal::TrackCoverThumbnailGroupScript>()}
{
    auto updateCache = [](const int sizeMb) {
        m_coverCache.setMaxCost(sizeMb * 1024LL * 1024LL);
    };

    updateCache(m_settings->value<Settings::Gui::Internal::PixmapCacheSize>());
    m_settings->subscribe<Settings::Gui::Internal::PixmapCacheSize>(m_self, updateCache);

    m_settings->subscribe<Settings::Gui::Internal::TrackCoverPaths>(
        m_self, [this](const QVariant& var) { m_paths = var.value<CoverPaths>(); });
    m_settings->subscribe<Settings::Gui::Internal::TrackCoverSourcePreference>(
        m_self, [this](int preference) { m_sourcePreference = static_cast<ArtworkSourcePreference>(preference); });
    m_settings->subscribe<Settings::Gui::Internal::TrackCoverThumbnailGroupScript>(m_self, [this](QString script) {
        m_thumbnailGroupScript = std::move(script);
        m_noCoverKeys.clear();
    });
    m_settings->subscribe<Settings::Gui::IconTheme>(m_self, []() {
        for(const auto size : {None, Tiny, Small, MediumSmall, Medium, Large, VeryLarge, ExtraLarge, Huge, Full}) {
            m_coverCache.remove(noCoverCacheKey(size));
        }
    });
}

QString CoverProvider::CoverProviderPrivate::thumbnailCoverKey(const Track& track, Track::Cover type) const
{
    QString group = evaluateThumbnailGroupScript(m_thumbnailGroupScript, track);
    if(group.isEmpty()) {
        group = track.albumHash();
    }

    return generateGroupedCoverKey(group, type, m_sourcePreference);
}

QPixmap CoverProvider::CoverProviderPrivate::loadNoCover(const ThumbnailSize size) const
{
    const int coverSize    = size == None ? MaxSize : static_cast<int>(size);
    const QString cacheKey = noCoverCacheKey(coverSize);
    if(auto* cover = m_coverCache.object(cacheKey)) {
        return *cover;
    }

    const QIcon icon = Fooyin::Gui::iconFromTheme(Fooyin::Constants::Icons::NoCover);
    const QSize requestedSize{coverSize, coverSize};

    auto* cover    = new QPixmap(icon.pixmap(requestedSize, Utils::windowDpr()));
    const int cost = cover->width() * cover->height() * cover->depth() / 8;

    if(m_coverCache.insert(cacheKey, cover, cost)) {
        return *cover;
    }

    return {};
}

void CoverProvider::CoverProviderPrivate::processCoverResult(const CoverLoader& loader)
{
    m_pendingCovers.erase(loader.key);
    clearNoCoverRetry(loader.key);
    m_noCoverKeys.erase(loader.key);

    if(loader.cover.isNull()) {
        m_noCoverKeys.emplace(loader.key);
        return;
    }

    auto* cover = new QPixmap(QPixmap::fromImage(loader.cover));
    cover->setDevicePixelRatio(Utils::windowDpr());

    const int cost = cover->width() * cover->height() * cover->depth() / 8;

    if(!m_coverCache.insert(loader.isThumb ? generateThumbCoverKey(loader.key, loader.size) : loader.key, cover,
                            cost)) {
        qCDebug(COV_PROV) << "Failed to cache cover for:" << loader.track.filepath();
    }

    Q_EMIT m_self->coverAdded(loader.track);
}

QPixmap CoverProvider::CoverProviderPrivate::processLoadResult(const CoverLoader& loader)
{
    if(loader.cover.isNull()) {
        return {};
    }

    QPixmap cover = QPixmap::fromImage(loader.cover);
    cover.setDevicePixelRatio(Utils::windowDpr());

    return cover;
}

QPixmap CoverProvider::CoverProviderPrivate::processOriginalLoadResult(const CoverLoader& loader)
{
    if(loader.cover.isNull()) {
        return {};
    }

    return QPixmap::fromImage(loader.cover);
}

void CoverProvider::CoverProviderPrivate::cachePixmap(const QString& key, const QPixmap& cover, int size)
{
    if(cover.isNull()) {
        return;
    }

    auto* cachedCover      = new QPixmap(cover);
    const int cost         = cachedCover->width() * cachedCover->height() * cachedCover->depth() / 8;
    const QString cacheKey = size == 0 ? key : generateThumbCoverKey(key, size);

    if(!m_coverCache.insert(cacheKey, cachedCover, cost)) {
        qCDebug(COV_PROV) << "Failed to cache cover for key:" << cacheKey;
    }
}

void CoverProvider::CoverProviderPrivate::fetchCover(const QString& key, const Track& track, Track::Cover type,
                                                     bool thumbnail, ThumbnailSize size)
{
    CoverLoader loader;
    loader.key              = key;
    loader.track            = track;
    loader.type             = type;
    loader.sourcePreference = m_sourcePreference;
    loader.audioLoader      = m_audioLoader;
    loader.paths            = m_paths;
    loader.isThumb          = thumbnail;
    loader.size             = size;

    auto loaderResult = Utils::asyncExec([loader]() -> CoverLoader {
        auto result = loadCoverImage(loader);
        return result;
    });
    loaderResult.then(m_self, [this, key, track](const CoverLoader& result) { processCoverResult(result); });
}

QFuture<QPixmap> CoverProvider::CoverProviderPrivate::loadCover(const Track& track, Track::Cover type) const
{
    CoverLoader loader;
    loader.track            = track;
    loader.type             = type;
    loader.sourcePreference = m_sourcePreference;
    loader.audioLoader      = m_audioLoader;
    loader.paths            = m_paths;

    auto loaderResult = Utils::asyncExec([loader]() -> CoverLoader {
        auto result = loadCoverImage(loader);
        return result;
    });
    return loaderResult.then(m_self, [track](const CoverLoader& result) { return processLoadResult(result); });
}

QFuture<QPixmap> CoverProvider::CoverProviderPrivate::loadOriginalCover(const Track& track, Track::Cover type) const
{
    CoverLoader loader;
    loader.track            = track;
    loader.type             = type;
    loader.sourcePreference = m_sourcePreference;
    loader.audioLoader      = m_audioLoader;
    loader.paths            = m_paths;
    loader.originalSize     = true;

    auto loaderResult = Utils::asyncExec([loader]() -> CoverLoader {
        auto result = loadCoverImage(loader);
        return result;
    });
    return loaderResult.then(m_self, [](const CoverLoader& result) { return processOriginalLoadResult(result); });
}

QFuture<QPixmap> CoverProvider::CoverProviderPrivate::loadThumbnail(const QString& key, const Track& track,
                                                                    ThumbnailSize size, Track::Cover type) const
{
    CoverLoader loader;
    loader.key              = key;
    loader.track            = track;
    loader.type             = type;
    loader.sourcePreference = m_sourcePreference;
    loader.audioLoader      = m_audioLoader;
    loader.paths            = m_paths;
    loader.isThumb          = true;
    loader.size             = size;

    auto loaderResult = Utils::asyncExec([loader]() -> CoverLoader {
        auto result = loadCoverImage(loader);
        return result;
    });

    return loaderResult.then(m_self, [this, key, size](const CoverLoader& result) {
        clearNoCoverRetry(key);
        m_noCoverKeys.erase(key);
        const QPixmap cover = processLoadResult(result);
        if(cover.isNull()) {
            m_noCoverKeys.emplace(key);
        }
        else {
            cachePixmap(key, cover, size);
        }
        return cover;
    });
}

QPixmap CoverProvider::CoverProviderPrivate::loadCachedCover(const QString& key, int size)
{
    const QString cacheKey = size == 0 ? key : generateThumbCoverKey(key, size);
    if(auto* cover = m_coverCache.object(cacheKey)) {
        return *cover;
    }

    return {};
}

QFuture<bool> CoverProvider::CoverProviderPrivate::hasCover(const QString& key, const Track& track,
                                                            Track::Cover type) const
{
    CoverLoader loader;
    loader.key              = key;
    loader.track            = track;
    loader.type             = type;
    loader.sourcePreference = m_sourcePreference;
    loader.audioLoader      = m_audioLoader;
    loader.paths            = m_paths;

    auto loaderResult = Utils::asyncExec([loader]() -> bool {
        const bool result = hasCoverImage(loader);
        return result;
    });

    return loaderResult;
}

bool CoverProvider::CoverProviderPrivate::shouldRetryNoCover(const QString& key) const
{
    const auto now = static_cast<int64_t>(QDateTime::currentMSecsSinceEpoch());
    if(const auto it = m_noCoverRetryAfterMs.find(key); it != m_noCoverRetryAfterMs.cend() && now < it->second) {
        return false;
    }

    m_noCoverRetryAfterMs.emplace(key, now + RetryIntervalMs);
    return true;
}

void CoverProvider::CoverProviderPrivate::clearNoCoverRetry(const QString& key) const
{
    m_noCoverRetryAfterMs.erase(key);
}

CoverProvider::CoverProvider(std::shared_ptr<AudioLoader> audioLoader, SettingsManager* settings, QObject* parent)
    : QObject{parent}
    , p{std::make_unique<CoverProviderPrivate>(this, std::move(audioLoader), settings)}
{ }

CoverProvider::~CoverProvider() = default;

void CoverProvider::setUsePlaceholder(bool enabled)
{
    p->m_usePlacerholder = enabled;
}

QFuture<bool> CoverProvider::trackHasCover(const Track& track, Track::Cover type) const
{
    if(!track.isValid()) {
        return Utils::asyncExec([] { return false; });
    }

    const QString coverKey = generateTrackCoverKey(track, type, p->m_sourcePreference);

    if(m_noCoverKeys.contains(coverKey)) {
        return Utils::asyncExec([] { return false; });
    }

    return p->hasCover(coverKey, track, type);
}

QPixmap CoverProvider::trackCover(const Track& track, Track::Cover type) const
{
    if(!track.isValid()) {
        return p->m_usePlacerholder ? p->loadNoCover() : QPixmap{};
    }

    const QString coverKey = generateTrackCoverKey(track, type, p->m_sourcePreference);
    if(!p->m_pendingCovers.contains(coverKey)) {
        QPixmap cover = p->loadCachedCover(coverKey);
        if(!cover.isNull()) {
            return cover;
        }

        p->m_pendingCovers.emplace(coverKey);
        p->fetchCover(coverKey, track, type, false);
    }

    return p->m_usePlacerholder ? p->loadNoCover() : QPixmap{};
}

QFuture<QPixmap> CoverProvider::trackCoverFull(const Track& track, Track::Cover type) const
{
    if(!track.isValid()) {
        return Utils::asyncExec([] { return QPixmap{}; });
    }

    return p->loadCover(track, type);
}

QFuture<QPixmap> CoverProvider::trackCoverOriginal(const Track& track, Track::Cover type) const
{
    if(!track.isValid()) {
        return Utils::asyncExec([] { return QPixmap{}; });
    }

    return p->loadOriginalCover(track, type);
}

QFuture<QPixmap> CoverProvider::trackCoverThumbnailAsync(const Track& track, ThumbnailSize size,
                                                         Track::Cover type) const
{
    if(!track.isValid()) {
        return makeReadyFuture(QPixmap{});
    }

    const QString coverKey = p->thumbnailCoverKey(track, type);
    if(m_noCoverKeys.contains(coverKey)) {
        if(!p->shouldRetryNoCover(coverKey)) {
            return makeReadyFuture(QPixmap{});
        }

        m_noCoverKeys.erase(coverKey);
    }

    if(const QPixmap cover = p->loadCachedCover(coverKey, size); !cover.isNull()) {
        return makeReadyFuture(cover);
    }

    return p->loadThumbnail(coverKey, track, size, type);
}

QFuture<QPixmap> CoverProvider::trackCoverThumbnailAsync(const Track& track, const QSize& size, Track::Cover type) const
{
    return trackCoverThumbnailAsync(track, findThumbnailSize(size), type);
}

QPixmap CoverProvider::trackCoverThumbnail(const Track& track, ThumbnailSize size, Track::Cover type) const
{
    if(!track.isValid()) {
        return p->m_usePlacerholder ? p->loadNoCover(size) : QPixmap{};
    }

    const QString coverKey = p->thumbnailCoverKey(track, type);
    if(m_noCoverKeys.contains(coverKey)) {
        if(!p->m_pendingCovers.contains(coverKey) && p->shouldRetryNoCover(coverKey)) {
            m_noCoverKeys.erase(coverKey);
            p->m_pendingCovers.emplace(coverKey);
            p->fetchCover(coverKey, track, type, true, size);
        }

        return p->m_usePlacerholder ? p->loadNoCover(size) : QPixmap{};
    }

    if(!p->m_pendingCovers.contains(coverKey)) {
        QPixmap cover = p->loadCachedCover(coverKey, size);
        if(!cover.isNull()) {
            return cover;
        }

        p->m_pendingCovers.emplace(coverKey);
        p->fetchCover(coverKey, track, type, true, size);
    }

    return p->m_usePlacerholder ? p->loadNoCover(size) : QPixmap{};
}

QPixmap CoverProvider::trackCoverThumbnail(const Track& track, const QSize& size, Track::Cover type) const
{
    return trackCoverThumbnail(track, findThumbnailSize(size), type);
}

QPixmap CoverProvider::placeholderCover() const
{
    return p->loadNoCover();
}

CoverProvider::ThumbnailSize CoverProvider::findThumbnailSize(const QSize& size)
{
    const int maxSize = std::max(size.width(), size.height());

    if(maxSize <= 32) {
        return Small;
    }
    if(maxSize <= 64) {
        return MediumSmall;
    }
    if(maxSize <= 96) {
        return Medium;
    }
    if(maxSize <= 128) {
        return Large;
    }
    if(maxSize <= 192) {
        return VeryLarge;
    }
    if(maxSize <= 256) {
        return ExtraLarge;
    }
    if(maxSize <= 512) {
        return Huge;
    }

    return Full;
}

void CoverProvider::clearCache()
{
    QDir cache{Gui::coverPath()};
    cache.removeRecursively();

    m_coverCache.clear();
    m_noCoverKeys.clear();
}

void CoverProvider::removeFromCache(const Track& track, const SettingsManager& settings)
{
    const auto thumbnailGroupScript = settings.value<Settings::Gui::Internal::TrackCoverThumbnailGroupScript>();
    QString group                   = evaluateThumbnailGroupScript(thumbnailGroupScript, track);
    if(group.isEmpty()) {
        group = track.albumHash();
    }

    removeFromCache(track, group);
}

void CoverProvider::removeFromCache(const Track& track)
{
    removeFromCache(track, track.albumHash());
}

void CoverProvider::removeFromCache(const Track& track, const QString& thumbnailGroup)
{
    auto removeKey = [](const QString& key) {
        QDir cache{Fooyin::Gui::coverPath()};
        cache.remove(coverThumbnailPath(key));
        m_noCoverKeys.erase(key);
        m_coverCache.remove(key);

        for(const auto size : {Tiny, Small, MediumSmall, Medium, Large, VeryLarge, ExtraLarge, Huge}) {
            m_coverCache.remove(generateThumbCoverKey(key, size));
        }
    };

    for(const auto type : {Track::Cover::Front, Track::Cover::Back, Track::Cover::Artist}) {
        for(const auto sourcePreference :
            {ArtworkSourcePreference::PreferDirectory, ArtworkSourcePreference::PreferEmbedded}) {
            removeKey(generateGroupedCoverKey(thumbnailGroup, type, sourcePreference));
            removeKey(generateTrackCoverKey(track, type, sourcePreference));
        }
    }
}
} // namespace Fooyin

#include "gui/moc_coverprovider.cpp"
