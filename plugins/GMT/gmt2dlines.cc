/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2008
________________________________________________________________________

-*/

#include "gmt2dlines.h"

#include "bufstringset.h"
#include "draw.h"
#include "ioobj.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "strmdata.h"
#include "survgeom2d.h"
#include "od_ostream.h"
#include "survinfo.h"


#include <math.h>


int GMT2DLines::factoryid_ = -1;

void GMT2DLines::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "2D Lines", GMT2DLines::createInstance );
}

GMTPar* GMT2DLines::createInstance( const IOPar& iop )
{
    return new GMT2DLines( iop );
}


const char* GMT2DLines::userRef() const
{
    BufferString* str = new BufferString( "2D Lines: " );
    const char* nm = find( sKey::Name() );
    *str += nm;
    return str->buf();
}


bool GMT2DLines::fillLegendPar( IOPar& par ) const
{
    FixedString str = find( sKey::Name() );
    par.set( sKey::Name(), str );
    par.set( ODGMT::sKeyShape(), "Line" );
    par.set( sKey::Size(), 1 );
    str = find( ODGMT::sKeyLineStyle() );
    par.set( ODGMT::sKeyLineStyle(), str );
    return true;
}


bool GMT2DLines::execute( od_ostream& strm, const char* fnm )
{
    FixedString namestr = find( sKey::Name() );
    strm << "Posting 2D Lines " << namestr << " ...  ";

    GeomIDSet geomids;
    get( sKey::GeomID(), geomids );

    BufferStringSet linenms;
    OD::LineStyle ls;
    BufferString lsstr = find( ODGMT::sKeyLineStyle() );
    ls.fromString( lsstr );
    bool postlabel = false;
    getYN( ODGMT::sKeyPostLabel(), postlabel );

    BufferString comm = "@psxy ";
    BufferString rgstr; mGetRangeProjString( rgstr, "X" );
    comm += rgstr;
    comm += " -O -K -N";
    mGetLineStyleString( ls, lsstr );
    comm += " -W"; comm += lsstr;

    comm += " 1>> "; comm += fileName( fnm );
    od_ostream procstrm = makeOStream( comm, strm );
    if ( !procstrm.isOK() ) mErrStrmRet("Failed")

    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	const auto& geom2d = Survey::Geometry::get2D( geomids[idx] );
	if ( geom2d.isEmpty() )
	    continue;

	const PosInfo::Line2DData& geom = geom2d.data();
	const TypeSet<PosInfo::Line2DPos>& posns = geom.positions();

	procstrm << "> " << geom2d.name() << "\n";

	for ( int tdx=0; tdx<posns.size(); tdx++ )
	{
	    Coord pos = posns[tdx].coord_;
	    procstrm << pos.x_ << " " << pos.y_ << "\n";
	}
    }

    if ( !postlabel )
    {
	strm << "Done" << od_endl;
	return true;
    }

    int sz = 10;
    get( ODGMT::sKeyFontSize(), sz );
    comm = "@pstext "; comm += rgstr;
    BufferString colstr; mGetColorString( ls.color_, colstr );
    comm += " -F+f12p,Sans,"; comm += colstr;
    comm += " -O -K -N 1>> "; comm += fileName( fnm );
    procstrm = makeOStream( comm, strm );
    if ( !procstrm.isOK() )
	mErrStrmRet("Failed")

    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	const auto& geom2d = Survey::Geometry::get2D( geomids[idx] );
	if ( geom2d.isEmpty() )
	    continue;

	const PosInfo::Line2DData& geom = geom2d.data();
	const TypeSet<PosInfo::Line2DPos>& posns = geom.positions();
	const int nrtrcs = posns.size();
	Coord pos = posns[0].coord_;
	Coord cvec = posns[1].coord_ - posns[0].coord_;
	float angle = mCast(float, Math::Atan2( cvec.y_, cvec.x_ ) );
	float dy = sin( angle );
	float dx = cos( angle );
	angle = Math::toDegrees( angle );

	float rotangle = fabs(angle) > 90 ? 180+angle : angle;
	float perpangle = angle > 0 ? angle - 90 : angle + 90;
	BufferString al = fabs(angle) > 90 ? "ML " : "MR ";
	bool poststart = true;
	getYN( ODGMT::sKeyPostStart(), poststart );
	const float distfactor = xrg.width() / 100;
	if ( poststart )
	{
	    pos -= Coord( distfactor*dx, distfactor*dy );
	    procstrm << pos.x_ << " " << pos.y_ << " " << sz << " " ;
	    procstrm << rotangle << " " << 4;
	    procstrm << " " << al.buf() << geom2d.name() << "\n";
	}

	bool poststop = false;
	getYN( ODGMT::sKeyPostStop(), poststop );
	if ( poststop )
	{
	    pos = posns[nrtrcs-1].coord_;
	    cvec = posns[nrtrcs-2].coord_ - pos;
	    angle = mCast(float, Math::Atan2( cvec.y_, cvec.x_ ) );
	    dy = sin( angle );
	    dx = cos( angle );
	    angle = Math::toDegrees( angle );
	    rotangle = fabs(angle) > 90 ? 180+angle : angle;
	    pos -= Coord( distfactor*dx, distfactor*dy );
	    al = fabs(angle) > 90 ? "ML " : "MR ";
	    procstrm << pos.x_ << " " << pos.y_ << " " << sz << " " ;
	    procstrm << rotangle << " " << 4;
	    procstrm << " " << al.buf() << geom2d.name() << "\n";
	}

	bool postnrs = true;
	getYN( ODGMT::sKeyPostTraceNrs(), postnrs );
	if ( postnrs )
	{
	    int labelintv = 100;
	    get( ODGMT::sKeyLabelIntv(), labelintv );
	    for ( int tdx=0; tdx<posns.size(); tdx+=labelintv )
	    {
		BufferString lbl = "- "; lbl += posns[tdx].nr_;
		Coord posc = posns[tdx].coord_;
		if ( tdx > 4 && tdx < posns.size()-5 )
		{
		    cvec = posns[tdx+5].coord_ - posns[tdx-5].coord_;
		    angle = mCast( float,
			   Math::toDegrees( Math::Atan2( cvec.y_, cvec.x_ ) ) );
		    perpangle = angle > 0 ? angle - 90 : angle + 90;
		}

		procstrm << posc.x_ << " " << posc.y_ << " " << sz << " " ;
		procstrm << perpangle << " " << 4;
		procstrm << " " << "ML " << lbl.buf() << "\n";
	    }
	}
    }

    strm << "Done" << od_endl;
    return true;
}


int GMTRandLines::factoryid_ = -1;

void GMTRandLines::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Random Lines",
				  GMTRandLines::createInstance );
}

GMTPar* GMTRandLines::createInstance( const IOPar& iop )
{
    return new GMTRandLines( iop );
}


const char* GMTRandLines::userRef() const
{
    BufferString* str = new BufferString( "Random Lines: " );
    const char* nm = find( sKey::Name() );
    *str += nm;
    return str->buf();
}


bool GMTRandLines::fillLegendPar( IOPar& par ) const
{
    FixedString str = find( sKey::Name() );
    par.set( sKey::Name(), str );
    par.set( ODGMT::sKeyShape(), "Line" );
    par.set( sKey::Size(), 1 );
    str = find( ODGMT::sKeyLineStyle() );
    par.set( ODGMT::sKeyLineStyle(), str );
    return true;
}


bool GMTRandLines::execute( od_ostream& strm, const char* fnm )
{
    DBKey id;
    get( sKey::ID(), id );
    const IOObj* ioobj = id.getIOObj();
    if ( !ioobj )
	mErrStrmRet("Cannot find line")

    BufferStringSet linenms;
    get( ODGMT::sKeyLineNames(), linenms );
    strm << "Posting Random Lines " << ioobj->name() << " ...  ";

    Geometry::RandomLineSet inprls; BufferString msg;
    if ( !RandomLineSetTranslator::retrieve(inprls,ioobj,msg) )
	mErrStrmRet("Cannot read random lines")

    OD::LineStyle ls;
    BufferString lsstr = find( ODGMT::sKeyLineStyle() );
    ls.fromString( lsstr );
    bool postlabel = false;
    getYN( ODGMT::sKeyPostLabel(), postlabel );

    BufferString comm = "@psxy ";
    BufferString rgstr; mGetRangeProjString( rgstr, "X" );
    comm += rgstr;
    comm += " -O -K -N";
    mGetLineStyleString( ls, lsstr );
    comm += " -W"; comm += lsstr;

    comm += " 1>> "; comm += fileName( fnm );
    od_ostream procstrm = makeOStream( comm, strm );
    if ( !procstrm.isOK() ) mErrStrmRet("Failed")

    for ( int idx=0; idx<inprls.size(); idx++ )
    {
	const Geometry::RandomLine* rdl = inprls.lines()[idx];
	if ( !rdl || linenms.indexOf(rdl->name()) < 0 )
	    continue;

	procstrm << "> " << rdl->name() << "\n";
	for ( int tdx=0; tdx<rdl->nrNodes(); tdx++ )
	{
	    Coord posc = SI().transform( rdl->nodePosition(tdx) );
	    procstrm << posc.x_ << " " << posc.y_ << "\n";
	}
    }

    if ( !postlabel )
    {
	strm << "Done" << od_endl;
	return true;
    }

    int sz = 10;
    get( ODGMT::sKeyFontSize(), sz );
    comm = "@pstext "; comm += rgstr;
    BufferString colstr; mGetColorString( ls.color_, colstr );
    comm += " -F+f12p,Sans,"; comm += colstr;
    comm += " -O -K -N 1>> "; comm += fileName( fnm );
    procstrm = makeOStream( comm, strm );
    if ( !procstrm.isOK() )
	mErrStrmRet("Failed")

    for ( int idx=0; idx<inprls.size(); idx++ )
    {
	const Geometry::RandomLine* rdl = inprls.lines()[idx];
	if ( !rdl || linenms.indexOf(rdl->name()) < 0 || rdl->nrNodes() < 2 )
	    continue;

	Coord posc = SI().transform( rdl->nodePosition(0) );
	Coord cvec = SI().transform( rdl->nodePosition(1) ) - posc;
	float angle = mCast(float, Math::Atan2( cvec.y_, cvec.x_ ) );
	const float dy = cos( angle );
	const float dx = sin( angle );
	angle = Math::toDegrees( angle );

	float rotangle = fabs(angle) > 90 ? 180+angle : angle;
	BufferString al = fabs(angle) > 90 ? "BR " : "BL ";
	const float distfactor = xrg.width() / 100;
	posc += Coord( -distfactor*dx, distfactor*dy );
	procstrm << posc.x_ << " " << posc.y_ << " " << sz << " " ;
	procstrm << rotangle << " " << 4;
	procstrm << " " << al.buf() << rdl->name() << "\n";
    }

    strm << "Done" << od_endl;
    return true;
}
