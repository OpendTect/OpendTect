/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		December 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "basemaptr.h"
#include "ascstream.h"
#include "ioman.h"


// BasemapTranslatorGroup
mDefSimpleTranslatorsWithSelKey(Basemap,"Basemap Data",od,Misc,100070)


// BasemapTranslator
bool BasemapTranslator::retrieve( IOPar& par, const IOObj* ioobj,
				  BufferString& bs )
{
    if ( !ioobj )
	{ bs = "Cannot find basemap object in data base"; return false; }
    mDynamicCast(BasemapTranslator*,PtrMan<BasemapTranslator> tr,
		 ioobj->createTranslator());
    if ( !tr ) { bs = "Selected object is not a BaseMap"; return false; }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	{ bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }
    bs = tr->read( par, *conn );
    return bs.isEmpty();
}


bool BasemapTranslator::store( const IOPar& par, const IOObj* ioobj,
			       BufferString& bs )
{
    if ( !ioobj )
	{ bs = "No object to store basemap in data base"; return false; }
    mDynamicCast(BasemapTranslator*,PtrMan<BasemapTranslator> tr,
		 ioobj->createTranslator());

    if ( !tr ) { bs = "Selected object is not a Basemap"; return false;}

    bs = "";
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
	{ bs = "Cannot open "; bs += ioobj->fullUserExpr(false); }
    else
	bs = tr->write( par, *conn );

    return bs.isEmpty();
}

// dgbBasemapTranslator
const char* odBasemapTranslator::read( IOPar& par, Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    if ( !astrm.isOK() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(Basemap)) )
	return "Input file is not a Basemap";
    if ( atEndOfSection(astrm) )
	astrm.next();
    if ( atEndOfSection(astrm) )
	return "Input file is empty";

    par.setName( IOM().nameOf(conn.linkedTo()) );
    par.getFrom( astrm );
    return 0;
}


const char* odBasemapTranslator::write( const IOPar& par, Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(Basemap) );
    if ( !astrm.isOK() )
	return "Cannot write to output Basemap file";

    par.putTo( astrm );
    return astrm.isOK() ? 0 : "Error during write to Basemap file";
}
