# Custom Files

A modified version of GNOME Files (Nautilus) for GNOME desktops.

## Changes

- Embedded terminal. Press `F4` to show or hide it. It runs the host Bash shell and reads `.bashrc`.
- Folder colors, including matching colors in the left sidebar.
- **Copy Full Path** in the right-click menu.
- Double-click empty space to show or hide hidden files.

## Install

Works on Arch Linux and Debian-based distributions. Download and run the installer:

```bash
curl -fLO https://raw.githubusercontent.com/aradar46/nautilus-custom/main/install.sh
chmod +x install.sh
./install.sh
```

The installer adds Flathub, installs any required packages, downloads the latest release, and makes Custom Files the default folder handler. The system version of GNOME Files is not removed.

Launch it from the application menu or run:

```bash
flatpak run org.gnome.Nautilus.Custom
```

## Update

Run the installer again. It always downloads the latest release.

## Uninstall

```bash
flatpak uninstall --user org.gnome.Nautilus.Custom
```

## Manual installation

Download `nautilus-custom.flatpak` from the [latest release](https://github.com/aradar46/nautilus-custom/releases/latest), then run:

```bash
flatpak install --user nautilus-custom.flatpak
```

This project is based on [GNOME Files](https://gitlab.gnome.org/GNOME/nautilus) and is licensed under GPL-3.0-or-later.
