/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uidesktopservices.h"

#include "bufstring.h"
#include "debugmasks.h"
#include "envvars.h"
#include "uimsg.h"

#include <QDesktopServices>
#include <QUrl>


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

bool uiDesktopServices::openUrl( const char* url )
{
    BufferString myurl = url;
    if ( myurl[1] == ':' )
    {
	myurl = "file:///";
	myurl += url;
    }
    else if ( !strncasecmp(url,"www.",4) )
    {
	myurl = "http://";
	myurl += url;
    }


    QUrl qurl( myurl.buf() );
    if ( qurl.isRelative() )
	qurl.setScheme( "file" );

    if ( DBG::isOn(DBG_IO) )
    {
	BufferString msg( "Open url: " );
	msg += qurl.toString().toAscii().data();
	DBG::message( msg );
    }

    const BufferString syslibpath( GetEnvVar(sKeySysLibPath) );
    const BufferString odenvlibpath( sKeyLDLibPath ? GetEnvVar(sKeyLDLibPath)
	    					   : 0 );
    if ( odenvlibpath.isEmpty() )
	return QDesktopServices::openUrl( qurl );

    SetEnvVar( sKeyLDLibPath, syslibpath.buf() );
    const bool res = QDesktopServices::openUrl( qurl );
    SetEnvVar( sKeyLDLibPath, odenvlibpath.buf() );
    return res;
}
