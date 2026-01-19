#pragma once
#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QMenu>
#include <QWebEngineView>
#include <QWebEnginePage>
#include "../backend/YouTubeService.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void performSearch();
    void handleSearchResults(const QList<VideoResult> &results);
    void openInNewTab(QListWidgetItem *item);
    void openVideoById(const QString &videoId, const QString &title);
    void showContextMenu(const QPoint &pos);
    void showError(const QString &msg);

private:
    YouTubeService *m_service;
    QLineEdit *m_searchInput;
    QListWidget *m_videoList;
    QPushButton *m_searchBtn;
    QTabWidget *m_tabs;
    QWidget *m_searchTab;
    QWebEngineView *m_youtubeHome;

    void fetchThumbnail(const QString &url, QListWidgetItem *item);
};
