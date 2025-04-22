@echo off
set "params=%*"
cd /d "%~dp0" && ( if exist "%temp%\getadmin.vbs" del "%temp%\getadmin.vbs" ) && fsutil dirty query %systemdrive% 1>nul 2>nul || (  echo Set UAC = CreateObject^("Shell.Application"^) : UAC.ShellExecute "cmd.exe", "/k cd ""%~sdp0"" && %~s0 %params%", "", "runas", 1 >> "%temp%\getadmin.vbs" && "%temp%\getadmin.vbs" && exit /B )
powershell -Command Get-ComputerInfo -property "HyperVisorPresent" | find /i "True"
if not errorlevel 1 (
    echo Hardware Virtualization is enabled
) else (
	powershell -Command Get-ComputerInfo -property "HyperVRequirementVirtualizationFirmwareEnabled" | find /i "True"
	if errorlevel 1 (
        echo Hardware Virtualization is disabled. Enable it in BIOS settings and try again
		pause
		exit
	)
)
wsl --status | find /i "running" >nul
set wsl_running=%errorlevel%
wsl test -e /var/tf2server/tf/gameinfo.txt >nul
set wsl_gameinfo_found=%errorlevel%

if exist TF2Server (
    if %wsl_running%==1 (
        if %wsl_gameinfo_found%==0 (
            choice /C YN /M "Server files already exist. Do you want to fully reinstall the server?"
            if errorlevel 2 (
                rem continue
        ) else (
                echo Reinstalling server...
                wsl rm -rf /var/tf2server
                wsl rm -rf /var/steamcmd
                rmdir /q /s TF2Server
            )
        )
    )
)

mkdir TF2Server
cd TF2Server

(
echo @echo off
echo cd /d "%%~dp0"
echo echo Setting default wsl version to 2
echo wsl --set-default-version  2
echo echo.
echo echo ----------------------
echo echo Installing ubuntu
echo echo ----------------------
echo ubuntu install --root

echo echo.
echo echo ----------------------
echo echo Installing packages
echo echo ----------------------
echo echo.
echo wsl -d ubuntu -u root bash -c "echo nameserver 8.8.8.8 > /etc/resolv.conf"
echo wsl -d ubuntu -u root bash -c "echo generateResolveConf=false >> /etc/wsl.conf"
echo wsl -d ubuntu -u root dpkg --add-architecture i386
echo wsl -d ubuntu -u root apt-get update
echo wsl -d ubuntu -u root apt install -y python3-venv gcc g++-multilib lbzip2 unzip wget cpufrequtils lib32z1 libncurses6 lib32tinfo6 libbz2-1.0 libcurl4-gnutls-dev lib32stdc++6 libstdc++6:i386 libncurses6:i386 libbz2-1.0:i386 lib32gcc-s1 libc6-i386 libstdc++6 libcurl4-gnutls-dev:i386
echo wsl -d ubuntu -u root ufw disable

echo echo.
echo echo ----------------------
echo echo Installing game server
echo echo ----------------------
echo echo.
echo wsl -d ubuntu -u root mkdir -p /var/tf2server
echo wsl -d ubuntu -u root mkdir -p /var/steamcmd
echo wsl -d ubuntu -u root useradd -m gameserver
echo wsl -d ubuntu -u root chown -R gameserver:gameserver /var/tf2server
echo wsl -d ubuntu -u root chown -R gameserver:gameserver /var/steamcmd
echo wsl -d ubuntu -u gameserver cd /var/steamcmd; wget "https://steamcdn-a.akamaihd.net/client/installer/steamcmd_linux.tar.gz"; tar zxf steamcmd_linux.tar.gz; ./steamcmd.sh +force_install_dir ../tf2server +login anonymous +app_update 232250 +quit

echo echo.
echo echo ----------------------
echo echo Installing Sourcemod
echo echo ----------------------
echo echo Installing Metamod
echo wsl -d ubuntu -u gameserver cd /var/tf2server; curl https://mms.alliedmods.net/mmsdrop/1.11/`curl https://mms.alliedmods.net/mmsdrop/1.11/mmsource-latest-linux` -o metamod.tar.gz; tar -xf metamod.tar.gz -C ../tf2server/tf;
echo echo Installing Sourcemod
echo wsl -d ubuntu -u gameserver cd /var/tf2server; curl https://sm.alliedmods.net/smdrop/1.11/`curl https://sm.alliedmods.net/smdrop/1.11/sourcemod-latest-linux` -o sourcemod.tar.gz; tar -xf sourcemod.tar.gz -C ../tf2server/tf;
echo echo Installing Sigsegv-MvM extension
echo wsl -d ubuntu -u gameserver cd /var/tf2server; curl -LJ https://github.com/rafradek/sigsegv-mvm/releases/latest/download/package-linux.zip -o package-linux.zip; unzip -d ../tf2server/tf package-linux.zip;

@REM echo echo.
@REM echo echo ----------------------
@REM echo echo Compiling Accelerator
@REM echo echo ----------------------
@REM echo echo.
@REM echo wsl -d ubuntu -u root rm -rf /var/accelerator; mkdir -p /var/accelerator; cd /var/accelerator; git clone https://github.com/asherkin/accelerator.git .; chmod -R 755 /var/accelerator;
@REM echo wsl -d ubuntu -u root cd /var/accelerator; git clone https://github.com/alliedmodders/ambuild.git --depth 1;git clone https://github.com/alliedmodders/metamod-source.git --depth 1 -b 1.11-dev; git clone --recursive https://github.com/alliedmodders/sourcemod.git --depth 1 -b 1.11-dev; chmod -R 755 /var/accelerator
@REM echo wsl -d ubuntu -u root mkdir -p ~/.venvs; python3 -m venv ~/.venvs/ambuild; chmod -R 755 ~/.venvs; cd /var/accelerator; ~/.venvs/ambuild/bin/pip install /var/accelerator/ambuild
@REM echo wsl -d ubuntu -u root chmod +x /var/accelerator/configure.py
@REM @REM echo wsl -d ubuntu -u root mkdir -p /var/accelerator/build/release; cd /var/accelerator/build/release; CC=gcc CXX=g++ ~/.venvs/ambuild/bin/python /var/accelerator/configure.py --targets=x86 --mms-path=/var/ambuild/metamod-source --sm-path=/var/ambuild/sourcemod
@REM echo wsl -d ubuntu -u root mkdir -p /var/accelerator/build/release; cd /var/accelerator/build/release; CC=gcc CXX=g++ ~/.venvs/ambuild/bin/python /var/accelerator/configure.py
echo echo.
echo echo ----------------------
echo echo INSTALLATION COMPLETE!
echo echo ----------------------
echo echo To run the game server, enter the newly created TF2Server directory and run the Run Server.bat file

echo echo.
echo echo -------------------------------------------------------------------------------------------
echo echo NOTE: If your server crashes on launch:
echo echo 1. sourcemod may not be installed correctly.
echo echo 2. the latest rafmod release may not be updated.
echo echo.
echo echo If the latest release crashes follow these steps:
echo echo 1. Download LINUX sourcemod directly: https://www.sourcemod.net/downloads.php?branch=stable
echo echo 2. Grab the latest rafmod build from the latest GitHub action.
echo echo a.   https://github.com/rafradek/sigsegv-mvm/actions
echo echo b.   Click on the most recent run and click build-and-package
echo echo c.   Click on 'Upload package-linux' and click the 'Artifact download URL:' link
echo echo d.   Unzip the package-linux.zip file, unzip the other one inside too
echo echo 3. Click 'Browse Files.bat' in TF2Server and manually install sourcemod and rafmod
echo echo 4. Restart the server
echo echo -------------------------------------------------------------------------------------------

echo pause
echo start /b "" cmd /c del "%%~f0"^&exit /b
echo exit
) > "SetupSigsegvAfterBoot.bat"

(
echo wsl -d ubuntu -u root bash -c "echo nameserver 8.8.8.8 > /etc/resolv.conf"
echo wsl -d ubuntu -u root bash -c "echo generateResolveConf=false >> /etc/wsl.conf"
echo wsl -d ubuntu -u gameserver cd /var/steamcmd; ./steamcmd.sh +force_install_dir ../tf2server +login anonymous +app_update 232250 +quit
echo wsl -d ubuntu -u gameserver /var/tf2server/srcds_run +maxplayers 32 +map mvm_bigrock -enablefakeip
) > "Run Server.bat"

(
echo wsl -d ubuntu -u root bash -c "echo nameserver 8.8.8.8 > /etc/resolv.conf"
echo wsl -d ubuntu -u root bash -c "echo generateResolveConf=false >> /etc/wsl.conf"
echo echo Installing Metamod
echo wsl -d ubuntu -u gameserver cd /var/tf2server; curl https://mms.alliedmods.net/mmsdrop/1.12/`curl https://mms.alliedmods.net/mmsdrop/1.12/mmsource-latest-linux` -o metamod.tar.gz; tar -xf metamod.tar.gz -C ../tf2server/tf;
echo echo Installing Sourcemod
echo wsl -d ubuntu -u gameserver cd /var/tf2server; curl https://sm.alliedmods.net/smdrop/1.12/`curl https://sm.alliedmods.net/smdrop/1.12/sourcemod-latest-linux` -o sourcemod.tar.gz; tar -xf sourcemod.tar.gz -C ../tf2server/tf --exclude "addons/sourcemod/configs" --exclude "cfg";
echo echo Installing Sigsegv-MvM extension
echo wsl -d ubuntu -u gameserver cd /var/tf2server; curl -LJ https://github.com/rafradek/sigsegv-mvm/releases/latest/download/package-linux.zip -o package-linux.zip; unzip -o -x "cfg/*" -d ../tf2server/tf package-linux.zip; 
echo echo Update complete
) > "Update Server.bat"

(
echo wsl -d ubuntu -u gameserver cd /var/tf2server/tf; explorer.exe . 
) > "Browse Files.bat"


echo choice /C YN /M "Do you want to delete the linux VM and all server files?A"> "Delete Server.bat"
echo if errorlevel 2 (>> "Delete Server.bat"
echo Server files are not deleted>> "Delete Server.bat"
echo ) else (>> "Delete Server.bat"
echo wsl --unregister ubuntu>> "Delete Server.bat"
echo cd ..>> "Delete Server.bat"
echo rmdir /q /s TF2Server>> "Delete Server.bat"
echo )>> "Delete Server.bat"

if not exist "%windir%\System32\bash.exe" (
    wsl --install -d ubuntu
    echo [wsl2] > %USERPROFILE%/.wslconfig
    echo memory=2GB >> %USERPROFILE%/.wslconfig

    reg add HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\RunOnce /f /v "WslInstallContinue" /t REG_SZ /d "%COMSPEC% /c """%cd%\SetupSigsegvAfterBoot.bat"""
    echo -------------
    echo -------------
    echo Reboot system to continue installation
    choice /C YN /M "Do you want to reboot the system now?"
    if errorlevel 2 (
    echo Reboot the system later to continue
    ) else (
    shutdown -r -t 0
    )
    pause
) else (
    SetupSigsegvAfterBoot.bat
)
exit