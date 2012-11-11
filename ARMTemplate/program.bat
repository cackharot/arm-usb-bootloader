@echo off
setLocal Enabledelayedexpansion

AT > NUL
IF %ERRORLEVEL% EQU 0 (
	goto gotAdmin
) ELSE (
	REM --> If error flag set, we do not have admin.
    goto UACPrompt
)

:UACPrompt
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    echo UAC.ShellExecute "%~s0", "", "", "runas", 1 >> "%temp%\getadmin.vbs"

    "%temp%\getadmin.vbs"
    exit /B

:gotAdmin
    if exist "%temp%\getadmin.vbs" ( del "%temp%\getadmin.vbs" )
    pushd "%CD%"
    CD /D "%~dp0"
:--------------------------------------

IF NOT EXIST FIRMWARE.BIN GOTO binfilenotfound

set PNPDeviceID=LPC2148
set Q='wmic  diskdrive where "interfacetype='USB' and PNPDeviceID like '%%%PNPDeviceID%%%'"  assoc /assocclass:Win32_DiskDriveToDiskPartition' 
for /f "tokens=2,3,4,5 delims=,= " %%a in (%Q%) do (  
  set hd=%%a %%b, %%c %%d  
  call :_LIST_LETTER !hd!)  

echo %Part_letter: =%\FIRMWARE.BIN >tmp
set /p f=<tmp
IF NOT EXIST %f% GOTO binfilenotfound

wmic diskdrive get index,PNPDeviceID | gawk "/INTRLOGX/ { print $$1 }" > index.txt
set /p index=<index.txt
set index=%index: =%
IF %index%.==. GOTO notfound
echo "Programming the ARM LPC2148 Device via USB Storage at"
echo \\?\Device\Harddisk%index%\Partition1
dd bs=512 seek=8 if=FIRMWARE.BIN of=\\?\Device\Harddisk%index%\Partition1 2> nul
echo Done!
echo Disconnect the DBG_E jumper and hit the RESET button to run your code.
goto cleanexit

 
:_LIST_LETTER  
(echo %1 |find  "Disk ") >nul|| goto :_EOF   
for /f "tokens=3 delims==" %%a in ('WMIC Path Win32_LogicalDiskToPartition  ^|find %1') do set TMP_letter=%%a  
set Part_letter=%TMP_letter:~1,2%   
goto :_EOF  

:notfound
echo Cannot program device.
echo Interlogix LPC2148 Disk not found!
goto cleanexit

:binfilenotfound
echo Cannot program device.
echo FIRMWARE.BIN file not found!
goto cleanexit

:cleanexit
if exist index.txt del /f index.txt
if exist tmp del /f tmp
pause
exit
:_EOF
