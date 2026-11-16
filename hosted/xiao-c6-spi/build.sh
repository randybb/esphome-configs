#!/bin/sh
# Build the co-processor firmware with the ESP-IDF that ESPHome already downloaded, and flash it
# when a port is given: ./build.sh [/dev/serial/by-id/usb-Espressif_...]
set -e
cd "$(dirname "$0")"
IDF=$(ls -d "$HOME"/.cache/esphome/idf/frameworks/5.5.* | sort -V | tail -1)
export IDF_PATH="$IDF" IDF_TOOLS_PATH="$HOME/.cache/esphome/idf"
export IDF_PYTHON_ENV_PATH="$HOME/.cache/esphome/idf/penvs/$(basename "$IDF")"
. "$IDF_PATH/export.sh" >/dev/null
[ -f sdkconfig ] || idf.py set-target esp32c6
idf.py build
[ -z "$1" ] || idf.py -p "$1" flash
