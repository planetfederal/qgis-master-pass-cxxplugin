#!/bin/bash

# exit on errors
set -e

export HB=/Library/Boundless/Desktop/1.0
export PATH=$HB/bin:$HB/sbin:/usr/bin:/bin:/usr/sbin:/sbin
export PYTHONPATH=$HB/lib/qt-4/python2.7/site-packages

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")"; pwd -P)

cd $(dirname "${SCRIPT_DIR}")

rm -Rf build && mkdir build

pushd build

  cmake \
  -DCMAKE_FIND_FRAMEWORK:STRING=LAST \
  -DCMAKE_PREFIX_PATH:STRING="$HB/opt/qt-4;$HB/opt/sip-qt4;$HB/opt/pyqt-qt4;$HB/opt/qca-qt4;$HB/opt/qscintilla2-qt4;$HB/opt/qwt-qt4;$HB/opt/qwtpolar-qt4;$HB/opt/qtkeychain-qt4;$HB/opt/gdal2;$HB/opt/gsl;$HB/opt/geos;$HB/opt/proj;$HB/opt/libspatialite;$HB/opt/spatialindex;$HB/opt/fcgi;$HB/opt/expat;$HB/opt/sqlite;$HB/opt/flex;$HB/opt/bison" \
  -DQGIS_PREFIX_PATH:PATH=$HB/opt/qgis2-bdesk \
  -DQT_QMAKE_EXECUTABLE:FILEPATH=$HB/opt/qt-4/bin/qmake \
  ..

  time cmake --build . --target all

popd
