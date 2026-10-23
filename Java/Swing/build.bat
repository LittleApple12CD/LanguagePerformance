@echo off
cd /d %~dp0

echo [1/3] Compiling...
if not exist out mkdir out
javac -encoding UTF-8 -d out src\*.java
xcopy /Y /Q src\icon*.png out\ >nul
if errorlevel 1 goto :fail

echo [2/3] Copying data...
if not exist out\data mkdir out\data
xcopy /Y /Q data\*.csv out\data\ >nul

echo [3/3] Packaging JAR...
jar cfm build/LanguagePerformance.jar MANIFEST.MF -C out .

echo.
echo Done: LanguagePerformance.jar
echo.
pause
goto :eof

:fail
echo Build failed.
pause
exit /b 1