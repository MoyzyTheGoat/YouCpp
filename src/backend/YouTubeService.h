#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
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
    void fetchTranscript(const QString &videoId);

signals:
    void searchResultsReady(const QList<VideoResult> &results);
    void transcriptReady(const QString &text);
    void errorOccurred(const QString &message);

private slots:
    void onSearchReply(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_manager;
    QString m_apiKey;
    
    QString parseVttContent(const QString &rawText);
};
