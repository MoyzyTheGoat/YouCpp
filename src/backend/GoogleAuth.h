#pragma once
#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QSettings>

class GoogleAuth : public QObject {
    Q_OBJECT

public:
    explicit GoogleAuth(QObject *parent = nullptr);
    ~GoogleAuth();

    void setCredentials(const QString &clientId, const QString &clientSecret);
    void startLogin();
    void logout();
    
    bool isAuthenticated() const;
    QString accessToken() const;

signals:
    void authenticated();
    void authenticationFailed(const QString &error);
    void loggedOut();

private slots:
    void handleNewConnection();
    void exchangeCodeForToken(const QString &code);

private:
    void loadTokens();
    void saveTokens();
    void refreshAccessToken();
    
    QString m_clientId;
    QString m_clientSecret;
    QString m_accessToken;
    QString m_refreshToken;
    
    QNetworkAccessManager *m_networkManager;
    QTcpServer *m_callbackServer;
    int m_callbackPort = 0;
    
    static const QString AUTH_URL;
    static const QString TOKEN_URL;
    static const QString SCOPE;
};
