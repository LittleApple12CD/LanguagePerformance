@echo off
cd /d %~dp0
javaw -Xms512m -Xmx2g -jar build\LanguagePerformance.jar
pause