/***************************************************************************
     testauthbrowser.cpp
     ----------------------
    Date                 : October 2016
    Copyright            : (C) 2016 by Boundless Spatial, Inc. USA
    Author               : Larry Shaffer
    Email                : lshaffer at boundlessgeo dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QtTest/QtTest>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QTemporaryFile>

#include "testutils.h"
#include <qapplication.h>
#include "qgsapplication.h"
#include "qgsauthmanager.h"

#include <stdio.h>
#include <stdlib.h>


inline QTextStream& qStdout()
{
  static QTextStream r( stdout );
  return r;
}

void suppressDebugHandler( QtMsgType type, const char *msg )
{
  switch ( type )
  {
    case QtDebugMsg:
      break;
    case QtWarningMsg:
      fprintf( stderr, "Warning: %s\n", msg );
      break;
    case QtCriticalMsg:
      fprintf( stderr, "Critical: %s\n", msg );
      break;
    case QtFatalMsg:
      fprintf( stderr, "Fatal: %s\n", msg );
      abort();
  }
}

/** \ingroup UnitTests
 * Unit tests for QGIS Auth Test Browser
 */
class TestKeychainBridgePlugin: public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testKeychainBridgePlugin();

  private:
    static QString smHashes;
//    static QObject *smParentObj;
    QtMsgHandler mOrigMsgHandler;
};

QString TestKeychainBridgePlugin::smHashes = "#####################";
//QObject *TestKeychainBridgePlugin::smParentObj = new QObject();

void TestKeychainBridgePlugin::initTestCase()
{
  setPrefixEnviron();
  mOrigMsgHandler = qInstallMsgHandler( suppressDebugHandler );

  QgsApplication::init();
  QgsApplication::initQgis();
  if ( QgsAuthManager::instance()->isDisabled() )
    QSKIP( "Auth system is disabled, skipping test case", SkipAll );

  qInstallMsgHandler( mOrigMsgHandler );

//  qDebug() << QgsApplication::showSettings();
}

void TestKeychainBridgePlugin::cleanupTestCase()
{
  qDebug() << "\n";
  qInstallMsgHandler( suppressDebugHandler );
  QgsApplication::exitQgis();
}

void TestKeychainBridgePlugin::init()
{
  qStdout() << "\n" << smHashes << " Start "
  << QTest::currentTestFunction() << " " << smHashes << "\n";
  qStdout().flush();
}

void TestKeychainBridgePlugin::cleanup()
{
  qStdout() << smHashes << " End "
  << QTest::currentTestFunction() << " " << smHashes << "\n";
  qStdout().flush();
}

void TestKeychainBridgePlugin::testKeychainBridgePlugin()
{
  qDebug() << "Entered";
}

QTEST_MAIN( TestKeychainBridgePlugin )
#include "testkeychainbridgeplugin.moc"
