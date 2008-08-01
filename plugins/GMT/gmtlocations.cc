/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: gmtlocations.cc,v 1.1 2008-08-01 08:28:27 cvsraman Exp $
________________________________________________________________________

-*/

#include "gmtlocations.h"

#include "draw.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "pickset.h"
#include "picksettr.h"
#include "strmdata.h"
#include "strmprov.h"


static const char* sShapeKeys[] = { "a", "c", "d", "s", "t", "x", 0 };


int GMTLocations::factoryid_ = -1;

void GMTLocations::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Locations", GMTLocations::createInstance );
}

GMTPar* GMTLocations::createInstance( const IOPar& iop )
{
    return new GMTLocations( iop );
}


const char* GMTLocations::userRef() const
{
    BufferString* str = new BufferString( "Locations: " );
    const char* nm = find( sKey::Name );
    *str += nm;
    return str->buf();
}


bool GMTLocations::execute( std::ostream& strm, const char* fnm )
{
    MultiID id;
    get( sKey::ID, id );
    const IOObj* setobj = IOM().get( id );
    if ( !setobj ) mErrStrmRet("Cannot find pickset")

    Pick::Set set;
    BufferString errmsg;
    if ( !PickSetTranslator::retrieve(set,setobj,errmsg) )
	mErrStrmRet( errmsg )

    Color outcol; get( sKey::Color, outcol );
    BufferString outcolstr;
    mGetColorString( outcol, outcolstr );
    bool dofill;
    getYN( ODGMT::sKeyFill, dofill );

    float size;
    get( sKey::Size, size );
    const int shape = eEnum( ODGMT::Shape, find(ODGMT::sKeyShape) );

    BufferString comm = "@psxy -R -J -B -O -K -S";
    comm += sShapeKeys[shape]; comm += size;
    comm += " -W1p,"; comm += outcolstr;
    if ( dofill )
    {
	Color fillcol;
	get( ODGMT::sKeyFillColor, fillcol );
	BufferString fillcolstr;
	mGetColorString( fillcol, fillcolstr );
	comm += " -G";
	comm += fillcolstr;
    }

    comm += " >> "; comm += fnm;
    StreamData sd = StreamProvider(comm).makeOStream();
    if ( !sd.usable() ) mErrStrmRet("Failed to overlay locations")

    for ( int idx=0; idx<set.size(); idx++ )
	*sd.ostrm << set[idx].pos.x << " " << set[idx].pos.y << std::endl;

    sd.close();
    return true;
}



int GMTPolyline::factoryid_ = -1;

void GMTPolyline::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Polyline", GMTPolyline::createInstance );
}

GMTPar* GMTPolyline::createInstance( const IOPar& iop )
{
    return new GMTPolyline( iop );
}


const char* GMTPolyline::userRef() const
{
    BufferString* str = new BufferString( "Polyline: " );
    const char* nm = find( sKey::Name );
    *str += nm;
    return str->buf();
}


void getStyleString( LineStyle::Type typ, BufferString& str )
{
    switch( typ )
    {
	case LineStyle::Dash:
	    str = "-";
	    break;
	case LineStyle::Dot:
	    str = ".";
	    break;
	case LineStyle::DashDot:
	    str = "-.";
	    break;
	case LineStyle::DashDotDot:
	    str = "-..";
	default:
	    break;
    }
}


bool GMTPolyline::execute( std::ostream& strm, const char* fnm )
{
    MultiID id;
    get( sKey::ID, id );
    const IOObj* setobj = IOM().get( id );
    if ( !setobj ) mErrStrmRet("Cannot find pickset")

    Pick::Set set;
    BufferString errmsg;
    if ( !PickSetTranslator::retrieve(set,setobj,errmsg) )
	mErrStrmRet( errmsg )

    LineStyle ls;
    const char* lsstr = find( ODGMT::sKeyLineStyle );
    ls.fromString( lsstr );
    bool dofill;
    getYN( ODGMT::sKeyFill, dofill );

    bool drawline = true;
    if ( ls.type_ == LineStyle::None && dofill )
	drawline = false;

    BufferString comm = "@psxy -R -J -B -O -K -L";
    if ( drawline )
    {
	BufferString lsstr;
	mGetLineStyleString( ls, lsstr );
	comm += " -W"; comm += lsstr;
    }

    if ( dofill )
    {
	Color fillcol;
	get( ODGMT::sKeyFillColor, fillcol );
	BufferString fillcolstr;
	mGetColorString( fillcol, fillcolstr );
	comm += " -G";
	comm += fillcolstr;
    }

    comm += " >> "; comm += fnm;
    StreamData sd = StreamProvider(comm).makeOStream();
    if ( !sd.usable() ) mErrStrmRet("Failed to overlay polylines")

    for ( int idx=0; idx<set.size(); idx++ )
	*sd.ostrm << set[idx].pos.x << " " << set[idx].pos.y << std::endl;

    sd.close();

    return true;
}

