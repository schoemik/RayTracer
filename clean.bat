@echo off
set OUT_DIR=build
pushd %OUT_DIR%
del /Q *
popd
if "%1"=="build" if "%2"=="" (build) else (build %2)