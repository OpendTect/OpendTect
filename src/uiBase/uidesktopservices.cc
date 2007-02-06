/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: uidesktopservices.cc,v 1.1 2007-02-06 21:26:50 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidesktopservices.h"

#include <QDesktopServices>
#include <QUrl>


uiDesktopServices::uiDesktopServices()
    : qdesktopservices_(new QDesktopServices)
{}


void uiDesktopServices::openUrl( const char* url )
{
    QUrl qurl( url, QUrl::TolerantMode );
    qdesktopservices_->openUrl( qurl );
}
