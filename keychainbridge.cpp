/***************************************************************************
  keychainbridge.cpp

  Keychain authentication bridge

  This is much more like a pen test for the auth system than a well
  designed plugin, but hooking into the authentication system,
  that was designed to be secure in the first place is not easy.

  -------------------
  begin                : Nov 21, 2016
  copyright            : (C) 2016 Boundless Spatial Inc.
  author               : Alessandro Pasotti
  email                : apasotti@boundlessgeo.com

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

//
// QGIS Specific includes
//

#include <qgisinterface.h>
#include <qgisgui.h>

#include "keychainbridge.h"
#include "keychainbridgegui.h"

//
// Qt4 Related Includes
//

#include <QAction>
#include <QToolBar>
#include <QMessageBox>
#include <QSettings>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QTimer>

// QtKeyChain library
#include "qtkeychain/keychain.h"

// QGIS classes
#include "auth/qgsauthmanager.h"
#include "qgscredentials.h"
#include "qgscredentialdialog.h"
#include "qgsmessagelog.h"
#include "qgsmessagebar.h"


static const QString sName = QObject::tr( "KeyChain" );
static const QString sDescription = QObject::tr( "Keychain authentication bridge" );
static const QString sCategory = QObject::tr( "authentication" );
static const QString sPluginVersion = QObject::tr( "Version 0.1" );
static const QgisPlugin::PLUGINTYPE sPluginType = QgisPlugin::UI;
static const QString sPluginIcon = ":/keychainbridge/keychainbridge.png";
const QLatin1String KeyChainBridge::sMasterPasswordName( "QGIS-Master-Password" );
const QLatin1String KeyChainBridge::sWalletFolderName( "QGIS" );


//////////////////////////////////////////////////////////////////////
//
// THE FOLLOWING METHODS ARE MANDATORY FOR ALL PLUGINS
//
//////////////////////////////////////////////////////////////////////

/**
 * Constructor for the plugin. The plugin is passed a pointer
 * an interface object that provides access to exposed functions in QGIS.
 * @param theQGisInterface - Pointer to the QGIS interface object
 */
KeyChainBridge::KeyChainBridge( QgisInterface * theQgisInterface ):
    QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType ),
    mQGisIface( theQgisInterface ),
    mUseWallet( true ),
    mMasterPassword( "" ),
    mVerificationError( false ),
    mErrorMessage( "" ),
    mIsDirty( true ),
    mAuthManager( nullptr ),
    mUseWalletAction( nullptr ),
    mLoggingEnabledAction( nullptr ),
    mLoggingEnabled( false )
{
  // Read settings
  readSettings();

  // Connect to Auth Manager
  mAuthManager = QgsAuthManager::instance();
  if ( ! mAuthManager->isDisabled() )
  {
    connect( mAuthManager, SIGNAL( masterPasswordVerified( bool ) ), this, SLOT( masterPasswordVerified( bool ) ) ) ;
    QgsCredentialDialog* credentials = dynamic_cast<QgsCredentialDialog*>( QgsCredentials::instance() );

    if ( credentials )
    {
      credentials->installEventFilter( this );
      Q_ASSERT( connect( credentials, SIGNAL( accepted() ), this, SLOT( credentialsDialogAccepted() ) ) );
      Q_ASSERT( connect( credentials, SIGNAL( credentialsRequested( const QString&, QString *, QString *, const QString&, bool * ) ), this, SLOT( requestCredentials( QString, QString*, QString*, QString, bool* ) ) ) );
      Q_ASSERT( connect( credentials, SIGNAL( credentialsRequestedMasterPassword( QString*, bool, bool* ) ), this, SLOT( requestCredentialsMasterPassword( QString*, bool, bool* ) ) ) );
    }

    // Sync if the authm is open
    if ( mAuthManager->masterPasswordIsSet() )
    {
      askSaveMasterPassword( tr( "Do you want to store your master password now?" ) );
    }
  }
  else
  {
    debug( tr( "Auth manager is disabled." ) );
  }
}

KeyChainBridge::~KeyChainBridge()
{
  writeSettings();
}

/*
 * Initialize the GUI interface for the plugin - this is only called once when the plugin is
 * added to the plugin registry in the QGIS application.
 */
void KeyChainBridge::initGui()
{

  // Create the action for tool
  mQActionPointer = new QAction( QIcon( ":/keychainbridge/keychainbridge.svg" ), tr( "About KeyChain plugin" ), this );
  mQActionPointer->setObjectName( "mQActionPointer" );
  // Set the what's this text
  mQActionPointer->setWhatsThis( tr( "Store the master password in your wallet" ) );
  // Connect the action to the run
  connect( mQActionPointer, SIGNAL( triggered() ), this, SLOT( about() ) );
  // Add the icon to the toolbar
  // mQGisIface->addToolBarIcon( mQActionPointer );
  mQGisIface->addPluginToMenu( tr( "&KeyChain" ), mQActionPointer );

  QAction* action;
  action = new QAction( QIcon( ":/keychainbridge/save.svg" ), tr( "Store/update the master password in your wallet" ), mQGisIface->mainWindow() );
  connect( action, SIGNAL( triggered() ), this, SLOT( on_saveMasterPassword_triggered() ) );
  mQGisIface->addPluginToMenu( tr( "&KeyChain" ), action );
  action = new QAction( QIcon( ":/keychainbridge/trashcan.svg" ), tr( "Clear the master password from your wallet" ), mQGisIface->mainWindow() );
  connect( action, SIGNAL( triggered() ), this, SLOT( on_deleteMasterPassword_triggered() ) );
  mQGisIface->addPluginToMenu( tr( "&KeyChain" ), action );

  mUseWalletAction = new QAction( tr( "Enable the wallet" ), mQGisIface->mainWindow() );
  mUseWalletAction->setCheckable( true );
  mUseWalletAction->setChecked( mUseWallet );
  connect( mUseWalletAction, SIGNAL( changed() ), this, SLOT( on_useWallet_changed() ) );
  mQGisIface->addPluginToMenu( tr( "&KeyChain" ), mUseWalletAction );

  mLoggingEnabledAction = new QAction( tr( "Enable logging" ), mQGisIface->mainWindow() );
  mLoggingEnabledAction->setCheckable( true );
  mLoggingEnabledAction->setChecked( mLoggingEnabled );
  connect( mLoggingEnabledAction, SIGNAL( changed() ), this, SLOT( on_loggingEnabled_changed() ) );
  mQGisIface->addPluginToMenu( tr( "&KeyChain" ), mLoggingEnabledAction );

}
//method defined in interface
void KeyChainBridge::help()
{
  //implement me!
}


/*
 * Here the verified password is stored into the wallet and
 * the cached master password is checked for it validity,
 * There is no way for this plugin to be notified when
 * the password has been reset.
 */
void KeyChainBridge::masterPasswordVerified( bool verified )
{
  debug( QString( tr( "KeyChainBridge::masterPasswordVerified called %1" ) ).arg( verified ) );
  if ( pluginIsEnabled() )
  {
    mVerificationError = ! verified;
    // Check if the cached password is still valid
    if ( verified && ! isDirty() && ! masterPassword().isEmpty() && ! passwordIsSame( masterPassword() ) )
    {
      setIsDirty( true );
      //showInfo( tr("Cached master password is not valid anymore"));
      askSaveMasterPassword( tr( "Master password stored in the wallet is not valid anymore, do you want to update it now?" ) );
    }
    // Check if we have a valid password and we need to store it in the wallet
    if ( verified && isDirty() && passwordIsSame( masterPassword() ) )
    {
      if ( storeMasterPassword( masterPassword() ) )
      {
        setIsDirty( false ); // Password is synced!
        showInfo( tr( "Master password has been successfully stored in your wallet" ) );
      }
      else
      {
        showError( );
      }
    }
  }
}

void KeyChainBridge::requestCredentials( const QString &, QString *, QString *, const QString &, bool * )
{
  debug( tr( "KeyChainBridge::requestCredentials called" ) );
}

void KeyChainBridge::requestCredentialsMasterPassword( QString *password, bool stored, bool *ok )
{
  Q_UNUSED( password );
  Q_UNUSED( stored );
  Q_UNUSED( ok );
  debug( tr( "KeyChainBridge::requestCredentialsMasterPassword called" ) );
}

void KeyChainBridge::credentialsDialogAccepted()
{
  QgsCredentialDialog* credentials = static_cast<QgsCredentialDialog*>( QgsCredentials::instance() );
  QLineEdit* leMasterPass =  credentials->findChild<QLineEdit*>( "leMasterPass" );
  QString password = leMasterPass->text();
  if ( ! password.isEmpty() )
  {
    setIsDirty( mMasterPassword != password );
    mMasterPassword = password;
  }
}

void KeyChainBridge::on_saveMasterPassword_triggered()
{
  saveMasterPassword();
}


void KeyChainBridge::on_deleteMasterPassword_triggered()
{
  bool ok = deleteMasterPassword();
  mMasterPassword = "";
  setIsDirty( true );
  if ( ok )
  {
    showInfo( tr( "The master password has been successfully removed from your wallet" ) );
  }
  else
  {
    setErrorMessage( tr( "There was an error deleting the master password from your wallet: %1" ).arg( errorMessage() ) );
    showError();
  }
}

void KeyChainBridge::on_useWallet_changed()
{
  mUseWallet = mUseWalletAction->isChecked();
  writeSettings();
  showInfo( mUseWallet ? tr( "Your wallet will be <b>used from now</b> on to store and retrieve the master password" ) :
            tr( "Your wallet will <b>not be used anymore</b> to store and retrieve the master password" ) );
}

void KeyChainBridge::on_loggingEnabled_changed()
{
  mLoggingEnabled = mLoggingEnabledAction->isChecked();
  writeSettings();
  showInfo( mLoggingEnabled ? tr( "Logging is now <b>enabled</b>" ) :
            tr( "Logging is now <b>disabled</b>" ) );
}

bool KeyChainBridge::deleteMasterPassword()
{
  debug( "Opening wallet for DELETE ..." );
  QKeychain::DeletePasswordJob job( sWalletFolderName );
  job.setAutoDelete( false );
  job.setKey( sMasterPasswordName );
  QEventLoop loop;
  job.connect( &job, SIGNAL( finished( QKeychain::Job* ) ), &loop, SLOT( quit() ) );
  job.start();
  loop.exec();
  if ( job.error() )
  {
    setErrorMessage( QString( tr( "Delete password failed: %1" ) ).arg( job.errorString() ) );
    setIsDirty( true );
    return false;
  }
  else
  {
    setIsDirty( false );
    setErrorMessage( "" );
    return true;
  }
}



/*
 * Here it is the core plugin functionality:
 * Inject the password into the credentials dialog and accept it
 */
bool KeyChainBridge::eventFilter( QObject *obj, QEvent *event )
{
  // Don't even try!
  if ( ! pluginIsEnabled() )
  {
    return QObject::eventFilter( obj, event );
  }

  QgsCredentialDialog* credentials = qobject_cast<QgsCredentialDialog*>( obj );

  // Dialog is about to be shown: QGIS wants a master password
  if ( credentials && event->type() == QEvent::Show )
  {
    // If there was an error, we do not want to enter this pwd again, and again ...
    if ( ! mVerificationError )
    {
      // Retrieve master password from wallet
      mMasterPassword = readMasterPassword();
      if ( ! mMasterPassword.isEmpty() )
      {
        setIsDirty( false );
        QLineEdit* leMasterPass =  credentials->findChild<QLineEdit*>( "leMasterPass" );
        // Hackish!!!
        if ( leMasterPass->styleSheet() != "QLineEdit{color: rgb(200, 0, 0);}" )
        {
          leMasterPass->setText( mMasterPassword );
          QTimer::singleShot( 0, credentials, SLOT( accept() ) );
          showInfo( tr( "Master password has been successfully inserted from wallet!" ) );
        }
        else
        {
          setErrorMessage( "It seems like the password stored in the wallet is no longer valid" );
          showError();
          setIsDirty( true );
          mMasterPassword = "";
        }
      }
      else   // We've got an error
      {
        showError();
      }
    }
    return QObject::eventFilter( obj, event );
  }
  else
  {
    // standard event processing
    return QObject::eventFilter( obj, event );
  }
}

void KeyChainBridge::debug( QString msg )
{
  if ( mLoggingEnabled )
  {
    QgsMessageLog::logMessage( msg, name() );
  }
}

void KeyChainBridge::readSettings()
{
  QSettings settings;
  mUseWallet = settings.value( QString( "%1/useWallet" ).arg( name() ), true ).toBool();
  mLoggingEnabled = settings.value( QString( "%1/loggingEnabled" ).arg( name() ), true ).toBool();
}

void KeyChainBridge::writeSettings()
{
  QSettings settings;
  settings.setValue( QString( "%1/useWallet" ).arg( name() ), mUseWallet );
  settings.setValue( QString( "%1/loggingEnabled" ).arg( name() ), mLoggingEnabled );
}

bool KeyChainBridge::pluginIsEnabled()
{
  return mUseWallet && ! mAuthManager->isDisabled();
}

void KeyChainBridge::askSaveMasterPassword( QString message )
{
  QWidget *wdg = new QWidget( mQGisIface->mainWindow() );
  QHBoxLayout *hlay = new QHBoxLayout( wdg );
  QLabel *lbl = new QLabel( message, wdg );
  hlay->addWidget( lbl );
  QPushButton *btn1 = new QPushButton( tr( "Store/Update" ), wdg );
  connect( btn1, SIGNAL( clicked() ), this, SLOT( on_saveMasterPassword_triggered() ) );
  hlay->addWidget( btn1 );
  mQGisIface->messageBar()->pushWidget( wdg );
}



bool KeyChainBridge::passwordIsSame( QString password )
{
  // Note that this may fail if the DB is not open
  return mAuthManager->masterPasswordSame( password );
}

QString KeyChainBridge::readMasterPassword()
{
  // Retrieve it!
  QString password( "" );
  debug( "Opening wallet for READ ..." );
  QKeychain::ReadPasswordJob job( sWalletFolderName );
  job.setAutoDelete( false );
  job.setKey( sMasterPasswordName );
  QEventLoop loop;
  job.connect( &job, SIGNAL( finished( QKeychain::Job* ) ), &loop, SLOT( quit() ) );
  job.start();
  loop.exec();
  if ( job.error() )
  {
    setErrorMessage( QString( tr( "Retrieving password failed: %1" ) ).arg( job.errorString() ) );
  }
  else
  {
    password = job.textData();
    setErrorMessage( password.isEmpty() ? tr( "Empty password retrieved from wallet" ) : "" );
  }
  return password;
}

bool KeyChainBridge::storeMasterPassword( QString password )
{

  Q_ASSERT( !password.isEmpty() );
  debug( "Opening wallet for WRITE ..." );
  QKeychain::WritePasswordJob job( sWalletFolderName );
  job.setAutoDelete( false );
  job.setKey( sMasterPasswordName );
  job.setTextData( password );
  QEventLoop loop;
  job.connect( &job, SIGNAL( finished( QKeychain::Job* ) ), &loop, SLOT( quit() ) );
  job.start();
  loop.exec();
  if ( job.error() )
  {
    setErrorMessage( QString( tr( "Storing password failed: %1" ) ).arg( job.errorString() ) );
    setIsDirty( true );
    return false;
  }
  else
  {
    setIsDirty( false );
    setErrorMessage( "" );
    return true;
  }
}


/*
 * Here it is the storage control logic, the real writing in the wallet
 * is delegated to storeMasterPassword()
 */
void KeyChainBridge::saveMasterPassword()
{
  if ( ! mAuthManager->masterPasswordIsSet() || ! passwordIsSame( masterPassword() ) )
  {
    mVerificationError = true; // Prevent the password being inserted from the wallet if it's already there
    mAuthManager->clearMasterPassword();
    mAuthManager->setMasterPassword( true );
  }
  if ( ! masterPassword().isEmpty() )
  {
    if ( storeMasterPassword( masterPassword() ) )
    {
      showInfo( tr( "Master password has been successfully stored in your wallet!" ) );
    }
    else
    {
      showError();
    }
  }
  else
  {
    setErrorMessage( tr( "Could not retrieve the master password from the wallet" ) );
    showError( );
  }
}

void KeyChainBridge::showError()
{
  QString message( mErrorMessage.isEmpty() ? QString( tr( "Generic %1 plugin error" ) ).arg( name() ) : mErrorMessage );
  mQGisIface->messageBar()->pushCritical( QString( tr( "%1 plugin error" ) ).arg( name() ), message );
  debug( message );
}

void KeyChainBridge::showInfo( QString message )
{
  mQGisIface->messageBar()->pushInfo( QString( tr( "%1 plugin info" ) ).arg( name() ), message );
  debug( message );
}


// Slot called when the menu item is triggered
// If you created more menu items / toolbar buttons in initiGui, you should
// create a separate handler for each action - this single run() method will
// not be enough
void KeyChainBridge::about()
{
  KeyChainBridgeGui *myPluginGui = new KeyChainBridgeGui( mQGisIface->mainWindow(), QgisGui::ModalDialogFlags );
  myPluginGui->setAttribute( Qt::WA_DeleteOnClose );
  myPluginGui->show();
}

// Unload the plugin by cleaning up the GUI
void KeyChainBridge::unload()
{
  // remove the GUI
  mQGisIface->removePluginMenu( "&KeyChain", mQActionPointer );
  mQGisIface->removeToolBarIcon( mQActionPointer );
  delete mUseWalletAction;
  delete mLoggingEnabledAction;
  delete mQActionPointer;
}


//////////////////////////////////////////////////////////////////////////
//
//
//  THE FOLLOWING CODE IS AUTOGENERATED BY THE PLUGIN BUILDER SCRIPT
//    YOU WOULD NORMALLY NOT NEED TO MODIFY THIS, AND YOUR PLUGIN
//      MAY NOT WORK PROPERLY IF YOU MODIFY THIS INCORRECTLY
//
//
//////////////////////////////////////////////////////////////////////////


/**
 * Required extern functions needed  for every plugin
 * These functions can be called prior to creating an instance
 * of the plugin class
 */
// Class factory to return a new instance of the plugin class
QGISEXTERN QgisPlugin * classFactory( QgisInterface * theQgisInterfacePointer )
{
  return new KeyChainBridge( theQgisInterfacePointer );
}
// Return the name of the plugin - note that we do not user class members as
// the class may not yet be insantiated when this method is called.
QGISEXTERN QString name()
{
  return sName;
}

// Return the description
QGISEXTERN QString description()
{
  return sDescription;
}

// Return the category
QGISEXTERN QString category()
{
  return sCategory;
}

// Return the type (either UI or MapLayer plugin)
QGISEXTERN int type()
{
  return sPluginType;
}

// Return the version number for the plugin
QGISEXTERN QString version()
{
  return sPluginVersion;
}

QGISEXTERN QString icon()
{
  return sPluginIcon;
}

// Delete ourself
QGISEXTERN void unload( QgisPlugin * thePluginPointer )
{
  delete thePluginPointer;
}
