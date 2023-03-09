/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidesktopservices.h"

#include "bufstring.h"
#include "debug.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
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

#ifdef __lux__
static bool isLinuxSession( const char* sessnm )
{
    OS::MachineCommand mc( "pidof" );
    mc.addFlag( "s", OS::OldStyle ).addArg( sessnm );

    return mc.execute();
}
#endif

#define mQUrlCStr(qurl) qurl.toString().toUtf8().data()

static bool openLocalFragmentedUrl( const QUrl& qurl )
{
    OS::MachineCommand machcomm;

#ifdef __win__

    BufferString browser, errmsg;
    if ( !getDefaultBrowser(browser,errmsg) )
    {
	if ( DBG::isOn(DBG_IO) && !errmsg.isEmpty() )
	    DBG::message( browser.buf() );
    }

    QString brcmd( browser.buf() );
    const int index = brcmd.lastIndexOf( "%1" );
    if ( index < 0 )
    {
	machcomm.setProgram( browser );
	machcomm.addArg( mQUrlCStr(qurl) );
    }
    else
    {
	brcmd.replace( index, 2, "" );
	const BufferString workstr( brcmd );
	machcomm.setProgram( workstr.buf() );
	const BufferString urlstr( qurl.toString() );
	machcomm.addArg( urlstr.buf() );
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
		    StringView(urlstr).size(), kCFStringEncodingASCII, NULL );
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


bool uiDesktopServices::showInFolder( const char* file )
{
    const BufferStringSet mypaths;
    if ( __iswin__ )
    {
	const BufferString explorercmd =
			File::findExecutable( "explorer.exe", mypaths, true );
	if ( !explorercmd.isEmpty() )
	{
	    OS::MachineCommand mc( explorercmd, true );
	    BufferString filearg( "/select,", FilePath::getShortPath(file) );
	    mc.addArg( filearg );
	    return mc.execute( OS::RunInBG );
	}
    }
    else if ( __islinux__ )
    {
	BufferString filemgrcmd =
			File::findExecutable( "dolphin", mypaths, true );
	if ( filemgrcmd.isEmpty() )
	    filemgrcmd = File::findExecutable( "nautilus", mypaths, true );
	//TODO: Add other popular Linux file managers.

	if ( !filemgrcmd.isEmpty() )
	{
	    OS::MachineCommand mc( filemgrcmd, true );
	    mc.addKeyedArg( "select", file );
	    return mc.execute( OS::RunInBG );
	}
    }

    return false;
}
