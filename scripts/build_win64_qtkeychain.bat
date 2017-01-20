@echo off

REM LS: last successful build: 2017/01/19
REM     off of master branch:  10b2a2baeb5e016d73bc3e88d188eba38466b796 

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

set BUILDCONF=Release

echo "PATH: %PATH%"

cd /D "Z:\src\qtkeychain"

rd /s /q build
md build
cd /D build

REM cmake -G "Ninja" ^
cmake -G "Visual Studio 10 Win64" ^
  -D CMAKE_INSTALL_PREFIX=%O4W_ROOT% ^
  -D CMAKE_BUILD_TYPE=%BUILDCONF% ^
  -D CMAKE_CONFIGURATION_TYPES=%BUILDCONF% ^
  -D CMAKE_PREFIX_PATH:STRING=%OSGEO4W_ROOT% ^
  -D BUILD_WITH_QT4:BOOL=ON ^
  -D BUILD_TRANSLATIONS:BOOL=OFF ^
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

REM Install log example
REM -- Install configuration: "Release"
REM -- Installing: Z:/OSGeo4W64/include/qtkeychain/keychain.h
REM -- Installing: Z:/OSGeo4W64/include/qtkeychain/qkeychain_export.h

REM -- Installing: Z:/OSGeo4W64/lib/qtkeychain.lib
REM -- Installing: Z:/OSGeo4W64/bin/qtkeychain.dll

REM -- Installing: Z:/OSGeo4W64/mkspecs/modules/qt_QtKeychain.pri
REM -- Installing: Z:/OSGeo4W64/lib/cmake/QtKeychain/QtKeychainLibraryDepends.cmake
REM -- Installing: Z:/OSGeo4W64/lib/cmake/QtKeychain/QtKeychainLibraryDepends-release.cmake
REM -- Installing: Z:/OSGeo4W64/lib/cmake/QtKeychain/QtKeychainConfig.cmake
REM -- Installing: Z:/OSGeo4W64/lib/cmake/QtKeychain/QtKeychainConfigVersion.cmake
