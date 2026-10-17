@echo off
start "Perf Compare" cmd /k python -m http.server 8000
timeout /t 1 >nul
start "" "http://127.0.0.1:8000"
exit