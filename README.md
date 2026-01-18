# YouCaption

A sleek, modern desktop application for **consuming YouTube content faster** — built with C++ and Qt6.

![Qt](https://img.shields.io/badge/Qt-6.x-41CD52?logo=qt&logoColor=white)
![C++](https://img.shields.io/badge/C++-20-00599C?logo=cplusplus&logoColor=white)
![License](https://img.shields.io/badge/license-MIT-blue)

---

## 🚀 Why YouCaption?

YouTube limits playback speed to **2x** unless you pay for Premium. But let's be honest — most videos are packed with filler, long intros, and unnecessary padding. Why waste time?

YouCaption lets you:
- ⚡ **Watch at up to 5x speed** — bypass the 2x limit entirely
- 🚫 **See virtually no ads** — embedded player avoids most YouTube ads
- ⏱️ **Save hours** — cut through the fluff and get to the content that matters

---

## ✨ Features

- **Unrestricted Speed Control** — Adjust playback from 0.5x to 5.0x with presets and a fine-tuned slider
- **Ad-Free Experience** — Privacy-enhanced embed mode minimizes ads
- **YouTube Search** — Search for videos directly using the YouTube Data API
- **Tabbed Interface** — Open multiple videos in separate tabs
- **Modern Dark UI** — Beautiful Catppuccin-inspired theme with glassmorphism effects

---


## 🛠️ Prerequisites

Before building, ensure you have the following installed:

| Dependency | Version | Purpose |
|------------|---------|---------|
| **CMake** | ≥ 3.16 | Build system |
| **Qt6** | 6.x | UI framework |

### Qt6 Modules Required:
- `Qt6::Core`
- `Qt6::Gui`
- `Qt6::Widgets`
- `Qt6::Network`
- `Qt6::WebEngineWidgets`

---

## 🔑 Configuration

Create a `.env` file in the project root with your YouTube Data API key:

```env
YOUTUBE_API_KEY=your_api_key_here
```

> **Note:** Get your API key from the [Google Cloud Console](https://console.cloud.google.com/apis/credentials).

---

## 🚀 Build & Run

```bash
# Clone the repository
git clone https://github.com/yourusername/YouCpp.git
cd YouCpp

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)

# Run the application
./YouCaptionCpp
```

---

## 📁 Project Structure

```
YouCpp/
├── CMakeLists.txt          # Build configuration
├── .env                    # API key (gitignored)
├── src/
│   ├── main.cpp            # Application entry point & styling
│   ├── backend/
│   │   ├── YouTubeService.cpp   # YouTube API & yt-dlp integration
│   │   └── YouTubeService.h
│   └── ui/
│       ├── MainWindow.cpp       # Main search interface
│       ├── MainWindow.h
│       ├── TranscriptWindow.cpp # Video player & speed controls
│       └── TranscriptWindow.h
└── build/                  # Compiled output 
```

---

## 🎨 UI Design

YouCaption features a modern dark theme inspired by **Catppuccin Mocha**:

- **Background:** Deep dark gradients (`#0d0d14` → `#11111b`)
- **Accent:** Soft blue (`#89b4fa`)
- **Cards:** Glassmorphism with semi-transparent backgrounds
- **Typography:** Inter / Segoe UI with clean readability
- **Animations:** Smooth gradient hover transitions


---

## 📖 Usage

1. **Search** — Enter a query in the search bar and press Enter or click Search
2. **Open Video** — Click any video result to open it in a new tab
3. **Control Playback** — Use the speed presets (0.5x–4.0x) or the slider for fine control
4. **Close Tabs** — Click the X on any tab to close it (except the Search tab)

---

## 🔧 Dependencies Installation

### Ubuntu/Debian
```bash
# Qt6 and development tools
sudo apt install qt6-base-dev qt6-webengine-dev cmake build-essential
```

### Arch Linux
```bash
sudo pacman -S qt6-base qt6-webengine cmake
```

### macOS (Homebrew)
```bash
brew install qt@6 cmake
```

---

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request
