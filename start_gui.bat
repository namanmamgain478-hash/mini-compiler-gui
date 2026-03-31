@echo off

echo Starting Backend Server...
cd /d "D:\my project\mini compiler\mini compiler\mini-compiler-gui\backend"
start cmd /k node server.js

timeout /t 3 >nul

echo Opening Frontend...
cd /d "D:\my project\mini compiler\mini compiler\mini-compiler-gui\frontend"
start index.html

echo Done!
pause