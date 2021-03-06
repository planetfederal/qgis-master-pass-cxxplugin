/***************************************************************************
 *   Copyright (C) 2016 by Boundless Spatial                               *
 *   apasotti@boundlessgeo.com                                             *
 *                                                                         *
 *   This is a plugin generated from the QGIS plugin template              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include "keychainbridgegui.h"
#include "qgscontexthelp.h"
#include "qgsapplication.h"

//qt includes
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>


KeyChainBridgeGui::KeyChainBridgeGui( QWidget* parent, Qt::WindowFlags fl )
    : QDialog( parent, fl )
{
  setupUi( this );

  // Below is an example of how to make the translators job
  // much easier. Please follow this general guideline for LARGE
  // pieces of text. One-liners can be added in the .ui file

  // Note: Format does not relate to translation.
  QString format( "<html><body>"
                  "<h3>%1</h3>"
                  "<p>%2</p>"
                  "<p>%3</p>"
                  "</body></html>" );


  // Note: Translatable strings below
  QString text = format
                 .arg( tr( "Master Password Helper Plugin" ) )
                 .arg( tr( "This plugin enables storage and synchronization of the master password with an external wallet." ) )
                 .arg( tr( "The plugin does its best to keep the master password in sync with the one stored in the wallet, but"
                           "in a few cases (like password reset) it will not be able to capture the password from the user input and "
                           "you will be asked to enter the master password again."));

  textBrowser->setHtml( text );
}

KeyChainBridgeGui::~KeyChainBridgeGui()
{
}

void KeyChainBridgeGui::on_buttonBox_accepted()
{
  //close the dialog
  accept();
}

void KeyChainBridgeGui::on_buttonBox_rejected()
{
  reject();
}

void KeyChainBridgeGui::on_buttonBox_helpRequested()
{
  // Expects a local file path
  QSettings settings;
  QDesktopServices::openUrl( QUrl::fromLocalFile( settings.value( "plugins/master_password_helper/help_uri", QgsApplication::pkgDataPath() + "/doc/master_password_helper/index.html" ).toString( ) ) );
}
