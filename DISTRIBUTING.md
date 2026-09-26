# Distributing Custom Files

This project publishes an installable Flatpak for Arch Linux and Debian-based
GNOME systems. The target computer does not need build tools or Nautilus
development packages.

## First GitHub release

1. Create a GitHub repository and push this project to it.
2. In the repository settings, open **Actions → General** and ensure workflows
   have **Read and write permissions**.
3. Create and push a version tag:

   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

The **Build Flatpak release** workflow builds the application and creates a
GitHub Release containing:

- `nautilus-custom.flatpak`
- `install.sh`

## Installing on another computer

Download `install.sh` from the latest GitHub Release, then run:

```bash
chmod +x install.sh
./install.sh
```

The installer supports Arch Linux and Debian-based distributions. It installs
the required host packages, configures Flathub, downloads the latest Flatpak,
and installs it for the current user.

To launch it directly:

```bash
flatpak run org.gnome.Nautilus.Custom
```

## Publishing an update

Commit and push the changes, then create a new tag:

```bash
git tag v1.0.1
git push origin v1.0.1
```

Each new tag produces a new release. Running the latest `install.sh` upgrades
the existing installation.
