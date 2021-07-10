@echo off
set OUT_DIR=build 
set OUT_EXE=main
pushd %OUT_DIR%
clang -O3 ../src/main.cpp -o%OUT_EXE%.exe -luser32.lib -lgdi32.lib -lopengl32.lib -Xlinker /subsystem:console
popd