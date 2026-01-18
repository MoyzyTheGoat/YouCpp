#include "TranscriptWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QWebEngineHttpRequest>
#include <QUrl>

TranscriptWindow::TranscriptWindow(const QString &videoId, const QString &title, QWidget *parent) 
    : QWidget(parent) 
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_webView = new QWebEngineView(this);
    m_webView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    m_webView->settings()->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
    
    m_webView->page()->profile()->setHttpUserAgent("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    m_webView->page()->profile()->cookieStore()->deleteAllCookies();
    m_webView->page()->profile()->clearHttpCache();

    loadVideo(videoId);
    layout->addWidget(m_webView, 1);

    QWidget *controlBar = new QWidget(this);
    controlBar->setObjectName("controlBar");
    controlBar->setFixedHeight(70);
    controlBar->setStyleSheet(R"(
        QWidget#controlBar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #1e1e2e, stop:1 #181825);
            border-top: 1px solid #313244;
        }
    )");
    
    QHBoxLayout *controlLayout = new QHBoxLayout(controlBar);
    controlLayout->setContentsMargins(24, 8, 24, 8);
    controlLayout->setSpacing(12);

    // Speed Icon
    QLabel *speedIcon = new QLabel(">>", this);
    speedIcon->setStyleSheet("font-size: 16px; font-weight: 700; color: #89b4fa;");
    speedIcon->setToolTip("Playback Speed");
    
    // Speed Label
    QLabel *speedTextLabel = new QLabel("Speed:", this);
    speedTextLabel->setStyleSheet("font-size: 13px; font-weight: 500; color: #a6adc8;");


    QHBoxLayout *presetLayout = new QHBoxLayout();
    presetLayout->setSpacing(6);
    
    QList<double> presets = {0.5, 1.0, 1.5, 2.0, 4.0};
    for (double speed : presets) {
        QPushButton *btn = createSpeedButton(QString("%1x").arg(speed, 0, 'f', 1), speed);
        presetLayout->addWidget(btn);
    }

    m_speedSlider = new QSlider(Qt::Horizontal, this);
    m_speedSlider->setRange(5, 50); 
    m_speedSlider->setValue(15); 
    m_speedSlider->setMinimumWidth(150);
    m_speedSlider->setMaximumWidth(300);
    m_speedSlider->setToolTip("Fine-tune playback speed");
    m_speedSlider->setStyleSheet(R"(
        QSlider::groove:horizontal {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                        stop:0 #313244, stop:1 #45475a);
            height: 6px;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                        stop:0 #89b4fa, stop:1 #b4befe);
            width: 16px;
            height: 16px;
            margin: -5px 0;
            border-radius: 8px;
            border: 2px solid #11111b;
        }
        QSlider::handle:horizontal:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                        stop:0 #b4befe, stop:1 #cba6f7);
        }
        QSlider::sub-page:horizontal {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                        stop:0 #89b4fa, stop:1 #b4befe);
            border-radius: 3px;
        }
    )");
    
    m_speedLabel = new QLabel("1.0x", this);
    m_speedLabel->setFixedWidth(55);
    m_speedLabel->setAlignment(Qt::AlignCenter);
    m_speedLabel->setStyleSheet(R"(
        font-size: 13px;
        font-weight: 700;
        color: #cdd6f4;
        background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #313244, stop:1 #2a2a3c);
        border: 1px solid #45475a;
        border-radius: 6px;
        padding: 4px 8px;
    )");
    
    connect(m_speedSlider, &QSlider::valueChanged, this, &TranscriptWindow::updatePlaybackSpeed);
    
    controlLayout->addWidget(speedIcon);
    controlLayout->addWidget(speedTextLabel);
    controlLayout->addLayout(presetLayout);
    controlLayout->addWidget(m_speedSlider, 1);
    controlLayout->addWidget(m_speedLabel);
    controlLayout->addStretch();
    
    layout->addWidget(controlBar);
}

TranscriptWindow::~TranscriptWindow() {
    if (m_webView) {
        m_webView->stop();
        m_webView->setHtml(""); 
        m_webView->deleteLater();
    }
}

void TranscriptWindow::loadVideo(const QString &videoId) {
    QString html = QString(
        "<!DOCTYPE html>"
        "<html style='height:100%;width:100%;margin:0;padding:0;'>"
        "<head>"
            "<meta name='referrer' content='origin' />"
            "<style>"
                "html, body { height:100%; width:100%; margin:0; padding:0; overflow:hidden; background:#000; }"
                "iframe { position:absolute; top:0; left:0; width:100%; height:100%; border:none; }"
            "</style>"
        "</head>"
        "<body>"
            "<iframe id='player' "
            "src='https://www.youtube-nocookie.com/embed/%1?autoplay=1&enablejsapi=1&origin=https://www.youtube-nocookie.com&rel=0' "
            "allow='autoplay; encrypted-media' allowfullscreen "
            "referrerpolicy='origin'>"
            "</iframe>"
        "</body>"
        "</html>"
    ).arg(videoId);

    m_webView->setHtml(html, QUrl("https://www.youtube-nocookie.com/"));
}

void TranscriptWindow::updatePlaybackSpeed(int value) {
    double speed = value / 10.0;
    applySpeed(speed);
}

void TranscriptWindow::setSpeed(double speed) {
    m_speedSlider->blockSignals(true);
    m_speedSlider->setValue(static_cast<int>(speed * 10));
    m_speedSlider->blockSignals(false);
    applySpeed(speed);
}

void TranscriptWindow::applySpeed(double speed) {
    m_speedLabel->setText(QString("%1x").arg(speed, 0, 'f', 1));

    QString js = QString(
        "var v = document.querySelector('iframe').contentWindow.document.querySelector('video');"
        "if(v) { v.playbackRate = %1; }"
    ).arg(speed);
    
    m_webView->page()->runJavaScript(js);
}

QPushButton* TranscriptWindow::createSpeedButton(const QString &label, double speed) {
    QPushButton *btn = new QPushButton(label, this);
    btn->setFixedSize(48, 32);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setToolTip(QString("Set speed to %1").arg(label));
    btn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #313244, stop:1 #2a2a3c);
            border: 1px solid #45475a;
            border-radius: 6px;
            color: #cdd6f4;
            font-size: 11px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #45475a, stop:1 #3a3a4e);
            border: 1px solid #89b4fa;
            color: #89b4fa;
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                        stop:0 #89b4fa, stop:1 #b4befe);
            color: #11111b;
        }
    )");
    
    connect(btn, &QPushButton::clicked, this, [this, speed]() {
        setSpeed(speed);
    });
    
    return btn;
}
