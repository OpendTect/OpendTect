/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: uidesktopservices.cc,v 1.6 2007-05-25 03:32:34 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidesktopservices.h"

#include "bufstring.h"
#include "filegen.h"
#include "uimsg.h"

#include <QDesktopServices>
#include <QUrl>


bool uiDesktopServices::openUrl( const char* url )
{
    BufferString myurl = url;
    if ( !strncasecmp(url,"c:",2) )
    {
	myurl = "file:///";
	myurl += url;
    }
    else if ( !strncasecmp(url,"www.",4) )
    {
	myurl = "http://";
	myurl += url;
    }

    QUrl qurl( myurl.buf(), QUrl::TolerantMode );
    if ( qurl.isRelative() )
	qurl.setScheme( "file" );
    return QDesktopServices::openUrl( qurl );
}
