@echo off
echo Building Velocity...

if not exist build mkdir build
cd build

cmake .. -G "Visual Studio 17 2022" -A Win32
cmake --build . --config Release

echo.
echo Done! Executable: build\Release\Velocity.exe
pause
