/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: uidesktopservices.cc,v 1.2 2007-05-14 06:56:08 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidesktopservices.h"

#include "bufstring.h"
#include "filegen.h"
#include "uimsg.h"

#include <QDesktopServices>
#include <QUrl>


uiDesktopServices::uiDesktopServices()
    : qdesktopservices_(new QDesktopServices)
{}


void uiDesktopServices::openUrl( const char* url )
{
    if ( !File_exists(url) )
    {
	BufferString msg( "Cannot open file:\n" ); msg += url;
	uiMSG().error( msg );
	return;
    }

    QUrl qurl( url, QUrl::TolerantMode );
    qdesktopservices_->openUrl( qurl );
}
