/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: uidesktopservices.cc,v 1.8 2008-06-27 12:30:46 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidesktopservices.h"

#include "bufstring.h"
#include "debugmasks.h"
#include "filegen.h"
#include "uimsg.h"

#include <QDesktopServices>
#include <QUrl>


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
    return QDesktopServices::openUrl( qurl );
}
