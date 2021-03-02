#!/bin/sh
if test "$#" -ne 1; then
    echo "arguments: <server_id>"
    exit
fi

systemctl stop "tfpotatoarch$1"
systemctl disable "tfpotatoarch$1"

rm "/etc/systemd/system/tfpotatoarch$1.service"
