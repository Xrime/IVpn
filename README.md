#  IVpn

This is a secure VPN application, developed in C++ for Windows, routes your system's traffic through the Tor network to ensure online anonymity.

I built this project to learn more about networking, Windows Services, IPC (Inter-Process Communication), and modern C++.

## Features
* **Full System Routing:** Uses the `Wintun` driver to capture and route your computer's entire traffic, not just your browser!
* **Tor Integration:** Automatically sets up Tor circuits to encrypt and hide your traffic.
* **Country Selection:** Pick exactly which country you want your traffic to exit from (Not working perfectly for some reason because of tor).
* **Terminal UI (TUI):** A sleek, terminal-based user interface using the `FTXUI` library.
* **Background Service Daemon:** Runs silently in the background as a Windows Service (`LocalSystem`) so you don't need to keep a console window open.
* **Secure Killswitch:** Blocks all internet traffic if the Tor connection drops, keeping your real IP safe.

## Tech Stack
* **Language:** C++20
* **Build System:** CMake
* **Libraries Used:** 
  * `wintun` - For creating a virtual network adapter
  * `tor` - For secure onion routing
  * `FTXUI` - For the terminal user interface
  * `spdlog` - For fast, thread-safe logging
  * `libmaxminddb` - For GeoIP lookups
  * `fmt` - For string formatting

## How It Works (Architecture)

The app is split into two main parts that communicate over a Named Pipe (\\.\pipe\ivpn_pipe). I transitioned to this architecture because I do not want the user to face a User Account Control (UAC) prompt for Administrator privileges every time they open the app.

1. **`ivpn_daemon`:** This runs as a background Windows Service. It launches `tor.exe`, creates the `Wintun` network interface, sets up the Windows routing tables, intercepts DNS requests, and proxies your TCP/UDP packets through Tor via a SOCKS5 proxy.
2. **`ivpn_client` (The Face):** This is the UI you see on your screen. It connects to the daemon, asks it for the current connection status, and tells it when to switch countries or disconnect.

## How to Install & Run
1. Download the `IVpn_Setup.exe` installer.
2. Run the installer (it will ask for Administrator privileges so it can install the background service).
3. Wait about 30-60 seconds for the Tor network to bootstrap and download its relay descriptors.
4. Launch the **IVpn Secure Routing** shortcut on your desktop!
5. Select a country, click "Connect", and enjoy your secure connection.

## Known Issues & Future Plans
* The Tor server is not producing the actual country that I want, if a particular country was pick it may not be it.
* Add support for UDP over Tor (currently Tor only supports TCP and DNS).
* Add a graphical UI (GUI) instead of just a terminal UI.

---
