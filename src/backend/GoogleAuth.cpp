#include "GoogleAuth.h"
#include <QRegularExpression>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QTcpSocket>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>

const QString GoogleAuth::AUTH_URL = "https://accounts.google.com/o/oauth2/v2/auth";
const QString GoogleAuth::TOKEN_URL = "https://oauth2.googleapis.com/token";
const QString GoogleAuth::SCOPE = "https://www.googleapis.com/auth/youtube.readonly";

GoogleAuth::GoogleAuth(QObject *parent) 
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_callbackServer(new QTcpServer(this))
{
    connect(m_callbackServer, &QTcpServer::newConnection, 
            this, &GoogleAuth::handleNewConnection);
    
    loadTokens();
}

GoogleAuth::~GoogleAuth() {
    if (m_callbackServer->isListening()) {
        m_callbackServer->close();
    }
}

void GoogleAuth::setCredentials(const QString &clientId, const QString &clientSecret) {
    m_clientId = clientId;
    m_clientSecret = clientSecret;
}

void GoogleAuth::startLogin() {
    if (m_clientId.isEmpty() || m_clientSecret.isEmpty()) {
        emit authenticationFailed("OAuth credentials not configured");
        return;
    }

    if (!m_callbackServer->listen(QHostAddress::LocalHost, 0)) {
        emit authenticationFailed("Failed to start callback server");
        return;
    }
    m_callbackPort = m_callbackServer->serverPort();
    

    QUrl authUrl(AUTH_URL);
    QUrlQuery query;
    query.addQueryItem("client_id", m_clientId);
    query.addQueryItem("redirect_uri", QString("http://127.0.0.1:%1").arg(m_callbackPort));
    query.addQueryItem("response_type", "code");
    query.addQueryItem("scope", SCOPE);
    query.addQueryItem("access_type", "offline");
    query.addQueryItem("prompt", "consent");
    authUrl.setQuery(query);
    
    // Open system browser
    QDesktopServices::openUrl(authUrl);
}

void GoogleAuth::handleNewConnection() {
    QTcpSocket *socket = m_callbackServer->nextPendingConnection();
    if (!socket) return;
    
    connect(socket, &QTcpSocket::readyRead, [this, socket]() {
        QString request = QString::fromUtf8(socket->readAll());
        
        QRegularExpression codeRegex(R"(code=([^&\s]+))");
        QRegularExpressionMatch match = codeRegex.match(request);
        
        QString responseHtml;
        if (match.hasMatch()) {
            QString code = QUrl::fromPercentEncoding(match.captured(1).toUtf8());
            responseHtml = R"(
                <html><body style="font-family: system-ui; text-align: center; padding: 50px;">
                <h1 style="color: #4CAF50;">✓ Authentication Successful!</h1>
                <p>You can close this window and return to YouCpp.</p>
                </body></html>
            )";
            exchangeCodeForToken(code);
        } else {
            responseHtml = R"(
                <html><body style="font-family: system-ui; text-align: center; padding: 50px;">
                <h1 style="color: #f44336;">✗ Authentication Failed</h1>
                <p>Please try again.</p>
                </body></html>
            )";
            emit authenticationFailed("No authorization code received");
        }
        
        QString response = QString(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"
            "\r\n%1"
        ).arg(responseHtml);
        
        socket->write(response.toUtf8());
        socket->flush();
        socket->disconnectFromHost();
    });
    
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}

void GoogleAuth::exchangeCodeForToken(const QString &code) {
    m_callbackServer->close();
    
    QUrl tokenUrl(TOKEN_URL);
    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    QUrlQuery postData;
    postData.addQueryItem("code", code);
    postData.addQueryItem("client_id", m_clientId);
    postData.addQueryItem("client_secret", m_clientSecret);
    postData.addQueryItem("redirect_uri", QString("http://127.0.0.1:%1").arg(m_callbackPort));
    postData.addQueryItem("grant_type", "authorization_code");
    
    QNetworkReply *reply = m_networkManager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            emit authenticationFailed(reply->errorString());
            return;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        
        m_accessToken = obj["access_token"].toString();
        m_refreshToken = obj["refresh_token"].toString();
        
        if (m_accessToken.isEmpty()) {
            emit authenticationFailed("No access token in response");
            return;
        }
        
        saveTokens();
        emit authenticated();
    });
}

void GoogleAuth::logout() {
    m_accessToken.clear();
    m_refreshToken.clear();
    
    QSettings settings("YouCpp", "YouCpp");
    settings.remove("auth/accessToken");
    settings.remove("auth/refreshToken");
    
    emit loggedOut();
}

bool GoogleAuth::isAuthenticated() const {
    return !m_accessToken.isEmpty();
}

QString GoogleAuth::accessToken() const {
    return m_accessToken;
}

void GoogleAuth::loadTokens() {
    QSettings settings("YouCpp", "YouCpp");
    m_accessToken = settings.value("auth/accessToken").toString();
    m_refreshToken = settings.value("auth/refreshToken").toString();

    if (m_accessToken.isEmpty() && !m_refreshToken.isEmpty()) {
        refreshAccessToken();
    }
}

void GoogleAuth::saveTokens() {
    QSettings settings("YouCpp", "YouCpp");
    settings.setValue("auth/accessToken", m_accessToken);
    settings.setValue("auth/refreshToken", m_refreshToken);
}

void GoogleAuth::refreshAccessToken() {
    if (m_refreshToken.isEmpty() || m_clientId.isEmpty() || m_clientSecret.isEmpty()) {
        return;
    }
    
    QUrl tokenUrl(TOKEN_URL);
    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    QUrlQuery postData;
    postData.addQueryItem("refresh_token", m_refreshToken);
    postData.addQueryItem("client_id", m_clientId);
    postData.addQueryItem("client_secret", m_clientSecret);
    postData.addQueryItem("grant_type", "refresh_token");
    
    QNetworkReply *reply = m_networkManager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            return;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        
        QString newAccessToken = obj["access_token"].toString();
        if (!newAccessToken.isEmpty()) {
            m_accessToken = newAccessToken;
            saveTokens();
            emit authenticated();
        }
    });
}
