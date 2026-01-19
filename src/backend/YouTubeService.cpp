#include "YouTubeService.h"
#include <QUrlQuery>
#include <QDebug>
#include <cstdio>
#include <algorithm>

#include <QSettings>
#include <cmath>
#include <QDateTime>

YouTubeService::YouTubeService(QObject *parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);
    m_apiKey = qEnvironmentVariable("YOUTUBE_API_KEY");
    loadSettings();
}

void YouTubeService::setAccessToken(const QString &token) {
    m_accessToken = token;
}

void YouTubeService::searchVideos(const QString &query) {
    if (m_apiKey.isEmpty()) {
        emit errorOccurred("API Key missing. Please set YOUTUBE_API_KEY.");
        return;
    }

    QUrl url("https://www.googleapis.com/youtube/v3/search");
    QUrlQuery q;
    q.addQueryItem("part", "snippet");
    q.addQueryItem("maxResults", "25");
    q.addQueryItem("q", query);
    q.addQueryItem("type", "video");
    q.addQueryItem("key", m_apiKey);
    url.setQuery(q);

    QNetworkReply *reply = m_manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        this->onSearchReply(reply);
    });
}

void YouTubeService::fetchSubscriptionsFeed() {
    if (m_accessToken.isEmpty()) {
        emit errorOccurred("Not authenticated. Please sign in first.");
        return;
    }

    printf("[YouTubeService] Fetching subscriptions...\n");
    fflush(stdout);

    m_pendingFeedRequests = 0;
    m_accumulatedFeedResults.clear();

    QUrl url("https://www.googleapis.com/youtube/v3/subscriptions");
    QUrlQuery q;
    q.addQueryItem("part", "snippet");
    q.addQueryItem("mine", "true");
    q.addQueryItem("maxResults", "50");
    url.setQuery(q);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_accessToken).toUtf8());

    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error()) {
            printf("[YouTubeService] Subscriptions ERROR: %s\n", reply->errorString().toUtf8().constData());
            emit errorOccurred("Failed to fetch subscriptions: " + reply->errorString());
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray items = doc.object()["items"].toArray();
        
        printf("[YouTubeService] Found %d subscriptions\n", items.size());
        fflush(stdout);
        
        QStringList channelIds;
        for (const auto &item : items) {
            QString channelId = item.toObject()["snippet"].toObject()
                               ["resourceId"].toObject()["channelId"].toString();
            if (!channelId.isEmpty()) {
                channelIds.append(channelId);
            }
        }
        reply->deleteLater();

        if (channelIds.isEmpty()) {
            emit subscriptionFeedReady({});
            return;
        }

        QUrl channelsUrl("https://www.googleapis.com/youtube/v3/channels");
        QUrlQuery cq;
        cq.addQueryItem("part", "contentDetails");
        cq.addQueryItem("id", channelIds.join(","));
        cq.addQueryItem("maxResults", "50");
        if (!m_apiKey.isEmpty()) {
            cq.addQueryItem("key", m_apiKey);
        }
        channelsUrl.setQuery(cq);

        QNetworkRequest channelsRequest(channelsUrl);
        channelsRequest.setRawHeader("Authorization", QString("Bearer %1").arg(m_accessToken).toUtf8());

        QNetworkReply *channelsReply = m_manager->get(channelsRequest);
        connect(channelsReply, &QNetworkReply::finished, this, [this, channelsReply]() {
            if (channelsReply->error()) {
                printf("[YouTubeService] Channels ERROR: %s\n", channelsReply->errorString().toUtf8().constData());
                emit errorOccurred("Failed to fetch channel details");
                channelsReply->deleteLater();
                return;
            }

            QJsonDocument colDoc = QJsonDocument::fromJson(channelsReply->readAll());
            QJsonArray channels = colDoc.object()["items"].toArray();
            
            QStringList uploadPlaylistIds;
            for (const auto &item : channels) {
                QString playlistId = item.toObject()["contentDetails"].toObject()
                                        ["relatedPlaylists"].toObject()["uploads"].toString();
                if (!playlistId.isEmpty()) {
                    uploadPlaylistIds.append(playlistId);
                }
            }
            channelsReply->deleteLater();
            
            printf("[YouTubeService] Found %d upload playlists. Fetching videos...\n", uploadPlaylistIds.size());
            fflush(stdout);

            if (uploadPlaylistIds.isEmpty()) {
                emit subscriptionFeedReady({});
                return;
            }

            int limit = std::min((int)uploadPlaylistIds.size(), 20);
            m_pendingFeedRequests = limit;
            
            for (int i = 0; i < limit; ++i) {
                QUrl playlistUrl("https://www.googleapis.com/youtube/v3/playlistItems");
                QUrlQuery pq;
                pq.addQueryItem("part", "snippet");
                pq.addQueryItem("playlistId", uploadPlaylistIds[i]);
                pq.addQueryItem("maxResults", "5");
                if (!m_apiKey.isEmpty()) {
                    pq.addQueryItem("key", m_apiKey);
                }
                playlistUrl.setQuery(pq);

                QNetworkRequest plRequest(playlistUrl);
                plRequest.setRawHeader("Authorization", QString("Bearer %1").arg(m_accessToken).toUtf8());
                
                QNetworkReply *plReply = m_manager->get(plRequest);
                connect(plReply, &QNetworkReply::finished, this, [this, plReply]() {
                    if (plReply->error() == QNetworkReply::NoError) {
                        QJsonDocument plDoc = QJsonDocument::fromJson(plReply->readAll());
                        QJsonArray plItems = plDoc.object()["items"].toArray();
                        
                        for (const auto &item : plItems) {
                            QJsonObject snip = item.toObject()["snippet"].toObject();
                            VideoResult vid;
                            vid.id = snip["resourceId"].toObject()["videoId"].toString();
                            vid.title = snip["title"].toString();
                            vid.channel = snip["channelTitle"].toString();
                            vid.channelId = snip["channelId"].toString();
                            vid.thumbnailUrl = snip["thumbnails"].toObject()["medium"].toObject()["url"].toString();
                            vid.publishedAt = snip["publishedAt"].toString();
                            
                            bool isMuted = m_mutedChannelIds.contains(vid.channelId);
                            if (!vid.title.contains("Private video") && !vid.title.contains("Deleted video") && !isMuted) {
                                m_accumulatedFeedResults.append(vid);
                            }
                        }
                    } else {
                        printf("[YouTubeService] Playlist fetch error: %s\n", plReply->errorString().toUtf8().constData());
                    }
                    plReply->deleteLater();
                    
                    m_pendingFeedRequests--;
                    if (m_pendingFeedRequests <= 0) {
                        QStringList batchIds;
                        for(const auto &v : m_accumulatedFeedResults) {
                            batchIds.append(v.id);
                        }
                        
                        
                        fetchVideoStatistics(batchIds);
                    }
                });
            }
        });
    });
}

void YouTubeService::fetchRecommendations() {
    if (m_accessToken.isEmpty()) {
        emit errorOccurred("Not authenticated. Please sign in first.");
        return;
    }

    emit recommendationsReady({});
}

void YouTubeService::fetchVideoStatistics(QList<QString> videoIds) {
    if (videoIds.isEmpty()) {
        emit subscriptionFeedReady(m_accumulatedFeedResults);
        return;
    }

    if (videoIds.size() > 50) videoIds = videoIds.mid(0, 50);

    QUrl url("https://www.googleapis.com/youtube/v3/videos");
    QUrlQuery q;
    q.addQueryItem("part", "statistics,contentDetails");
    q.addQueryItem("id", videoIds.join(","));
    if (!m_apiKey.isEmpty()) {
        q.addQueryItem("key", m_apiKey);
    }
    url.setQuery(q);

    QNetworkRequest request(url);
    if (!m_accessToken.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_accessToken).toUtf8());
    }

    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, videoIds]() {
        if (reply->error()) {
            printf("[YouTubeService] Stats fetch error: %s\n", reply->errorString().toUtf8().constData());
            emit subscriptionFeedReady(m_accumulatedFeedResults);
        } else {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonArray items = doc.object()["items"].toArray();
            
            QMap<QString, QJsonObject> statsMap;
            for(const auto &item : items) {
                QJsonObject obj = item.toObject();
                statsMap.insert(obj["id"].toString(), obj);
            }

            for(auto &vid : m_accumulatedFeedResults) {
                if (statsMap.contains(vid.id)) {
                    QJsonObject stats = statsMap[vid.id]["statistics"].toObject();
                    vid.viewCount = stats["viewCount"].toString().toULongLong();
                    vid.likeCount = stats["likeCount"].toString().toULongLong();
                    
                    QJsonObject content = statsMap[vid.id]["contentDetails"].toObject();
                    vid.duration = content["duration"].toString();
                }
            }

            QDateTime now = QDateTime::currentDateTime();
            
            std::sort(m_accumulatedFeedResults.begin(), m_accumulatedFeedResults.end(), 
                [now](const VideoResult &a, const VideoResult &b) {
                    QDateTime da = QDateTime::fromString(a.publishedAt, Qt::ISODate);
                    QDateTime db = QDateTime::fromString(b.publishedAt, Qt::ISODate);
                    
                    double hoursA = da.secsTo(now) / 3600.0;
                    double hoursB = db.secsTo(now) / 3600.0;
                    if (hoursA < 0) hoursA = 0;
                    if (hoursB < 0) hoursB = 0;
                    
                    double scoreA = (double)a.viewCount / std::pow(hoursA + 2.0, 1.5);
                    double scoreB = (double)b.viewCount / std::pow(hoursB + 2.0, 1.5);
                    
                    return scoreA > scoreB;
                });
                
             printf("[YouTubeService] Smart sorted %d videos\n", m_accumulatedFeedResults.size());
             fflush(stdout);
             emit subscriptionFeedReady(m_accumulatedFeedResults);
        }
        reply->deleteLater();
    });
}

void YouTubeService::muteChannel(const QString &channelId, const QString &channelName) {
    if (channelId.isEmpty()) return;
    
    if (!m_mutedChannelIds.contains(channelId)) {
        m_mutedChannelIds.insert(channelId);
        saveSettings();
        printf("[YouTubeService] Muted channel: %s (%s)\n", channelName.toUtf8().constData(), channelId.toUtf8().constData());
    }
}

void YouTubeService::unmuteChannel(const QString &channelId) {
    if (m_mutedChannelIds.remove(channelId)) {
        saveSettings();
        printf("[YouTubeService] Unmuted channel: %s\n", channelId.toUtf8().constData());
    }
}

bool YouTubeService::isChannelMuted(const QString &channelId) const {
    return m_mutedChannelIds.contains(channelId);
}

QStringList YouTubeService::getMutedChannels() const {
    return m_mutedChannelIds.values();
}

void YouTubeService::loadSettings() {
    QSettings settings("YouCpp", "YouCpp");
    QStringList list = settings.value("mutedChannels").toStringList();
    m_mutedChannelIds = QSet<QString>(list.begin(), list.end());
}

void YouTubeService::saveSettings() {
    QSettings settings("YouCpp", "YouCpp");
    settings.setValue("mutedChannels", QStringList(m_mutedChannelIds.values()));
}

void YouTubeService::onSearchReply(QNetworkReply *reply) {
    if (reply->error()) {
        emit errorOccurred("Network Error: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QList<VideoResult> results = parseVideosFromJson(doc);
    emit searchResultsReady(results);
    reply->deleteLater();
}

QList<VideoResult> YouTubeService::parseVideosFromJson(const QJsonDocument &doc) {
    QJsonArray items = doc.object()["items"].toArray();
    
    QList<VideoResult> results;
    for (const auto &item : items) {
        QJsonObject obj = item.toObject();
        VideoResult vid;
        
        // Handle both search results (id is object) and direct video results (id is string)
        QJsonValue idValue = obj["id"];
        if (idValue.isObject()) {
            vid.id = idValue.toObject()["videoId"].toString();
        } else {
        vid.id = idValue.toString();
        }
        
        vid.title = obj["snippet"].toObject()["title"].toString();
        vid.channel = obj["snippet"].toObject()["channelTitle"].toString();
         vid.channelId = obj["snippet"].toObject()["channelId"].toString();
        vid.thumbnailUrl = obj["snippet"].toObject()["thumbnails"]
                           .toObject()["medium"]
                           .toObject()["url"].toString();
        if (vid.thumbnailUrl.isEmpty()) {
            vid.thumbnailUrl = obj["snippet"].toObject()["thumbnails"]
                               .toObject()["default"]
                               .toObject()["url"].toString();
        }
        vid.publishedAt = obj["snippet"].toObject()["publishedAt"].toString();
        
        if (!vid.id.isEmpty()) {
            results.append(vid);
        }
    }
    
    return results;
}
