#!/bin/bash

BOSSAC_PATH="BOSSA/bossac"

usage() { echo "usage: flash_clearcore.sh -f <.bin file>"; exit 1;}

# Parse args
while getopts ":f:" arg; do
    case "${arg}" in
        f)
            file=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done

if [[ -z "$file" ]]; then
    usage
    exit 1
fi

# Verify bossac exists
if ! [[ -f "$BOSSAC_PATH" ]]; then
    echo "Cannot find the bossac tool"
    exit 1
fi

# Enter in bootloader mode
$BOSSAC_PATH --debug --arduino-erase

# Flash the binary file
$BOSSAC_PATH --debug --usb-port --write --erase --verify --offset=0x4000 --reset $file
if [ $? != 0 ]; then
    echo "Error: failed to flash"
fi