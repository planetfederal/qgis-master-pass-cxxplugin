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
#include <QStackedWidget>

// QtKeyChain library
#include "qtkeychain/keychain.h"

// QGIS classes
#include "qgsauthmanager.h"
#include "qgscredentials.h"
#include "qgscredentialdialog.h"
#include "qgsmessagelog.h"
#include "qgsmessagebar.h"


static const QString sName = QObject::tr( "KeyChain" );
static const QString sDescription = QObject::tr( "Master Password <-> KeyChain storage plugin. Store your master password in your Wallet/KeyChain/Password Manager" );
static const QString sCategory = QObject::tr( "authentication" );
static const QString sPluginVersion = QObject::tr( "Version 0.1" );
static const QgisPlugin::PLUGINTYPE sPluginType = QgisPlugin::UI;
static const QString sPluginIcon = ":/keychainbridge/keychainbridge.png";
const QLatin1String KeyChainBridge::sMasterPasswordName( "QGIS-Master-Password" );
const QLatin1String KeyChainBridge::sWalletFolderName( "QGIS" );

#if defined(Q_OS_MAC)
const QString KeyChainBridge::sWalletDisplayName( "KeyChain" );
#elif defined(Q_OS_WIN)
const QString KeyChainBridge::sWalletDisplayName( "Password Manager" );
#elif defined(Q_OS_LINUX)
const QString KeyChainBridge::sWalletDisplayName( "Wallet" );
#else
const QString KeyChainBridge::sWalletDisplayName( "Password Manager" );
#endif


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
    mErrorCode( QKeychain::NoError ),
    mIsDirty( true ),
    mAuthManager( nullptr ),
    mUseWalletAction( nullptr ),
    mLoggingEnabledAction( nullptr ),
    mSaveMasterPasswordAction( nullptr ),
    mClearMasterPasswordAction( nullptr ),
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

    Q_ASSERT( credentials );
    credentials->installEventFilter( this );
    connect( credentials, SIGNAL( accepted() ), this, SLOT( credentialsDialogAccepted() ) );

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
  mAboutAction = new QAction( QIcon( ":/keychainbridge/keychainbridge.svg" ), tr( "About KeyChain plugin" ), this );
  mAboutAction->setObjectName( "KeyChainQActionPointer" );
  // Set the what's this text
  mAboutAction->setWhatsThis( tr( "Store the master password in your %1" ).arg( sWalletDisplayName ) );
  // Connect the action to the run
  connect( mAboutAction, SIGNAL( triggered() ), this, SLOT( about() ) );
  mQGisIface->addPluginToMenu( tr( "&Master Password <-> %1" ).arg( sWalletDisplayName ), mAboutAction );

  mSaveMasterPasswordAction = new QAction( QIcon( ":/keychainbridge/save.svg" ), tr( "Store/update the master password in your %1" ).arg( sWalletDisplayName ), mQGisIface->mainWindow() );
  connect( mSaveMasterPasswordAction, SIGNAL( triggered() ), this, SLOT( on_saveMasterPassword_triggered() ) );
  mQGisIface->addPluginToMenu( tr( "&Master Password <-> %1" ).arg( sWalletDisplayName ), mSaveMasterPasswordAction );
  mClearMasterPasswordAction = new QAction( QIcon( ":/keychainbridge/trashcan.svg" ), tr( "Clear the master password from your %1" ).arg( sWalletDisplayName ), mQGisIface->mainWindow() );
  connect( mClearMasterPasswordAction, SIGNAL( triggered() ), this, SLOT( on_deleteMasterPassword_triggered() ) );
  mQGisIface->addPluginToMenu( tr( "&Master Password <-> %1" ).arg( sWalletDisplayName ), mClearMasterPasswordAction );

  mUseWalletAction = new QAction( tr( "Enable the integration with the %1" ).arg( sWalletDisplayName ), mQGisIface->mainWindow() );
  mUseWalletAction->setCheckable( true );
  mUseWalletAction->setChecked( useWallet( ) );
  connect( mUseWalletAction, SIGNAL( changed() ), this, SLOT( on_useWallet_changed() ) );
  mQGisIface->addPluginToMenu( tr( "&Master Password <-> %1" ).arg( sWalletDisplayName ), mUseWalletAction );

  mLoggingEnabledAction = new QAction( tr( "Enable logging" ), mQGisIface->mainWindow() );
  mLoggingEnabledAction->setCheckable( true );
  mLoggingEnabledAction->setChecked( mLoggingEnabled );
  connect( mLoggingEnabledAction, SIGNAL( changed() ), this, SLOT( on_loggingEnabled_changed() ) );
  mQGisIface->addPluginToMenu( tr( "&Master Password <-> %1" ).arg( sWalletDisplayName ), mLoggingEnabledAction );

}
//method defined in interface
void KeyChainBridge::help()
{
  // FIXME: implement!
  QMessageBox::information( nullptr, "To be completed", "To be completed" );
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
      askSaveMasterPassword( tr( "Master password stored in the %1 is not valid anymore, do you want to update it now?" ).arg( sWalletDisplayName ) );
    }
    // Check if we have a valid password and we need to store it in the wallet
    if ( verified && isDirty() && passwordIsSame( masterPassword() ) )
    {
      if ( storeMasterPassword( masterPassword() ) )
      {
        setIsDirty( false ); // Password is synced!
        showInfo( tr( "Master password has been successfully stored in your %1" ).arg( sWalletDisplayName ) );
      }
      else
      {
        showError( );
      }
    }
  }
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
    debug( tr("Password has been captured successfully") );
  }
  else
  {
    debug( tr("Could not capture the password") );
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
    showInfo( tr( "The master password has been successfully removed from your %1" ).arg( sWalletDisplayName ) );
  }
  else
  {
    showError();
  }
}

void KeyChainBridge::on_useWallet_changed()
{
  setUseWallet( mUseWalletAction->isChecked() );
  writeSettings();
  showInfo( useWallet() ? tr( "Your %1 will be <b>used from now</b> on to store and retrieve the master password" ).arg( sWalletDisplayName ) :
            tr( "Your %1 will <b>not be used anymore</b> to store and retrieve the master password" ).arg( sWalletDisplayName ) );
}

void KeyChainBridge::on_loggingEnabled_changed()
{
  setLoggingEnabled( mLoggingEnabledAction->isChecked() );
  writeSettings();
  showInfo( loggingEnabled( ) ? tr( "Logging is now <b>enabled</b>" ) :
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
    setErrorCode( job.error() );
    setErrorMessage( QString( tr( "Delete password failed: %1" ) ).arg( job.errorString() ) );
    setIsDirty( true );
    return false;
  }
  else
  {
    setIsDirty( false );
    clearErrors();
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
  Q_ASSERT( credentials );
  QStackedWidget* stackedWidget =  credentials->findChild<QStackedWidget*>( "stackedWidget" );
  Q_ASSERT( stackedWidget );

  // Dialog is about to be shown: QGIS wants a master password
  if ( stackedWidget->currentIndex() == 1 &&
       event->type() == QEvent::Show)
  {
    // If there was an error, we do not want to enter this pwd again, and again ...
    if ( ! mVerificationError )
    {
      // Retrieve master password from wallet
      mMasterPassword = readMasterPassword();
      if ( errorCode() == QKeychain::NoError )
      {
        setIsDirty( false );
        QLineEdit* leMasterPass =  credentials->findChild<QLineEdit*>( "leMasterPass" );
        // Hackish!!!
        if ( leMasterPass->styleSheet() != "QLineEdit{color: rgb(200, 0, 0);}" )
        {
          leMasterPass->setText( mMasterPassword );
          QTimer::singleShot( 0, credentials, SLOT( accept() ) );
          showInfo( tr( "Master password has been successfully inserted from %1!" ).arg( sWalletDisplayName ) );
        }
        else
        {
          setErrorMessage( tr("It seems like the password stored in the %1 is no longer valid" ).arg( sWalletDisplayName ) );
          showError();
          setIsDirty( true );
          mMasterPassword = "";
        }
      }
      else   // We've got an error
      {
        // Process the error
        processError();
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
  if ( loggingEnabled( ) )
  {
    QgsMessageLog::logMessage( msg, name() );
  }
}

void KeyChainBridge::readSettings()
{
  QSettings settings;
  setUseWallet( settings.value( QString( "%1/useWallet" ).arg( name() ), true ).toBool() );
  setLoggingEnabled( settings.value( QString( "%1/loggingEnabled" ).arg( name() ), false ).toBool() );
}

void KeyChainBridge::writeSettings()
{
  QSettings settings;
  settings.setValue( QString( "%1/useWallet" ).arg( name() ), useWallet( ) );
  settings.setValue( QString( "%1/loggingEnabled" ).arg( name() ), loggingEnabled( ) );
}

bool KeyChainBridge::pluginIsEnabled()
{
  return useWallet( ) && ! mAuthManager->isDisabled();
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
    setErrorCode( job.error() );
    setErrorMessage( QString( tr( "Retrieving password failed: %1" ) ).arg( job.errorString() ) );
  }
  else
  {
    password = job.textData();
    // Password is there but it is empty, treat it like if it were not found
    if ( password.isEmpty() )
    {
      setErrorCode( QKeychain::EntryNotFound );
      setErrorMessage( tr( "Empty password retrieved from the %1" ).arg( sWalletDisplayName ) );
    }
    else
    {
      clearErrors();
    }
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
    setErrorCode( job.error() );
    setErrorMessage( QString( tr( "Storing password failed: %1" ) ).arg( job.errorString() ) );
    setIsDirty( true );
    return false;
  }
  else
  {
    setIsDirty( false );
    clearErrors();
    return true;
    }
}

void KeyChainBridge::clearErrors()
{
  setErrorCode( QKeychain::NoError );
  setErrorMessage( "" );
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
    storeMasterPassword( masterPassword() );
    if ( errorCode() == QKeychain::NoError )
    {
      showInfo( tr( "Master password has been successfully stored in your %1!" ).arg( sWalletDisplayName ) );
    }
    else
    {
      processError();
    }
  }
  else
  {
    setErrorMessage( tr( "Master password is empty: nothing to store" ) );
    showError( );
  }
}

void KeyChainBridge::showError()
{
  QString message( mErrorMessage.isEmpty() ? QString( tr( "Generic %1 plugin error" ) ).arg( name() ) : mErrorMessage );
  mQGisIface->messageBar()->pushCritical( QString( tr( "%1 plugin error" ) ).arg( name() ), message );
  debug( message );
}


// If the error is permanent or the user denied access to the wallet
// we also want to disable the wallet system to prevent annoying
// notification on each subsequent access try.
void KeyChainBridge::processError()
{
  if ( errorCode() == QKeychain::AccessDenied ||
       errorCode() == QKeychain::AccessDeniedByUser ||
       errorCode() == QKeychain::NoBackendAvailable ||
       errorCode() == QKeychain::NotImplemented )
  {
    setUseWallet( false );
    mUseWalletAction->setChecked( false );
    setErrorMessage( QString( tr("There was an error and the %1 system has been disabled, you can re-enable it at any time through the menus. %2").arg( sWalletDisplayName ).arg( errorMessage( ) ) ) );
  }
  showError();
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
  mQGisIface->removePluginMenu( tr( "&Master Password <-> %1" ).arg( sWalletDisplayName ), mAboutAction );
  mQGisIface->removePluginMenu( tr( "&Master Password <-> %1" ).arg( sWalletDisplayName ), mUseWalletAction );
  mQGisIface->removePluginMenu( tr( "&Master Password <-> %1" ).arg( sWalletDisplayName ), mLoggingEnabledAction );
  mQGisIface->removePluginMenu( tr( "&Master Password <-> %1" ).arg( sWalletDisplayName ), mSaveMasterPasswordAction );
  mQGisIface->removePluginMenu( tr( "&Master Password <-> %1" ).arg( sWalletDisplayName ), mClearMasterPasswordAction );
  // Disconnect all signals
  disconnect(this, 0, 0, 0);
  // Remove event filter
  QgsCredentialDialog* credentials = dynamic_cast<QgsCredentialDialog*>( QgsCredentials::instance() );
  credentials->removeEventFilter( this );
  delete mUseWalletAction;
  delete mLoggingEnabledAction;
  delete mSaveMasterPasswordAction;
  delete mClearMasterPasswordAction;
  delete mAboutAction;
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
