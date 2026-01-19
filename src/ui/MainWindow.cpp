#include "MainWindow.h"
#include "TranscriptWindow.h"
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QRegularExpression>

// Custom page to intercept navigation and open videos in embedded tabs
class YouTubeHomePage : public QWebEnginePage {
public:
    YouTubeHomePage(MainWindow *mainWindow, QObject *parent = nullptr)
        : QWebEnginePage(parent), m_mainWindow(mainWindow) {}

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override {
        QString urlStr = url.toString();
        
        // Check if this is a video URL
        static QRegularExpression videoRegex(R"(/watch\?v=([a-zA-Z0-9_-]{11}))");
        QRegularExpressionMatch match = videoRegex.match(urlStr);
        
        if (match.hasMatch() && type == NavigationTypeLinkClicked) {
            QString videoId = match.captured(1);
            // Extract title from URL or use video ID as fallback
            QMetaObject::invokeMethod(m_mainWindow, "openVideoById",
                Qt::QueuedConnection,
                Q_ARG(QString, videoId),
                Q_ARG(QString, QString("Video: %1").arg(videoId)));
            return false; // Don't navigate, we're opening in a new tab
        }
        
        return true; // Allow all other navigation
    }

private:
    MainWindow *m_mainWindow;
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QWidget* w = m_tabs->widget(index);
        if (w != m_searchTab && w != m_youtubeHome) { 
            m_tabs->removeTab(index);
            w->deleteLater(); 
        }
    });

    setCentralWidget(m_tabs);

    // === YouTube Homepage Tab ===
    m_youtubeHome = new QWebEngineView(this);
    m_youtubeHome->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    m_youtubeHome->settings()->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, true);
    
    // Use custom page to intercept video clicks
    YouTubeHomePage *homePage = new YouTubeHomePage(this, m_youtubeHome);
    m_youtubeHome->setPage(homePage);
    
    // Set user agent for proper YouTube rendering
    m_youtubeHome->page()->profile()->setHttpUserAgent(
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"
    );
    
    // Load YouTube homepage
    m_youtubeHome->setUrl(QUrl("https://www.youtube.com"));
    m_tabs->addTab(m_youtubeHome, "YouTube");

    // === Search Tab ===
    m_searchTab = new QWidget();
    m_searchTab->setObjectName("centralWidget");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_searchTab);
    mainLayout->setContentsMargins(32, 28, 32, 28);
    mainLayout->setSpacing(20);

    QLabel *headerLabel = new QLabel("Search YouTube Videos", this);
    headerLabel->setStyleSheet("font-size: 22px; font-weight: 700; color: #cdd6f4; margin-bottom: 8px;");
    mainLayout->addWidget(headerLabel);
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLayout->setSpacing(12);
    
    m_searchInput = new QLineEdit(this);
    m_searchInput->setObjectName("searchInput");
    m_searchInput->setPlaceholderText("Search for videos, channels, or topics...");
    m_searchInput->setMinimumHeight(52);
    m_searchInput->setClearButtonEnabled(true);

    m_searchBtn = new QPushButton("Search", this);
    m_searchBtn->setObjectName("searchBtn");
    m_searchBtn->setMinimumHeight(52);
    m_searchBtn->setMinimumWidth(120);
    m_searchBtn->setCursor(Qt::PointingHandCursor);
    m_searchBtn->setToolTip("Press Enter or click to search");
    
    m_videoList = new QListWidget(this);
    m_videoList->setIconSize(QSize(200, 112));
    m_videoList->setSpacing(12);
    m_videoList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_videoList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_videoList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_videoList->setFrameShape(QFrame::NoFrame);
    m_videoList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_videoList->setAlternatingRowColors(false);

    searchLayout->addWidget(m_searchInput, 1); 
    searchLayout->addWidget(m_searchBtn);
    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(m_videoList, 1);

    m_tabs->addTab(m_searchTab, "Search");

    m_service = new YouTubeService(this);

    connect(m_searchInput, &QLineEdit::returnPressed, this, &MainWindow::performSearch);
    connect(m_searchBtn, &QPushButton::clicked, this, &MainWindow::performSearch);
    connect(m_videoList, &QListWidget::itemClicked, this, &MainWindow::openInNewTab);
    connect(m_videoList, &QListWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);
    connect(m_service, &YouTubeService::searchResultsReady, this, &MainWindow::handleSearchResults);
    connect(m_service, &YouTubeService::errorOccurred, this, &MainWindow::showError);

    setWindowTitle("YouCaption - YouTube Caption Viewer");
    resize(1100, 850);
    setMinimumSize(800, 600);
}

void MainWindow::showContextMenu(const QPoint &pos) {
    QListWidgetItem *item = m_videoList->itemAt(pos);
    if (!item) return;

    QMenu menu(this);
    QAction *openAction = menu.addAction("Open in New Tab");
    connect(openAction, &QAction::triggered, [this, item]() { openInNewTab(item); });
    menu.exec(m_videoList->mapToGlobal(pos));
}

void MainWindow::openInNewTab(QListWidgetItem *item) {
    QString videoId = item->data(Qt::UserRole).toString();
    QString title = item->data(Qt::UserRole + 1).toString();
    openVideoById(videoId, title);
}

void MainWindow::openVideoById(const QString &videoId, const QString &title) {
    auto *tw = new TranscriptWindow(videoId, title, this);
    int index = m_tabs->addTab(tw, title.left(15) + "...");
    m_tabs->setCurrentIndex(index);
}

void MainWindow::performSearch() {
    QString query = m_searchInput->text().trimmed();
    if (query.isEmpty()) return;
    
    m_searchBtn->setText("Searching...");
    m_searchBtn->setEnabled(false);
    m_videoList->clear();
    m_service->searchVideos(query);
}

void MainWindow::handleSearchResults(const QList<VideoResult> &results) {
    m_searchBtn->setText("Search");
    m_searchBtn->setEnabled(true);
    
    if (results.isEmpty()) {
        QListWidgetItem *emptyItem = new QListWidgetItem(m_videoList);
        emptyItem->setText("No videos found. Try a different search term.");
        emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsSelectable);
        emptyItem->setTextAlignment(Qt::AlignCenter);
        emptyItem->setForeground(QColor("#a6adc8"));
        return;
    }
    
    for (const auto &vid : results) {
        QListWidgetItem *item = new QListWidgetItem(m_videoList);
        QString displayText = QString("%1\n%2").arg(vid.title, vid.channel);
        item->setText(displayText);
        item->setForeground(QColor("#cdd6f4"));
        item->setData(Qt::UserRole, vid.id); 
        item->setData(Qt::UserRole + 1, vid.title); 
        item->setData(Qt::UserRole + 2, vid.channel);
        item->setToolTip(QString("Click to watch: %1").arg(vid.title));
        fetchThumbnail(vid.thumbnailUrl, item);
    }
}

void MainWindow::fetchThumbnail(const QString &url, QListWidgetItem *item) {
    QNetworkAccessManager *net = new QNetworkAccessManager(this);
    QNetworkReply *reply = net->get(QNetworkRequest(QUrl(url)));
    connect(reply, &QNetworkReply::finished, [reply, item, net]() {
        if (reply->error() == QNetworkReply::NoError) {
            QPixmap pixmap;
            pixmap.loadFromData(reply->readAll());
            item->setIcon(QIcon(pixmap));
        }
        reply->deleteLater();
        net->deleteLater();
    });
}

void MainWindow::showError(const QString &msg) {
    m_searchBtn->setText("Search");
    m_searchBtn->setEnabled(true);
    QMessageBox::critical(this, "Error", msg);
}
