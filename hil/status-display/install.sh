#!/usr/bin/env bash
# Install the HIL status display as a systemd service on the Pi.
# Run on the Pi as the HIL user from this directory; it calls sudo where needed.
# Safe to run again (updates the installed copy and restarts the service).
set -euo pipefail

if [[ $EUID -eq 0 ]]; then
    echo "Run as the HIL user, not as root (the service runs as this user)." >&2
    exit 1
fi

src=$(cd "$(dirname "$0")" && pwd)
dest=/opt/cim-hil/status-display
service=cim-hil-status.service

sudo install -d -o "$USER" -g "$USER" "$dest"
install -m 644 "$src/status_display.py" "$src/requirements.txt" "$dest/"
python3 -m venv "$dest/venv"
"$dest/venv/bin/pip" install -q -r "$dest/requirements.txt"

sudo tee "/etc/systemd/system/$service" >/dev/null <<EOF
[Unit]
Description=CIM HIL status display (OLED on HAT J4)
After=network-online.target

[Service]
User=$USER
SupplementaryGroups=i2c gpio
RuntimeDirectory=cim-hil
RuntimeDirectoryMode=0775
RuntimeDirectoryPreserve=yes
ExecStart=$dest/venv/bin/python $dest/status_display.py
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable "$service"
sudo systemctl restart "$service"
systemctl --no-pager status "$service" | head -5
