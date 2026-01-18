#pragma once
#include <QWidget>
#include <QWebEngineView>
#include <QSlider>
#include <QLabel>
#include <QPushButton>

class TranscriptWindow : public QWidget {
    Q_OBJECT

public:
    TranscriptWindow(const QString &videoId, const QString &title, QWidget *parent = nullptr);
    ~TranscriptWindow();

private slots:
    void updatePlaybackSpeed(int value);
    void setSpeed(double speed);

private:
    void loadVideo(const QString &videoId);
    void applySpeed(double speed);
    QPushButton* createSpeedButton(const QString &label, double speed);
    
    QWebEngineView *m_webView;
    QLabel *m_speedLabel;
    QSlider *m_speedSlider;
};
