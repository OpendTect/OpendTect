/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: uidesktopservices.cc,v 1.5 2007-05-23 12:20:07 cvsdgb Exp $
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
    return QDesktopServices::openUrl( qurl );
}
