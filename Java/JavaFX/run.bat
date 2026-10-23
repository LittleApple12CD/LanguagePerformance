@echo off
cd /d %~dp0
start "" javaw -Xms1G -Xmx1G -XX:+UseParallelGC -XX:+UnlockDiagnosticVMOptions -XX:+UseSuperWord -Djavafx.animation.fullspeed=true -Dprism.vsync=false -jar target\LanguagePerformance-1.0.jar
