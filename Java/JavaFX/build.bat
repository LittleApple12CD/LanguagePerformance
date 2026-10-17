@echo off
cd /d %~dp0

echo [1/4] Copying data to resources...
if not exist src\main\resources\data mkdir src\main\resources\data
xcopy /Y /Q data\*.csv src\main\resources\data\ >nul
if errorlevel 1 goto :fail

echo [2/4] Cleaning...
call mvn clean -q
if errorlevel 1 goto :fail

echo [3/4] Packaging (shade fat JAR)...
call mvn package -q -DskipTests
if errorlevel 1 goto :fail

echo [4/4] Done.
echo.
echo Output: target\LanguagePerformance-1.0.jar
echo.
pause
goto :eof

:fail
echo.
echo BUILD FAILED.
pause
exit /b 1