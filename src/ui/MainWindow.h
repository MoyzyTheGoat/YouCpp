#pragma once
#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QMenu>
#include "../backend/YouTubeService.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void performSearch();
    void handleSearchResults(const QList<VideoResult> &results);
    void openInNewTab(QListWidgetItem *item);
    void showContextMenu(const QPoint &pos);
    void showError(const QString &msg);

private:
    YouTubeService *m_service;
    QLineEdit *m_searchInput;
    QListWidget *m_videoList;
    QPushButton *m_searchBtn;
    QTabWidget *m_tabs;
    QWidget *m_searchTab;

    void fetchThumbnail(const QString &url, QListWidgetItem *item);
};
