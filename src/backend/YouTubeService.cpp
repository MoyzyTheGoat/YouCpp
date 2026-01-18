#include "YouTubeService.h"
#include <QUrlQuery>
#include <QDebug>


YouTubeService::YouTubeService(QObject *parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);
    m_apiKey = qEnvironmentVariable("YOUTUBE_API_KEY"); 
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

void YouTubeService::onSearchReply(QNetworkReply *reply) {
    if (reply->error()) {
        emit errorOccurred("Network Error: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray items = doc.object()["items"].toArray();
    
    QList<VideoResult> results;
    for (const auto &item : items) {
        QJsonObject obj = item.toObject();
        VideoResult vid;
        vid.id = obj["id"].toObject()["videoId"].toString();
        vid.title = obj["snippet"].toObject()["title"].toString();
        vid.channel = obj["snippet"].toObject()["channelTitle"].toString();
        vid.thumbnailUrl = obj["snippet"].toObject()["thumbnails"]
                                      .toObject()["default"]
                                      .toObject()["url"].toString();
        results.append(vid);
    }
    
    emit searchResultsReady(results);
    reply->deleteLater();
}


