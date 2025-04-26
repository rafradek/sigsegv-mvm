@echo off
choice /C YN /M "Do you want to delete the linux VM and all server files?"
if errorlevel 2 (
echo Server files are not deleted
) else (
wsl --unregister ubuntu
cd ..
pause
rmdir /q /s TF2Server
)
