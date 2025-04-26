@echo off
cd /d "%~dp0"
echo Setting default wsl version to 2
wsl --set-default-version  2
echo.
echo ----------------------
echo Installing ubuntu
echo ----------------------
ubuntu install --root
echo.
echo ----------------------
echo Installing packages
echo ----------------------
echo.
wsl -d ubuntu -u root bash -c "echo nameserver 8.8.8.8 > /etc/resolv.conf"
wsl -d ubuntu -u root bash -c "echo generateResolveConf=false >> /etc/wsl.conf"
wsl -d ubuntu -u root dpkg --add-architecture i386
wsl -d ubuntu -u root apt-get update
wsl -d ubuntu -u root apt install -y python3-venv gcc g++-multilib lbzip2 unzip wget cpufrequtils lib32z1 libncurses6 lib32tinfo6 libbz2-1.0 libcurl4-gnutls-dev lib32stdc++6 libstdc++6:i386 libncurses6:i386 libbz2-1.0:i386 lib32gcc-s1 libc6-i386 libstdc++6 libcurl4-gnutls-dev:i386
wsl -d ubuntu -u root ufw disable
echo.
echo ----------------------
echo Installing game server
echo ----------------------
echo.
wsl -d ubuntu -u root mkdir -p /var/tf2server
wsl -d ubuntu -u root mkdir -p /var/steamcmd
wsl -d ubuntu -u root useradd -m gameserver
wsl -d ubuntu -u root chown -R gameserver:gameserver /var/tf2server
wsl -d ubuntu -u root chown -R gameserver:gameserver /var/steamcmd
wsl -d ubuntu -u gameserver cd /var/steamcmd; wget "https://steamcdn-a.akamaihd.net/client/installer/steamcmd_linux.tar.gz"; tar zxf steamcmd_linux.tar.gz; ./steamcmd.sh +force_install_dir ../tf2server +login anonymous +app_update 232250 +quit
echo.
echo ----------------------
echo Installing Sourcemod
echo ----------------------
echo Installing Metamod
wsl -d ubuntu -u gameserver cd /var/tf2server; curl https://mms.alliedmods.net/mmsdrop/1.11/`curl https://mms.alliedmods.net/mmsdrop/1.11/mmsource-latest-linux` -o metamod.tar.gz; tar -xf metamod.tar.gz -C ../tf2server/tf;
echo Installing Sourcemod
wsl -d ubuntu -u gameserver cd /var/tf2server; curl https://sm.alliedmods.net/smdrop/1.11/`curl https://sm.alliedmods.net/smdrop/1.11/sourcemod-latest-linux` -o sourcemod.tar.gz; tar -xf sourcemod.tar.gz -C ../tf2server/tf;
echo Installing Sigsegv-MvM extension
wsl -d ubuntu -u gameserver cd /var/tf2server; curl -LJ https://github.com/rafradek/sigsegv-mvm/releases/latest/download/package-linux.zip -o package-linux.zip; unzip -d ../tf2server/tf package-linux.zip;
echo.
echo ----------------------
echo INSTALLATION COMPLETE!
echo ----------------------
echo To run the game server, enter the newly created TF2Server directory and run the Run Server.bat file
echo.
echo -------------------------------------------------------------------------------------------
echo NOTE: If your server crashes on launch:
echo 1. sourcemod may not be installed correctly.
echo 2. the latest rafmod release may not be updated.
echo.
echo If the latest release crashes follow these steps:
echo 1. Download LINUX sourcemod directly: https://www.sourcemod.net/downloads.php?branch=stable
echo 2. Grab the latest rafmod build from the latest GitHub action.
echo a.   https://github.com/rafradek/sigsegv-mvm/actions
echo b.   Click on the most recent run and click build-and-package
echo c.   Click on 'Upload package-linux' and click the 'Artifact download URL:' link
echo d.   Unzip the package-linux.zip file, unzip the other one inside too
echo 3. Click 'Browse Files.bat' in TF2Server and manually install sourcemod and rafmod
echo 4. Restart the server
echo -------------------------------------------------------------------------------------------
pause
start /b "" cmd /c del "%~f0"&exit /b
exit
