@echo off

call "Z:\qgis-build\build\win\build-batch-scripts\config-paths-64.bat"
call "%OSGEO4W_ROOT%\bin\o4w_env.bat"

set O4W_ROOT=%OSGEO4W_ROOT:\=/%
set LIB_DIR=%O4W_ROOT%

call "%PF%\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.Cmd" /x64 /Release

REM There is no 64-bit compiler in VS 2010 Express
REM call "%PF86%\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" amd64

set PYTHONPATH=
path %DEV_TOOLS%\bscmake;%PATH%;%DEV_TOOLS%\Ninja;%CMAKE_BIN%;%GIT_BIN%;%CYGWIN_BIN%;%QTCREATOR_BIN%

set LIB=%LIB%;%OSGEO4W_ROOT%\lib;%OSGEO4W_ROOT%\apps\Python27\libs
set INCLUDE=%INCLUDE%;%OSGEO4W_ROOT%\include

set BUILDCONF=RelWithDebInfo
set QGIS_DIR_NAME=qgis-ltr-dev

echo "PATH: %PATH%"
echo "LIB: %LIB%"
echo "INCLUDE: %INCLUDE%"

cd "%~dp0\.."

rd /s /q build
md build
cd /D build

REM cmake -G "Ninja" ^
cmake -G "Visual Studio 10 Win64" ^
  -D OSGEO4W_ROOT_DIR=%O4W_ROOT% ^
  -D CMAKE_BUILD_TYPE=%BUILDCONF% ^
  -D CMAKE_CONFIGURATION_TYPES=%BUILDCONF% ^
  -D CMAKE_CXX_FLAGS_RELWITHDEBINFO="/MD /Zi /MP /Od /D NDEBUG /D QGISDEBUG" ^
  -D CMAKE_PREFIX_PATH:STRING=%O4W_ROOT%/apps/%QGIS_DIR_NAME%;%O4W_ROOT% ^
  -D QGIS_PLUGIN_DIR:PATH=%O4W_ROOT%/apps/%QGIS_DIR_NAME%/plugins ^
  ..
if errorlevel 1 goto error

cmake --build . --config %BUILDCONF%
if errorlevel 1 goto error

cmake --build . --target INSTALL --config %BUILDCONF%
if errorlevel 1 goto error

:finish
exit /b

:error
echo Failed with error #%errorlevel%.
exit /b %errorlevel%
