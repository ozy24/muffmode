@echo off
echo Building Muffmode Game DLL...

REM Find Visual Studio Build Tools
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Navigate to src directory where the solution file is located
cd src

REM Build the project
msbuild game.sln /p:Configuration=Release /p:Platform=x64

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful! 
    echo Game DLL created: game_x64.dll
    echo.
    echo You can now use this DLL for your Quake 2 Muffmode!
) else (
    echo.
    echo Build failed. Check the output above for errors.
)
