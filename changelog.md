##

- 🐛 Don't show Midi error if a Midi controller is plugged in and user in another software

## 2.2.1

- 🐛 Fix crash when cancelling the installation of a version 

## 2.2.0

- 🐛 Fixed: We couldn't install any versions anymore
<img width="342" height="100" alt="image" src="https://github.com/user-attachments/assets/678eae8b-99d1-41ab-bfa5-8b20665399a9" />

## 2.1.0

- 🤏 Added quotes around the name of the version to make it clearer it is just a name (e.g. `1.2.0 "MacOS"`)
- 🐛 Fixed: A project file saved on Windows would fail to open on Linux
- 🐛 Fixed connecting to the internet on some Linux distributions (missing CA cert path)
- 🐛 Fixed the UI being a bit buggy on Linux Wayland and clicks being offset
- 🐛 Fixed the missing window decorations on Linux (by switching to X11 instead of Wayland)
- 🐛 Fixed the ability to drag panels outside of the main window on Linux (by switching to X11 instead of Wayland)

## 2.0.1

- 🐛 In Linux's .desktop file, add missing %f to indicate that you can pass a file path to the exe

## 2.0.0

- 💥 Fix bug with project info hash which could change. This creates an incompatibility with older versions of Coollab (< 1.2.0)
- 🐛 Fixed issue with connecting to the Internet on some Linux distributions, and on MacOS (timeout too long)

## 1.1.0

- ✨ Log errors to a log file

## 1.0.0

- 🎉 Initial release of the launcher! 🥳