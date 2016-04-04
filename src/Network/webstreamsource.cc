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


WebStreamSource::WebStreamSource()
{
    // init netcomm
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


#include "filepath.h"

bool WebStreamSource::fill( StreamData& sd, StreamSource::Type typ ) const
{
    const FixedString url( sd.fileName() );
    const bool forread = typ == Read;
    FilePath fp( url );
    const BufferString filename = fp.fileName();
    fp.setPath( "/tmp/web/" );

    if ( isHttp(url) )
    {
	fp.setFileName( "http" ).add( filename );
	const BufferString fullpath = fp.fullPath();
	if ( forread )
	    sd = StreamProvider(fullpath).makeIStream();
	else
	    sd = StreamProvider(fullpath).makeOStream();
    }
    else if ( isFtp(url) )
    {
	fp.setFileName( "ftp" ).add( filename );
	const BufferString fullpath = fp.fullPath();
	if ( forread )
	    sd = StreamProvider(fullpath).makeIStream();
	else
	    sd = StreamProvider(fullpath).makeOStream();
    }

    return sd.usable();
}
