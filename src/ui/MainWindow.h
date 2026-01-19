#pragma once
#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QMenu>
#include <QStackedWidget>
#include "../backend/YouTubeService.h"
#include "../backend/GoogleAuth.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void performSearch();
    void handleSearchResults(const QList<VideoResult> &results);
    void handleSubscriptionFeed(const QList<VideoResult> &results);
    void handleRecommendations(const QList<VideoResult> &results);
    void openVideoFromItem(QListWidgetItem *item);
    void openVideoById(const QString &videoId, const QString &title);
    void showContextMenu(const QPoint &pos);
    void showError(const QString &msg);
    
    void onSignInClicked();
    void onAuthenticated();
    void onAuthFailed(const QString &error);
    void onLoggedOut();

private:
    void setupHomeTab();
    void updateAuthUI();
    void populateVideoList(QListWidget *list, const QList<VideoResult> &results);
    void fetchThumbnail(const QString &url, QListWidgetItem *item);
    
    YouTubeService *m_service;
    GoogleAuth *m_auth;
    
    QTabWidget *m_tabs;
    
    // Home tab
    QWidget *m_homeTab;
    QStackedWidget *m_homeStack;
    QWidget *m_signInPage;
    QWidget *m_feedPage;
    QPushButton *m_signInBtn;
    QPushButton *m_signOutBtn;
    QLabel *m_authStatusLabel;
    QListWidget *m_feedList;
    
    // Search tab
    QWidget *m_searchTab;
    QLineEdit *m_searchInput;
    QListWidget *m_videoList;
    QPushButton *m_searchBtn;
};
