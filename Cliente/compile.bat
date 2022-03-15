@echo off
set /p ip="IP do servidor: "
set /p grupo="Grupo a entrar: "

g++ Cliente.cpp -o Cliente -lws2_32

Cliente %ip% %grupo%