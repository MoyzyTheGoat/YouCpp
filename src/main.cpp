#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include "ui/MainWindow.h"

void loadEnv() {
    QString envPath = ".env";
    if (!QFile::exists(envPath)) {
        envPath = "../.env"; 
    }

    QFile file(envPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#')) continue;

            QStringList parts = line.split('=');
            if (parts.size() >= 2) {
                QString key = parts[0].trimmed();
                QString value = parts[1].trimmed().remove('"').remove('\'');
                qputenv(key.toUtf8(), value.toUtf8());
            }
        }
        file.close();
    }
}

#include <QApplication>
#include "ui/MainWindow.h"
#include <QFontDatabase>
#include <cstdio>

int main(int argc, char *argv[]) {
    printf("[YouCpp] Starting application...\n");
    fflush(stdout);
    
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", 
            "--no-sandbox "
            "--autoplay-policy=no-user-gesture-required "
            "--allow-running-insecure-content "
            "--user-agent=\"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36\"");

    QApplication app(argc, argv);

    app.setStyle("Fusion");

    QFont font("Inter");
    if (QFontDatabase::addApplicationFont(":/fonts/Inter-Regular.ttf") == -1) {
        font.setFamily("Segoe UI"); 
    }
    font.setPointSize(10);
    app.setFont(font);

    QString qss = R"(
        /* === Base Styles === */
        QMainWindow, QWidget#centralWidget {
            background: qlineargradient(x1:0, y1:0, x2:0.5, y2:1,
                        stop:0 #0d0d14, stop:0.5 #11111b, stop:1 #0a0a10);
        }
        
        QLabel {
            color: #cdd6f4;
            font-family: 'Inter', 'Segoe UI', sans-serif;
        }

        /* === Modern Search Bar === */
        QLineEdit {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #1e1e2e, stop:1 #181825);
            border: 2px solid #313244;
            border-radius: 14px;
            padding: 14px 20px;
            color: #cdd6f4;
            font-size: 15px;
            selection-background-color: #89b4fa;
            selection-color: #11111b;
        }
        QLineEdit:hover {
            border: 2px solid #45475a;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #232334, stop:1 #1e1e2e);
        }
        QLineEdit:focus {
            border: 2px solid #89b4fa;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #252536, stop:1 #1e1e2e);
        }

        /* === Gradient Search Button === */
        QPushButton#searchBtn {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                        stop:0 #89b4fa, stop:0.5 #a6c8ff, stop:1 #b4befe);
            color: #11111b;
            border: none;
            border-radius: 14px;
            padding: 14px 28px;
            font-weight: 700;
            font-size: 14px;
            letter-spacing: 0.5px;
        }
        QPushButton#searchBtn:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                        stop:0 #b4befe, stop:0.5 #c6d0ff, stop:1 #cba6f7);
        }
        QPushButton#searchBtn:pressed {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                        stop:0 #74c7ec, stop:1 #89b4fa);
        }
        QPushButton#searchBtn:disabled {
            background: #45475a;
            color: #6c7086;
        }

        /* === Glassmorphism Video Cards === */
        QListWidget {
            background-color: transparent;
            border: none;
            outline: none;
            padding: 8px;
        }
        QListWidget::item {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                        stop:0 rgba(30, 30, 46, 0.95), stop:1 rgba(24, 24, 37, 0.9));
            border: 1px solid rgba(49, 50, 68, 0.6);
            border-radius: 16px;
            margin: 8px 4px;
            padding: 16px;
        }
        QListWidget::item:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                        stop:0 rgba(49, 50, 68, 0.98), stop:1 rgba(36, 36, 54, 0.95));
            border: 1px solid rgba(137, 180, 250, 0.5);
        }
        QListWidget::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                        stop:0 rgba(49, 50, 68, 1), stop:1 rgba(36, 36, 54, 0.98));
            border: 2px solid #89b4fa;
        }

        /* === Smooth Scrollbar === */
        QScrollBar:vertical {
            border: none;
            background: transparent;
            width: 10px;
            margin: 4px;
        }
        QScrollBar::handle:vertical {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                        stop:0 #45475a, stop:1 #585b70);
            border-radius: 5px;
            min-height: 40px;
        }
        QScrollBar::handle:vertical:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                        stop:0 #585b70, stop:1 #6c7086);
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }

        /* === Modern Tab Bar === */
        QTabWidget::pane {
            border: none;
            background: transparent;
            top: -1px;
        }
        QTabBar {
            background: transparent;
        }
        QTabBar::tab {
            background: transparent;
            color: #6c7086;
            padding: 14px 24px;
            margin-right: 4px;
            border: none;
            border-bottom: 3px solid transparent;
            font-weight: 600;
            font-size: 13px;
        }
        QTabBar::tab:hover {
            color: #a6adc8;
            background: rgba(49, 50, 68, 0.3);
            border-radius: 8px 8px 0 0;
        }
        QTabBar::tab:selected {
            color: #89b4fa;
            border-bottom: 3px solid #89b4fa;
            background: rgba(137, 180, 250, 0.08);
            border-radius: 8px 8px 0 0;
        }
        QTabBar::close-button {
            subcontrol-position: right;
            padding: 4px;
            margin: 4px;
        }
        QTabBar::close-button:hover {
            background: rgba(243, 139, 168, 0.3);
            border-radius: 4px;
        }

        /* === Splitter === */
        QSplitter::handle {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                        stop:0 #313244, stop:0.5 #45475a, stop:1 #313244);
            width: 3px;
        }
        QSplitter::handle:hover {
            background: #89b4fa;
        }

        /* === Context Menu === */
        QMenu {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #1e1e2e, stop:1 #181825);
            border: 1px solid #313244;
            border-radius: 12px;
            padding: 8px;
        }
        QMenu::item {
            padding: 10px 24px;
            border-radius: 8px;
            color: #cdd6f4;
            font-size: 13px;
        }
        QMenu::item:selected {
            background: rgba(137, 180, 250, 0.15);
            color: #89b4fa;
        }
        QMenu::separator {
            height: 1px;
            background: #313244;
            margin: 6px 12px;
        }

        /* === Message Box === */
        QMessageBox {
            background: #1e1e2e;
        }
        QMessageBox QLabel {
            color: #cdd6f4;
            font-size: 14px;
        }
        QMessageBox QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #313244, stop:1 #2a2a3c);
            border: 1px solid #45475a;
            border-radius: 10px;
            padding: 10px 24px;
            color: #cdd6f4;
            font-weight: 600;
            min-width: 80px;
        }
        QMessageBox QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #45475a, stop:1 #3a3a4e);
            border: 1px solid #585b70;
        }
        QMessageBox QPushButton:default {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                        stop:0 #89b4fa, stop:1 #b4befe);
            border: none;
            color: #11111b;
        }

        /* === Tooltip === */
        QToolTip {
            background: #1e1e2e;
            border: 1px solid #45475a;
            border-radius: 8px;
            padding: 8px 12px;
            color: #cdd6f4;
            font-size: 12px;
        }
    )";
    app.setStyleSheet(qss);
    
    loadEnv(); 

    MainWindow window;
    window.show();

    return app.exec();
}
