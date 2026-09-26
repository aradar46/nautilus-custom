# Build and Install Guide (Arch & Debian)

This custom Nautilus (GNOME Files) build includes:
- **Embedded Bottom Terminal** (Toggle with `F4` or right-click -> *Open Terminal Here*)
- **Native Folder Color Picker** (Right-click folder -> *Folder Color*, syncs with left sidebar)
- **Copy Full Path** (Right-click file/folder/background -> *Copy Full Path*)

---

## 1. Arch Linux / EndeavourOS

### Step 1: Install build dependencies
```bash
sudo pacman -S --needed \
  meson ninja gcc \
  gtk4 libadwaita vte4 \
  gexiv2 libportal-gtk4 libcloudproviders \
  gnome-desktop-4 gnome-autoar localsearch
```

### Step 2: Configure and compile
```bash
meson setup build -Dprefix=/usr
ninja -C build
```

### Step 3: Install as system Files app
```bash
# Clean up any leftover build files from /usr/local if present
sudo rm -rf /usr/local/bin/nautilus /usr/local/share/applications/org.gnome.Nautilus.desktop /usr/local/share/dbus-1/services/org.gnome.Nautilus* /usr/local/share/dbus-1/services/org.freedesktop.FileManager1.service

sudo ninja -C build install
sudo glib-compile-schemas /usr/share/glib-2.0/schemas
sudo update-desktop-database
killall -9 nautilus 2>/dev/null
```

---

## 2. Debian 12 / 13 (Trixie / Sid)

### Step 1: Install build dependencies
```bash
sudo apt update && sudo apt install -y \
  meson ninja-build gcc \
  libgtk-4-dev libadwaita-1-dev libvte-2.91-gtk4-dev \
  libgexiv2-dev libportal-gtk4-dev libcloudproviders-dev \
  libgnome-desktop-4-dev libgnome-autoar-0-dev liblocalsearch-3-dev
```

### Step 2: Configure and compile
```bash
meson setup build -Dprefix=/usr
ninja -C build
```

### Step 3: Install as system Files app
```bash
# Clean up any leftover build files from /usr/local if present
sudo rm -rf /usr/local/bin/nautilus /usr/local/share/applications/org.gnome.Nautilus.desktop /usr/local/share/dbus-1/services/org.gnome.Nautilus* /usr/local/share/dbus-1/services/org.freedesktop.FileManager1.service

sudo ninja -C build install
sudo glib-compile-schemas /usr/share/glib-2.0/schemas
sudo update-desktop-database
killall -9 nautilus 2>/dev/null
```

---

## 3. Test Without Installing

You can test the binary directly without modifying your system:
```bash
killall -9 nautilus 2>/dev/null
./build/src/nautilus -w
```

---

## 4. How to Restore Original System Nautilus

If you ever want to revert back to stock Nautilus:

- **Arch Linux**:
  ```bash
  sudo pacman -S --overwrite "*" nautilus
  ```

- **Debian**:
  ```bash
  sudo apt install --reinstall nautilus
  ```

