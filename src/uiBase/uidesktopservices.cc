/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: uidesktopservices.cc,v 1.3 2007-05-21 04:27:45 cvsnanne Exp $
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
    if ( !File_exists(url) )
    {
	BufferString msg( "Cannot open file:\n" ); msg += url;
	uiMSG().error( msg );
	return false;
    }

    QUrl qurl( url, QUrl::TolerantMode );
    return QDesktopServices::openUrl( qurl );
}
