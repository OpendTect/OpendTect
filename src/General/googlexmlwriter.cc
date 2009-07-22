/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/

static const char* rcsID = "$Id";

#include "odgooglexmlwriter.h"
#include "survinfo.h"
#include "strmprov.h"
#include <iostream>

ODGoogle::XMLWriter::XMLWriter( const char* enm, const char* fnm,
				const char* snm )
    : sd_(*new StreamData)
    , elemnm_(enm)
    , survnm_(snm)
{
    open( fnm );
}


std::ostream& ODGoogle::XMLWriter::strm()
{
    return *sd_.ostrm;
}


bool ODGoogle::XMLWriter::isOK() const
{
    return sd_.usable() && strm().good();
}


#define mErrRet(s,s2) { errmsg_ = s; if ( s2 ) errmsg_ += s2; return false; }

bool ODGoogle::XMLWriter::open( const char* fnm )
{
    close();

    if ( !fnm || !*fnm )
	mErrRet("No file name provided",0)
    sd_ = StreamProvider(fnm).makeOStream();
    if ( !sd_.usable() )
	mErrRet("Cannot create file: ",fnm)

    strm() << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    "<kml xmlns=\"http://earth.google.com/kml/2.2\">\n"
	    "<Document>\n";
    BufferString treenm = survnm_.isEmpty() ? SI().name() : survnm_;
    treenm += ": "; treenm += elemnm_;
    strm() << "<name>" << treenm << "</name>" << std::endl;

    if ( !strm().good() )
	mErrRet("Error during write of XML header info",fnm)

    return true;
}


void ODGoogle::XMLWriter::close()
{
    errmsg_.setEmpty();
    if ( !sd_.usable() ) return;

    strm() << "</Document>\n</kml>" << std::endl;
    sd_.close();
}


void ODGoogle::XMLWriter::start( const ODGoogle::XMLItem& itm )
{
}



void ODGoogle::XMLWriter::finish( const ODGoogle::XMLItem& itm )
{
}
