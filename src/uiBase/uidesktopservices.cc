/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2006
________________________________________________________________________

-*/

#include "uidesktopservices.h"

#include "bufstring.h"
#include "debug.h"
#include "envvars.h"
#include "oscommand.h"
#include "uimsg.h"

#ifdef __win__
# include "winutils.h"
#endif
#ifdef __mac__
    /* TODO MACENABLE
# include <CoreFoundation/CFBundle.h>
# include <ApplicationServices/ApplicationServices.h>
    */
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

static bool isLinuxSession( const char* sessnm )
{
#ifdef __lux__
    OS::MachineCommand mc( "pidof" );
    mc.addFlag( "s", OS::OldStyle ).addArg( sessnm );

    return mc.execute();
#else
    return false;
#endif
}

#define mQUrlCStr(qurl) qurl.toString().toUtf8().data()

static bool openLocalFragmentedUrl( const QUrl& qurl )
{
    OS::MachineCommand machcomm;

#ifdef __win__

    BufferString browser, errmsg;
    if ( !WinUtils::getDefaultBrowser(browser,errmsg) )
	{ ErrMsg( "No system browser found" ); return false; }

    QString brcmd( browser.buf() );
    const int index = brcmd.lastIndexOf( "%1" );
    if ( index < 0 )
    {
	machcomm.setProgram( browser );
	machcomm.addArg( mQUrlCStr(qurl) );
    }
    else
    {
	brcmd.replace( index, 2, qurl.toString() );
	BufferString workstr( brcmd );
	char* ptr = workstr.getCStr();
	mSkipBlanks( ptr ); mSkipNonBlanks( ptr );
	machcomm.setProgram( ptr );
	ptr++; mSkipBlanks( ptr );
	machcomm.addArg( ptr );
    }

#elif defined( __lux__ )

    if ( isLinuxSession("gnome-session") )
	machcomm.setProgram( "gnome-open" );
    else if ( isLinuxSession("ksmserver") )
    {
	machcomm.setProgram( "kfmclient" );
	machcomm.addArg( "exec" );
    }
    else
	return false;

    if ( machcomm.isBad() )
	{ ErrMsg( "No system browser found" ); return false; }

    machcomm.addArg( mQUrlCStr(qurl) );

#elif defined( __mac__ )

    /* TODO MACENABLE Should work:
    const char* urlstr = mQUrlCStr( qurl );
    CFURLRef urlref = CFURLCreateWithBytes ( NULL, (UInt8*)urlstr,
		    FixedString(urlstr).size(), kCFStringEncodingASCII, NULL );
    LSOpenCFURLRef( urlref, 0 );
    CFRelease( urlref );
    return true;
    */
    return false;

#endif

    return machcomm.execute( OS::RunInBG );
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
