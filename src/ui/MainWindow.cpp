#include "MainWindow.h"
#include "TranscriptWindow.h"
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <cstdio>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QWidget* w = m_tabs->widget(index);
        if (w != m_searchTab && w != m_homeTab) { 
            m_tabs->removeTab(index);
            w->deleteLater(); 
        }
    });

    setCentralWidget(m_tabs);

    m_auth = new GoogleAuth(this);
    m_service = new YouTubeService(this);

    QString clientId = qEnvironmentVariable("GOOGLE_CLIENT_ID");
    QString clientSecret = qEnvironmentVariable("GOOGLE_CLIENT_SECRET");
    m_auth->setCredentials(clientId, clientSecret);

    setupHomeTab();
    m_tabs->addTab(m_homeTab, "Home");

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

    connect(m_searchInput, &QLineEdit::returnPressed, this, &MainWindow::performSearch);
    connect(m_searchBtn, &QPushButton::clicked, this, &MainWindow::performSearch);
    connect(m_videoList, &QListWidget::itemClicked, this, &MainWindow::openVideoFromItem);
    connect(m_videoList, &QListWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);
    
    connect(m_service, &YouTubeService::searchResultsReady, this, &MainWindow::handleSearchResults);
    connect(m_service, &YouTubeService::subscriptionFeedReady, this, &MainWindow::handleSubscriptionFeed);
    connect(m_service, &YouTubeService::recommendationsReady, this, &MainWindow::handleRecommendations);
    connect(m_service, &YouTubeService::errorOccurred, this, &MainWindow::showError);
    
    connect(m_auth, &GoogleAuth::authenticated, this, &MainWindow::onAuthenticated);
    connect(m_auth, &GoogleAuth::authenticationFailed, this, &MainWindow::onAuthFailed);
    connect(m_auth, &GoogleAuth::loggedOut, this, &MainWindow::onLoggedOut);

    setWindowTitle("YouCpp - YouTube Video Player");
    resize(1100, 850);
    setMinimumSize(800, 600);
    

    if (m_auth->isAuthenticated()) {
        onAuthenticated();
    }
}

class VideoCard : public QWidget {
public:
    VideoCard(const QString &title, const QString &channel, QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 4, 4, 4); 
        layout->setSpacing(0);

        QWidget *card = new QWidget(this);
        card->setStyleSheet(R"(
            QWidget {
                background-color: #232433; /* Darker background for contrast */
                border-radius: 12px;
            }
        )");
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(0, 0, 0, 12);
        cardLayout->setSpacing(8);

        m_thumbnail = new QLabel(card);
        m_thumbnail->setFixedSize(300, 169); 
        m_thumbnail->setStyleSheet("background-color: #11111b; border-top-left-radius: 12px; border-top-right-radius: 12px; border-bottom-left-radius: 0; border-bottom-right-radius: 0;");
        m_thumbnail->setAlignment(Qt::AlignCenter);
        m_thumbnail->setScaledContents(true); 
        
        m_title = new QLabel(title, card);
        m_title->setWordWrap(true);
        m_title->setStyleSheet("font-weight: 700; font-size: 15px; color: #ffffff; padding: 0 12px; background: transparent;");
        m_title->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        m_title->setFixedHeight(45);
        
        m_channel = new QLabel(channel, card);
        m_channel->setStyleSheet("font-size: 13px; color: #bac2de; padding: 0 12px; background: transparent; font-weight: 500;");
        m_channel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        cardLayout->addWidget(m_thumbnail);
        cardLayout->addWidget(m_title);
        cardLayout->addWidget(m_channel);
        cardLayout->addStretch();
        
        layout->addWidget(card);
        
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }
    
    void setThumbnail(const QPixmap &pixmap) {
        if (!pixmap.isNull()) {
             m_thumbnail->setPixmap(pixmap.scaled(m_thumbnail->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        }
    }

private:
    QLabel *m_thumbnail;
    QLabel *m_title;
    QLabel *m_channel;
};

void MainWindow::setupHomeTab() {
    m_homeTab = new QWidget();
    m_homeTab->setObjectName("centralWidget");
    
    QVBoxLayout *layout = new QVBoxLayout(m_homeTab);
    layout->setContentsMargins(32, 28, 32, 28);
    layout->setSpacing(20);
    
    QHBoxLayout *headerLayout = new QHBoxLayout();
    
    QLabel *titleLabel = new QLabel("Your Feed", this);
    titleLabel->setStyleSheet("font-size: 22px; font-weight: 700; color: #cdd6f4;");
    headerLayout->addWidget(titleLabel);
    
    headerLayout->addStretch();
    
    m_authStatusLabel = new QLabel("Not signed in", this);
    m_authStatusLabel->setStyleSheet("font-size: 13px; color: #a6adc8;");
    headerLayout->addWidget(m_authStatusLabel);
    
    m_signInBtn = new QPushButton("Sign In with Google", this);
    m_signInBtn->setObjectName("searchBtn");
    m_signInBtn->setMinimumHeight(40);
    m_signInBtn->setCursor(Qt::PointingHandCursor);
    connect(m_signInBtn, &QPushButton::clicked, this, &MainWindow::onSignInClicked);
    headerLayout->addWidget(m_signInBtn);
    
    m_signOutBtn = new QPushButton("Sign Out", this);
    m_signOutBtn->setMinimumHeight(40);
    m_signOutBtn->setCursor(Qt::PointingHandCursor);
    m_signOutBtn->setStyleSheet(R"(
        QPushButton {
            background: #45475a;
            color: #cdd6f4;
            border: none;
            border-radius: 10px;
            padding: 8px 16px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #585b70;
        }
    )");
    m_signOutBtn->hide();
    connect(m_signOutBtn, &QPushButton::clicked, m_auth, &GoogleAuth::logout);
    headerLayout->addWidget(m_signOutBtn);
    
    layout->addLayout(headerLayout);
    
    m_homeStack = new QStackedWidget(this);
    
    // Sign-in page
    m_signInPage = new QWidget();
    QVBoxLayout *signInLayout = new QVBoxLayout(m_signInPage);
    signInLayout->addStretch();
    
    QLabel *signInPrompt = new QLabel("Sign in to see your personalized feed", this);
    signInPrompt->setStyleSheet("font-size: 18px; color: #a6adc8;");
    signInPrompt->setAlignment(Qt::AlignCenter);
    signInLayout->addWidget(signInPrompt);
    
    QLabel *signInNote = new QLabel("Your subscriptions and recommendations will appear here", this);
    signInNote->setStyleSheet("font-size: 14px; color: #6c7086;");
    signInNote->setAlignment(Qt::AlignCenter);
    signInLayout->addWidget(signInNote);
    
    signInLayout->addStretch();
    m_homeStack->addWidget(m_signInPage);
    
    m_feedPage = new QWidget();
    QVBoxLayout *feedLayout = new QVBoxLayout(m_feedPage);
    feedLayout->setContentsMargins(0, 0, 0, 0);
    
    m_feedList = new QListWidget(this);
    m_feedList->setViewMode(QListWidget::IconMode);
    m_feedList->setResizeMode(QListWidget::Adjust);
    m_feedList->setMovement(QListView::Static);
    m_feedList->setSpacing(16); 
    m_feedList->setUniformItemSizes(true);
    m_feedList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_feedList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); 
    m_feedList->setFrameShape(QFrame::NoFrame);
    m_feedList->setSelectionMode(QAbstractItemView::NoSelection); 
    m_feedList->setStyleSheet("QListWidget { background: transparent; padding: 10px; } QListWidget::item { background: transparent; } QListWidget::item:hover { background: transparent; }");
    m_feedList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_feedList, &QListWidget::itemClicked, this, &MainWindow::openVideoFromItem);
    connect(m_feedList, &QListWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);
    
    feedLayout->addWidget(m_feedList);
    m_homeStack->addWidget(m_feedPage);
    
    layout->addWidget(m_homeStack, 1);
    
    m_homeStack->setCurrentWidget(m_signInPage);
}

void MainWindow::updateAuthUI() {
    if (m_auth->isAuthenticated()) {
        m_authStatusLabel->setText("Signed in");
        m_authStatusLabel->setStyleSheet("font-size: 13px; color: #a6e3a1;");
        m_signInBtn->hide();
        m_signOutBtn->show();
        m_homeStack->setCurrentWidget(m_feedPage);
    } else {
        m_authStatusLabel->setText("Not signed in");
        m_authStatusLabel->setStyleSheet("font-size: 13px; color: #a6adc8;");
        m_signInBtn->show();
        m_signOutBtn->hide();
        m_homeStack->setCurrentWidget(m_signInPage);
        m_feedList->clear();
    }
}

void MainWindow::onSignInClicked() {
    printf("[YouCpp] Sign-in button clicked\n");
    fflush(stdout);
    m_signInBtn->setEnabled(false);
    m_signInBtn->setText("Signing in...");
    m_auth->startLogin();
}

void MainWindow::onAuthenticated() {
    printf("[YouCpp] Authentication successful!\n");
    fflush(stdout);
    m_signInBtn->setEnabled(true);
    m_signInBtn->setText("Sign In with Google");
    
    m_service->setAccessToken(m_auth->accessToken());
    updateAuthUI();
    
    // Fetch personalized content
    printf("[YouCpp] Fetching subscription feed...\n");
    fflush(stdout);
    m_feedList->clear();
    QListWidgetItem *loadingItem = new QListWidgetItem("Loading your feed...", m_feedList);
    loadingItem->setFlags(loadingItem->flags() & ~Qt::ItemIsSelectable);
    loadingItem->setTextAlignment(Qt::AlignCenter);
    loadingItem->setForeground(QColor("#a6adc8"));
    
    m_service->fetchSubscriptionsFeed();
}

void MainWindow::onAuthFailed(const QString &error) {
    printf("[YouCpp] Authentication FAILED: %s\n", error.toUtf8().constData());
    fflush(stdout);
    m_signInBtn->setEnabled(true);
    m_signInBtn->setText("Sign In with Google");
    QMessageBox::warning(this, "Authentication Failed", error);
}

void MainWindow::onLoggedOut() {
    updateAuthUI();
}

void MainWindow::handleSubscriptionFeed(const QList<VideoResult> &results) {
    populateVideoList(m_feedList, results);
}

void MainWindow::handleRecommendations(const QList<VideoResult> &results) {
    populateVideoList(m_feedList, results);
}

void MainWindow::populateVideoList(QListWidget *list, const QList<VideoResult> &results) {
    list->clear();
    
    if (results.isEmpty()) {
        QListWidgetItem *emptyItem = new QListWidgetItem("No videos found", list);
        emptyItem->setSizeHint(QSize(200, 50));
        emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsSelectable);
        emptyItem->setTextAlignment(Qt::AlignCenter);
        emptyItem->setForeground(QColor("#a6adc8"));
        return;
    }
    
    for (const auto &vid : results) {
        QListWidgetItem *item = new QListWidgetItem(list);
        
        item->setSizeHint(QSize(310, 270)); 
        
        VideoCard *cardWidget = new VideoCard(vid.title, vid.channel, list);
        list->setItemWidget(item, cardWidget);
        
        item->setData(Qt::UserRole, vid.id);
        item->setData(Qt::UserRole + 1, vid.title);
        item->setData(Qt::UserRole + 2, vid.channel);
        item->setData(Qt::UserRole + 3, vid.channelId);
        item->setToolTip(QString("Click to watch: %1").arg(vid.title));
        
        QNetworkAccessManager *net = new QNetworkAccessManager(this);
        QNetworkReply *reply = net->get(QNetworkRequest(QUrl(vid.thumbnailUrl)));
        connect(reply, &QNetworkReply::finished, [reply, cardWidget, net]() {
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap pixmap;
                pixmap.loadFromData(reply->readAll());
                cardWidget->setThumbnail(pixmap);
            }
            reply->deleteLater();
            net->deleteLater();
        });
    }
}

void MainWindow::showContextMenu(const QPoint &pos) {
    QListWidget *list = qobject_cast<QListWidget*>(sender());
    if (!list) return;

    QListWidgetItem *item = list->itemAt(pos);
    if (!item) return;

    QMenu menu(this);
    QAction *openAction = menu.addAction("Open in New Tab");
    connect(openAction, &QAction::triggered, [this, item]() { openVideoFromItem(item); });
    
    QString channelId = item->data(Qt::UserRole + 3).toString();
    QString channelName = item->data(Qt::UserRole + 2).toString();
    
    if (!channelId.isEmpty()) {
        menu.addSeparator();
        QAction *muteAction = menu.addAction("Mute Channel '" + channelName + "'");
        connect(muteAction, &QAction::triggered, [this, channelId, channelName]() {
            m_service->muteChannel(channelId, channelName);
            
            QMessageBox::information(this, "Channel Muted", 
                QString("Channel '%1' has been muted.\n\nVideos from this channel will no longer appear in your feed.").arg(channelName));
                
            if (m_auth->isAuthenticated()) {
                m_feedList->clear();
                QListWidgetItem *loadingItem = new QListWidgetItem("Refreshing feed...", m_feedList);
                loadingItem->setTextAlignment(Qt::AlignCenter);
                loadingItem->setForeground(QColor("#a6adc8"));
                m_service->fetchSubscriptionsFeed(); 
            }
        });
    }
    
    menu.exec(list->mapToGlobal(pos));
}

void MainWindow::openVideoFromItem(QListWidgetItem *item) {
    QString videoId = item->data(Qt::UserRole).toString();
    QString title = item->data(Qt::UserRole + 1).toString();
    if (!videoId.isEmpty()) {
        openVideoById(videoId, title);
    }
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
    populateVideoList(m_videoList, results);
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
