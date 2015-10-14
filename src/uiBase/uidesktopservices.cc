/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uidesktopservices.h"

#include "bufstring.h"
#include "debug.h"
#include "envvars.h"
#include "oscommand.h"
#include "uimsg.h"

#ifdef __win__
# include "winutils.h"
#endif

#include <QDesktopServices>
#include <QUrl>

mUseQtnamespace

static const char* sKeySysLibPath = "OD_SYSTEM_LIBRARY_PATH";
static const char* sKeyLDLibPath =
#ifdef __lux__
"LD_LIBRARY_PATH";
#else
# ifdef __mac__
"DYLD_LIBRARY_PATH";
# else
0;
# endif
#endif

static bool openLocalFragmentedUrl( const QUrl& qurl )
{
    BufferString odcmd;

#ifdef __win__

    BufferString browser, errmsg;
    if ( !getDefaultBrowser(browser,errmsg) )
    {
	if ( DBG::isOn(DBG_IO) && !errmsg.isEmpty() )
	    DBG::message( browser.buf() );
    }

    QString command = browser.buf();

    const int index = command.lastIndexOf( "%1" );
    if ( index != -1 )
	command.replace( index, 2, qurl.toString() );

    odcmd.set( command );

#elif defined( __lux__ )
    int res = system( "pidof -s gnome-session" );
    if ( res==0 )
	odcmd.set( "gnome-open" );

    res = system( "pidof -s ksmserver" );
    if ( res==0 )
	odcmd.set( "kfmclient exec" );

    if ( odcmd.isEmpty() )
	return false;

    odcmd.addSpace().add( qurl.toString() );

#elif defined( __mac__ )
    return false;
#endif

    if ( DBG::isOn(DBG_IO) )
	DBG::message( BufferString("Local command: ",odcmd) );
    const bool execres = OS::ExecCommand( odcmd.buf() );
    if ( !execres )
    {
	if ( DBG::isOn(DBG_IO) )
	    DBG::message( "Could not launch browser" ); // TODO: GetLastError
	return false;
    }

    return true;
}


bool uiDesktopServices::openUrl( const char* url )
{
    const QUrl qurl = QUrl::fromUserInput( url );

    if ( DBG::isOn(DBG_IO) )
    {
	BufferString msg( "Open url: " );
	msg.add( qurl.toString() );
	DBG::message( msg );
    }

    const BufferString syslibpath( GetEnvVar(sKeySysLibPath) );
    const BufferString odenvlibpath( sKeyLDLibPath ? GetEnvVar(sKeyLDLibPath)
						   : 0 );
    const bool needlibpathtrick = !odenvlibpath.isEmpty();
    if ( needlibpathtrick )
	SetEnvVar( sKeyLDLibPath, syslibpath.buf() );

    bool res = false;
    if ( qurl.hasFragment() && qurl.isLocalFile() )
	res = openLocalFragmentedUrl( qurl );

    if ( !res )
	res = QDesktopServices::openUrl( qurl );

    if ( needlibpathtrick )
	SetEnvVar( sKeyLDLibPath, odenvlibpath.buf() );

    return res;
}

