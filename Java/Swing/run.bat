@echo off
cd /d %~dp0
start "" javaw -Xms1G -Xmx1G -XX:+UseParallelGC -XX:+UnlockDiagnosticVMOptions -XX:+UseSuperWord -jar build\LanguagePerformance.jar