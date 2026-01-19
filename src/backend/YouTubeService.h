#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

struct VideoResult {
    QString id;
    QString title;
    QString channel;
    QString channelId;
    QString thumbnailUrl;
    QString publishedAt;
    
    unsigned long long viewCount = 0;
    unsigned long long likeCount = 0;
    QString duration;
};

class YouTubeService : public QObject {
    Q_OBJECT

public:
    explicit YouTubeService(QObject *parent = nullptr);
    
    // Public search (uses API key)
    void searchVideos(const QString &query);
    
    // Authenticated endpoints (require access token)
    void setAccessToken(const QString &token);
    void fetchSubscriptionsFeed();
    void fetchRecommendations();
    
    // Channel Muting
    void muteChannel(const QString &channelId, const QString &channelName);
    void unmuteChannel(const QString &channelId);
    bool isChannelMuted(const QString &channelId) const;
    QStringList getMutedChannels() const;

signals:
    void searchResultsReady(const QList<VideoResult> &results);
    void subscriptionFeedReady(const QList<VideoResult> &results);
    void recommendationsReady(const QList<VideoResult> &results);
    void errorOccurred(const QString &message);

private slots:
    void onSearchReply(QNetworkReply *reply);

private:
    void parseVideoList(QNetworkReply *reply, void (YouTubeService::*signal)(const QList<VideoResult> &));
    QList<VideoResult> parseVideosFromJson(const QJsonDocument &doc);
    
    QNetworkAccessManager *m_manager;
    QString m_apiKey;
    QString m_accessToken;
    
    int m_pendingFeedRequests = 0;
    QList<VideoResult> m_accumulatedFeedResults;

    void fetchVideoStatistics(QList<QString> videoIds);
    void loadSettings();
    void saveSettings();
    QSet<QString> m_mutedChannelIds;
};
