#!/bin/sh

set -eu

repository="aradar46/nautilus-custom"
app_id="org.gnome.Nautilus.Custom"
bundle_name="nautilus-custom.flatpak"
bundle_url="https://github.com/${repository}/releases/latest/download/${bundle_name}"

if [ -r /etc/os-release ]; then
    . /etc/os-release
else
    ID="unknown"
    ID_LIKE=""
fi

distribution="${ID:-unknown} ${ID_LIKE:-}"

install_host_dependencies() {
    case "$distribution" in
        *arch*)
            sudo pacman -S --needed --noconfirm flatpak util-linux curl
            ;;
        *debian*|*ubuntu*)
            sudo apt-get update
            sudo apt-get install -y flatpak util-linux curl
            ;;
        *)
            echo "Install Flatpak, util-linux, and curl, then run this installer again."
            exit 1
            ;;
    esac
}

if ! command -v flatpak >/dev/null 2>&1 ||
   ! command -v script >/dev/null 2>&1 ||
   ! command -v curl >/dev/null 2>&1; then
    install_host_dependencies
fi

flatpak remote-add --user --if-not-exists flathub \
    https://dl.flathub.org/repo/flathub.flatpakrepo

bundle_path="$(mktemp "${TMPDIR:-/tmp}/nautilus-custom.XXXXXX.flatpak")"
trap 'rm -f "$bundle_path"' EXIT HUP INT TERM

echo "Downloading ${bundle_url}"
curl --fail --location --progress-bar "$bundle_url" --output "$bundle_path"

if flatpak info --user "$app_id" >/dev/null 2>&1; then
    flatpak install --user --noninteractive --assumeyes --reinstall "$bundle_path"
else
    flatpak install --user --noninteractive --assumeyes "$bundle_path"
fi

if ! flatpak info --user "$app_id" >/dev/null 2>&1; then
    echo "Installation failed: ${app_id} was not installed." >&2
    exit 1
fi

if ! flatpak run --command=true "$app_id"; then
    echo "Flatpak cannot start apps in this system or VM." >&2
    if [ "$(cat /proc/sys/kernel/unprivileged_userns_clone 2>/dev/null || true)" = "0" ]; then
        echo "Enable user namespaces, then run this installer again:" >&2
        echo "  sudo sysctl kernel.unprivileged_userns_clone=1" >&2
    fi
    exit 1
fi

if command -v xdg-mime >/dev/null 2>&1; then
    xdg-mime default "${app_id}.desktop" inode/directory
    xdg-mime default "${app_id}.desktop" application/x-gnome-saved-search
fi

echo
echo "Installed Custom Files."
echo "Run it from the application menu or with:"
echo "  flatpak run ${app_id}"
