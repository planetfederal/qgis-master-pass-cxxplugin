#ifndef TESTPREFIX_H
#define TESTPREFIX_H

#include <QString>
#include <QFile>

#include "qgsconfig.h"

#include <stdlib.h>

inline void setPrefixEnviron()
{
  QString qgis_prefix( "" );
#ifdef QGIS_BUILD_PATH
  QString build_path( QGIS_BUILD_PATH );
  if ( !build_path.isEmpty() )
  {
    qgis_prefix += QString( QGIS_BUILD_PATH ) + "/output/bin";
#ifdef Q_OS_MACX
    qgis_prefix += "/QGIS.app/Contents/MacOS";
#endif
  }
#else
  // this is for CMAKE_INSTALL_PREFIX from qgsconfig.h, not this project
  qgis_prefix = CMAKE_INSTALL_PREFIX;
#endif

  if ( !qgis_prefix.isEmpty() && QFile::exists( qgis_prefix ) )
  {
#ifdef Q_OS_WIN
    _putenv( QString( "%1=%2" ).arg( "QGIS_PREFIX_PATH", qgis_prefix.toUtf8().constData() ).toUtf8().constData( ) );
#else
    setenv( "QGIS_PREFIX_PATH", qgis_prefix.toUtf8().constData(), 1 );
#endif
          }
        }

#endif // TESTPREFIX_H
