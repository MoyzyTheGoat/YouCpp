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
    QString thumbnailUrl;
};

class YouTubeService : public QObject {
    Q_OBJECT

public:
    explicit YouTubeService(QObject *parent = nullptr);
    void searchVideos(const QString &query);

signals:
    void searchResultsReady(const QList<VideoResult> &results);
    void errorOccurred(const QString &message);

private slots:
    void onSearchReply(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_manager;
    QString m_apiKey;
};
