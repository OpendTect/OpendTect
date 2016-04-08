/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "webstreamsource.h"
#include <QUrl>


void WebStreamSource::initClass()
{
    StreamProvider::addStreamSource( new WebStreamSource );
}


bool WebStreamSource::willHandle( const char* fnm )
{
    const FixedString url( fnm );
    return url.startsWith( "http://" ) || url.startsWith( "https://" )
	|| url.startsWith( "ftp://" );
}


bool WebStreamSource::canHandle( const char* fnm ) const
{
    return willHandle( fnm );
}


WebStreamSource::WebStreamSource()
{
}


bool WebStreamSource::fill( StreamData& sd, StreamSource::Type typ ) const
{
    return false;
}
