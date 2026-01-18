#include "YouTubeService.h"
#include <QUrlQuery>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QRegularExpression>

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
//transcript fetch
void YouTubeService::fetchTranscript(const QString &videoId) {
    QString tempPath = QDir::tempPath() + "/youcaption_" + videoId;
    QDir().mkpath(tempPath); 

    QProcess *process = new QProcess(this);
    QString program = "yt-dlp";
    QStringList arguments;
    arguments << "--skip-download"
              << "--write-auto-subs"
              << "--write-subs"
              << "--sub-lang" << "en.*"
              << "--sub-format" << "vtt"
              << "--no-warnings"
              << "--no-playlist"            
              << "--no-check-certificates"   
              << "--quiet"
              << "-o" << tempPath + "/sub.%(ext)s" 
              << "https://www.youtube.com/watch?v=" + videoId;

    connect(process, &QProcess::finished, this, [this, process, tempPath](int exitCode) {
        QString transcriptText;

        if (exitCode == 0) {
            QDir dir(tempPath);
            QStringList files = dir.entryList({"*.vtt"}, QDir::Files);

            if (!files.isEmpty()) {
                QFile file(dir.absoluteFilePath(files.first()));
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&file);
                    QString rawContent = in.readAll();
                    transcriptText = parseVttContent(rawContent); 
                    file.close();
                }
            } else {
                transcriptText = "Subtitles were found but could not be processed.";
            }
        } else {
            QString error = process->readAllStandardError();
            transcriptText = "yt-dlp error: " + error;
        }

        QDir(tempPath).removeRecursively();

        emit transcriptReady(transcriptText.isEmpty() ? "No text found." : transcriptText);
        process->deleteLater();
    });

    process->start(program, arguments);
}


QString YouTubeService::parseVttContent(const QString &rawText) {
    QString formattedText;
    QRegularExpression timestampRegex("<[^>]*>");
    QStringList lines = rawText.split('\n');
    
    int wordCount = 0;

    for (const QString &line : lines) {
        QString t = line.trimmed();
        t.remove(timestampRegex);
        
        if (t.startsWith("WEBVTT") || t.startsWith("Kind:") || 
            t.startsWith("Language:") || t.contains("-->") || t.isEmpty()) {
            continue;
        }

        if (!formattedText.isEmpty()) {
            QString lastBit = formattedText.right(60);
            if (lastBit.contains(t)) continue;
        }

        formattedText.append(t + " ");
        wordCount += t.split(' ').size();

        if (wordCount > 20) {
            formattedText.append("\n\n");
            wordCount = 0;
        }
    }
    
    return formattedText.trimmed();
}
