#!/usr/bin/env bash
# Install fcitx5 + Pinyin so Qt/GTK apps can type Chinese under WSLg.
#
# WSLg ships no input method at all, and Qt6 only bundles the compose (dead-key)
# and ibus platforminputcontext plugins, so the chat client can only accept
# ASCII until an IME daemon exists. fcitx5 is the lighter fit here; its Qt6 and
# GTK3 frontends give both the chat client and DBeaver working input.
#
# Installing the packages alone is not enough: fcitx5 with no Pinyin in its
# profile has nothing to switch to, and its wayland/waylandim addons die under
# WSLg. Both are handled below.
set -euo pipefail

PKGS=(
  fcitx5
  fcitx5-chinese-addons      # Pinyin engine
  fcitx5-frontend-qt6        # Qt6 platforminputcontext plugin
  fcitx5-frontend-gtk3       # GTK3 apps (DBeaver)
  fcitx5-config-qt           # GUI configuration
)

sudo apt-get update
sudo apt-get install -y "${PKGS[@]}"

CONF_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/fcitx5"
mkdir -p "$CONF_DIR"

# Without a pinyin entry the profile only offers keyboard-us, so Ctrl+Space has
# nothing to toggle to. Write the default group only when pinyin is missing, so
# an existing customized profile is left alone.
if ! grep -q '^Name=pinyin' "$CONF_DIR/profile" 2>/dev/null; then
  cat > "$CONF_DIR/profile" <<'EOF'
[Groups/0]
Name=Default
Default Layout=us
DefaultIM=pinyin

[Groups/0/Items/0]
Name=keyboard-us
Layout=

[Groups/0/Items/1]
Name=pinyin
Layout=

[GroupOrder]
0=Default
EOF
  echo "wrote $CONF_DIR/profile (added Pinyin)" >&2
fi

# A systemd user unit keeps the daemon alive across sessions. wayland/waylandim
# must stay disabled: WSLg's compositor denies zwp_input_method_v1 and the
# resulting fatal Wayland error takes the whole daemon down.
UNIT_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user"
mkdir -p "$UNIT_DIR"
cat > "$UNIT_DIR/fcitx5.service" <<'EOF'
[Unit]
Description=Fcitx5 input method (WSLg)
Documentation=https://fcitx-im.org/
After=default.target

[Service]
Type=simple
Environment=DISPLAY=:0
Environment=QT_IM_MODULE=fcitx
Environment=GTK_IM_MODULE=fcitx
Environment=XMODIFIERS=@im=fcitx
ExecStart=/usr/bin/fcitx5 --disable wayland,waylandim
Restart=on-failure
RestartSec=3

[Install]
WantedBy=default.target
EOF

if command -v systemctl >/dev/null 2>&1 && systemctl --user daemon-reload 2>&1; then
  systemctl --user enable --now fcitx5.service
  echo "enabled fcitx5.service" >&2
else
  echo "systemd user session unavailable; scripts/start_client.sh will start fcitx5 instead" >&2
fi

echo
echo "installed. Next:"
echo "  1. scripts/start_client.sh   # starts fcitx5 + the client"
echo "  2. press Ctrl+Space to switch between English and Pinyin"
echo "  3. fcitx5-config-qt          # add/remove input methods"
