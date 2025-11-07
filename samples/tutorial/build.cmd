@echo off
setlocal

if "%~1"=="" goto calc
if /i "%~1"=="calc" goto calc
if /i "%~1"=="clean" goto clean

echo.
echo ERROR: Invalid target.
echo Usage: %~nx0 [calc | clean]
goto :eof

:calc

echo Generating calc.cpp from calc.y...
ycc -f calc.y -a
if errorlevel 1 (
    echo ERROR: ycc generation failed. Is 'ycc' in your system PATH?
    exit /b 1
)


echo Compiling 'calc.cpp' to 'calc.exe'...
cl calc.cpp
if errorlevel 1 (
    echo.
    echo ERROR: C++ Compilation failed.
    exit /b 1
)

goto :eof

:clean
echo Deleting calc.log...
del /f /q calc.log 2>nul

echo Deleting calc.cpp...
del /f /q calc.cpp 2>nul

echo Deleting calc executable (calc.exe)...
del /f /q calc.exe 2>nul

goto :eof


endlocal