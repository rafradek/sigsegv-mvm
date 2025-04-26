@echo off
wsl -d ubuntu -u root bash -c "echo nameserver 8.8.8.8 > /etc/resolv.conf"
wsl -d ubuntu -u root bash -c "echo generateResolveConf=false >> /etc/wsl.conf"
wsl -d ubuntu -u gameserver cd /var/steamcmd; ./steamcmd.sh +force_install_dir ../tf2server +login anonymous +app_update 232250 +quit
wsl -d ubuntu -u gameserver /var/tf2server/srcds_run +maxplayers 32 +map mvm_bigrock -enablefakeip
