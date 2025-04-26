@echo off
wsl -d ubuntu -u root bash -c "echo nameserver 8.8.8.8 > /etc/resolv.conf"
wsl -d ubuntu -u root bash -c "echo generateResolveConf=false >> /etc/wsl.conf"
echo Installing Metamod
wsl -d ubuntu -u gameserver cd /var/tf2server; curl https://mms.alliedmods.net/mmsdrop/1.12/`curl https://mms.alliedmods.net/mmsdrop/1.12/mmsource-latest-linux` -o metamod.tar.gz; tar -xf metamod.tar.gz -C ../tf2server/tf;
echo Installing Sourcemod
wsl -d ubuntu -u gameserver cd /var/tf2server; curl https://sm.alliedmods.net/smdrop/1.12/`curl https://sm.alliedmods.net/smdrop/1.12/sourcemod-latest-linux` -o sourcemod.tar.gz; tar -xf sourcemod.tar.gz -C ../tf2server/tf --exclude "addons/sourcemod/configs" --exclude "cfg";
echo Installing Sigsegv-MvM extension
wsl -d ubuntu -u gameserver cd /var/tf2server; curl -LJ https://github.com/rafradek/sigsegv-mvm/releases/latest/download/package-linux.zip -o package-linux.zip; unzip -o -x "cfg/*" -d ../tf2server/tf package-linux.zip; 
echo Update complete
