@echo off
set "params=%*"
cd /d "%~dp0" && ( if exist "%temp%\getadmin.vbs" del "%temp%\getadmin.vbs" ) && fsutil dirty query %systemdrive% 1>nul 2>nul || (  echo Set UAC = CreateObject^("Shell.Application"^) : UAC.ShellExecute "cmd.exe", "/k cd ""%~sdp0"" && %~s0 %params%", "", "runas", 1 >> "%temp%\getadmin.vbs" && "%temp%\getadmin.vbs" && exit /B )

mkdir TF2Server
cd TF2Server


(
echo @echo off
echo cd /d "%%~dp0"
echo echo Setting default wsl version to 2
echo wsl --set-default-version  2
echo echo -
echo echo ----------------------
echo echo Installing ubuntu
echo echo ----------------------
echo ubuntu install --root

echo echo -
echo echo ----------------------
echo echo Installing packages
echo echo ----------------------
echo wsl -d ubuntu -u root dpkg --add-architecture i386
echo wsl -d ubuntu -u root apt-get update
echo wsl -d ubuntu -u root apt install -y unzip lib32z1 libncurses5:i386 libbz2-1.0:i386 lib32gcc1 lib32stdc++6 libtinfo5:i386 libcurl3-gnutls:i386
echo wsl -d ubuntu -u root ufw disable

echo echo -
echo echo ----------------------
echo echo Installing game server
echo echo ----------------------
echo wsl -d ubuntu -u root mkdir -p /var/tf2server
echo wsl -d ubuntu -u root useradd -m gameserver
echo wsl -d ubuntu -u root chown -R gameserver:gameserver /var/tf2server
echo wsl -d ubuntu -u gameserver cd /var/tf2server; wget "https://steamcdn-a.akamaihd.net/client/installer/steamcmd_linux.tar.gz"; tar zxf steamcmd_linux.tar.gz; ./steamcmd.sh  +force_install_dir ./tf +login anonymous +app_update 232250 +quit 

echo echo -
echo echo ----------------------
echo echo Installing Sourcemod
echo echo ----------------------
echo echo Installing Metamod
echo wsl -d ubuntu -u gameserver cd /var/tf2server; curl https://mms.alliedmods.net/mmsdrop/1.11/`curl https://mms.alliedmods.net/mmsdrop/1.11/mmsource-latest-linux` -o metamod.tar.gz; tar -xf metamod.tar.gz -C ./tf/tf/;
echo echo Installing Sourcemod
echo wsl -d ubuntu -u gameserver cd /var/tf2server; curl https://sm.alliedmods.net/smdrop/1.11/`curl https://sm.alliedmods.net/smdrop/1.11/sourcemod-latest-linux` -o sourcemod.tar.gz; tar -xf sourcemod.tar.gz -C ./tf/tf/; 
echo echo Installing Sigsegv-MvM extension
echo wsl -d ubuntu -u gameserver cd /var/tf2server; curl -LJ https://github.com/rafradek/sigsegv-mvm/releases/latest/download/package-linux.zip -o package-linux.zip; unzip -d ./tf/tf/ package-linux.zip;
echo echo -
echo echo ----------------------
echo echo INSTALLATION COMPLETE!
echo echo ----------------------
echo echo To run the game server, enter the newly created TF2Server directory and run the Run Server.bat file

echo pause
echo start /b "" cmd /c del "%%~f0"^&exit /b
echo exit
) > "SetupSigsegvAfterBoot.bat"

echo SET port=27015> "Run Server.bat"
echo FOR /F "tokens=* USEBACKQ" %%%%F IN (`wsl hostname -I`) DO (>> "Run Server.bat"
echo SET ip=%%%%F>> "Run Server.bat"
echo )>> "Run Server.bat"
echo start /b %%~dp0\sudppipe\sudppipe.exe -q -b 0.0.0.0 %%ip%% %%port%% %%port%%>> "Run Server.bat"
echo wsl -d ubuntu -u gameserver cd /var/tf2server; ./steamcmd.sh +force_install_dir ./tf +login anonymous +app_update 232250 +quit>> "Run Server.bat"
echo wsl -d ubuntu -u gameserver /var/tf2server/tf/srcds_run +map bigrock +maxplayers 32 -port 27015 -address 0.0.0.0>> "Run Server.bat"
echo wmic process where "commandline like '%%%%-q -b 0.0.0.0 %%%%ip%% %%port%% %%port%%%%'" delete>> "Run Server.bat"
(
echo echo Installing Metamod
echo wsl -d ubuntu -u gameserver cd /var/tf2server; curl https://mms.alliedmods.net/mmsdrop/1.11/`curl https://mms.alliedmods.net/mmsdrop/1.11/mmsource-latest-linux` -o metamod.tar.gz; tar -xf metamod.tar.gz -C ./tf/tf/;
echo echo Installing Sourcemod
echo wsl -d ubuntu -u gameserver cd /var/tf2server; curl https://sm.alliedmods.net/smdrop/1.11/`curl https://sm.alliedmods.net/smdrop/1.11/sourcemod-latest-linux` -o sourcemod.tar.gz; tar -xf sourcemod.tar.gz -C ./tf/tf/ --exclude "addons/sourcemod/configs" --exclude "cfg";
echo echo Installing Sigsegv-MvM extension
echo wsl -d ubuntu -u gameserver cd /var/tf2server; curl -LJ https://github.com/rafradek/sigsegv-mvm/releases/latest/download/package-linux.zip -o package-linux.zip; unzip -d ./tf/tf/ package-linux.zip; 
echo echo Update complete
) > "Update Server.bat"

(
echo wsl -d ubuntu -u gameserver cd /var/tf2server/tf; explorer.exe . 
) > "Browse Files.bat"


echo choice /C YN /M "Do you want to delete the linux VM and all server files ? [Y/N]"> "Delete Server.bat"
echo if errorlevel 2 (>> "Delete Server.bat"
echo Server files are not deleted>> "Delete Server.bat"
echo ) else (>> "Delete Server.bat"
echo wsl --unregister ubuntu>> "Delete Server.bat"
echo cd ..>> "Delete Server.bat"
echo rmdir /q /s TF2Server>> "Delete Server.bat"
echo )>> "Delete Server.bat"


curl http://aluigi.altervista.org/mytoolz/sudppipe.zip > sudppipe.zip
powershell -Command Expand-Archive -Force sudppipe.zip sudppipe

if not exist "%windir%\System32\bash.exe" (
    wsl --install -d ubuntu
    echo [wsl2] memory=2GB > %USERPROFILE%/.wslconfig
    
    reg add HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\RunOnce /f /v "WslInstallContinue" /t REG_SZ /d "%COMSPEC% /c """%cd%\SetupSigsegvAfterBoot.bat"""
    echo -------------
    echo -------------
    echo Reboot system to continue installation
    choice /C YN /M "Do you want to reboot the system now? [Y/N]"
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