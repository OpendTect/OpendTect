/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "webstreamsource.h"


inline static bool isHttp( const OD::String& url )
{
    return url.startsWith( "http://" ) || url.startsWith( "https://" );
}
inline static bool isFtp( const OD::String& url )
{
    return url.startsWith( "ftp://" );
}


void WebStreamSource::initClass()
{
    StreamProvider::addStreamSource( new WebStreamSource );
}


bool WebStreamSource::willHandle( const char* fnm )
{
    const FixedString url( fnm );
    return isHttp( url ) || isFtp( url );
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
