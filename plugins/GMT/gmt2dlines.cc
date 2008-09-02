/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: gmt2dlines.cc,v 1.4 2008-09-02 11:15:50 cvsraman Exp $
________________________________________________________________________

-*/

#include "gmt2dlines.h"

#include "bufstringset.h"
#include "draw.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "posinfo.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "seis2dline.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"


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
    const char* nm = find( sKey::Name );
    *str += nm;
    return str->buf();
}


bool GMT2DLines::fillLegendPar( IOPar& par ) const
{
    BufferString str = find( sKey::Name );
    par.set( sKey::Name, str );
    par.set( ODGMT::sKeyShape, "Line" );
    par.set( sKey::Size, 1 );
    str = find( ODGMT::sKeyLineStyle );
    par.set( ODGMT::sKeyLineStyle, str );
    return true;
}


bool GMT2DLines::execute( std::ostream& strm, const char* fnm )
{
    MultiID id;
    get( sKey::ID, id );
    const IOObj* ioobj = IOM().get( id );
    if ( !ioobj ) mErrStrmRet("Cannot find lineset")

    BufferStringSet linenms;
    get( ODGMT::sKeyLineNames, linenms );
    strm << "Posting 2D Lines " << ioobj->name() << " ...  ";
    Seis2DLineSet lset( *ioobj );

    LineStyle ls;
    BufferString lsstr = find( ODGMT::sKeyLineStyle );
    ls.fromString( lsstr );
    bool postlabel = false;
    getYN( ODGMT::sKeyPostLabel, postlabel );

    BufferString comm = "@psxy ";
    BufferString rgstr; mGetRangeProjString( rgstr, "X" );
    comm += rgstr;
    comm += " -O -K -N -M";
    mGetLineStyleString( ls, lsstr );
    comm += " -W"; comm += lsstr;

    comm += " >> "; comm += fnm;
    StreamData sd = StreamProvider(comm).makeOStream();
    if ( !sd.usable() ) mErrStrmRet("Failed")

    for ( int idx=0; idx<linenms.size(); idx++ )
    {
	PosInfo::Line2DData geom;
	const int lidx = lset.indexOf( linenms.get(idx) );
	if ( lidx<0  || !lset.getGeometry(lidx,geom) || geom.posns.size()<11)
	    continue;

	*sd.ostrm << "> " << linenms.get(idx) << std::endl;

	for ( int tdx=0; tdx<geom.posns.size(); tdx++ )
	{
	    Coord pos = geom.posns[tdx].coord_;
	    *sd.ostrm << pos.x << " " << pos.y << std::endl;
	}
    }

    sd.close();

    if ( !postlabel )
    {
	strm << "Done" << std::endl;
	return true;
    }

    int size = 10;
    get( ODGMT::sKeyFontSize, size );
    comm = "@pstext "; comm += rgstr;
    BufferString colstr; mGetColorString( ls.color_, colstr );
    comm += " -G"; comm += colstr;
    comm += " -O -K -N >> "; comm += fnm;
    sd = StreamProvider( comm ).makeOStream();
    if ( !sd.usable() )
	mErrStrmRet("Failed")
	    
    for ( int idx=0; idx<linenms.size(); idx++ )
    {
	PosInfo::Line2DData geom;
	const int lidx = lset.indexOf( linenms.get(idx) );
	if ( lidx<0  || !lset.getGeometry(lidx,geom) || geom.posns.size()<11)
	    continue;

	const int nrtrcs = geom.posns.size();
	Coord pos = geom.posns[0].coord_;
	Coord vec = geom.posns[10].coord_ - geom.posns[0].coord_;
	float angle = atan2( vec.y, vec.x );
	const float dy = sin( angle );
	const float dx = cos( angle );
	angle *= 180 / M_PI;

	float rotangle = fabs(angle) > 90 ? 180+angle : angle;
	float perpangle = angle > 0 ? angle - 90 : angle + 90;
	BufferString al = fabs(angle) > 90 ? "ML " : "MR ";
	bool poststart = true;
	getYN( ODGMT::sKeyPostStart, poststart );
	const float distfactor = xrg.width() / 100;
	if ( poststart )
	{
	    pos -= Coord( distfactor*dx, distfactor*dy );
	    *sd.ostrm << pos.x << " " << pos.y << " " << size << " " ;
	    *sd.ostrm << rotangle << " " << 4;
	    *sd.ostrm << " " << al.buf() << linenms.get(idx) << std::endl;
	}

	bool poststop = true;
	getYN( ODGMT::sKeyPostStop, poststop );
	if ( poststop )
	{
	    pos = geom.posns[nrtrcs-1].coord_;
	    pos += Coord( distfactor*dx, distfactor*dy );
	    al = fabs(angle) > 90 ? "MR " : "ML ";
	    *sd.ostrm << pos.x << " " << pos.y << " " << size << " " ;
	    *sd.ostrm << rotangle << " " << 4;
	    *sd.ostrm << " " << al.buf() << linenms.get(idx) << std::endl;
	}

	bool postnrs = true;
	getYN( ODGMT::sKeyPostTraceNrs, postnrs );
	if ( postnrs )
	{
	    int labelintv = 100;
	    get( ODGMT::sKeyLabelIntv, labelintv );
	    for ( int tdx=0; tdx<geom.posns.size(); tdx+=labelintv )
	    {
		BufferString lbl = "- "; lbl += geom.posns[tdx].nr_;
		Coord pos = geom.posns[tdx].coord_;
		if ( tdx > 4 && tdx < geom.posns.size()-5 )
		{
		    vec = geom.posns[tdx+5].coord_
				- geom.posns[tdx-5].coord_;
		    angle = atan2( vec.y, vec.x );
		    angle *= 180 / M_PI;
		    perpangle = angle > 0 ? angle - 90 : angle + 90;
		}

		*sd.ostrm << pos.x << " " << pos.y << " " << size << " " ;
		*sd.ostrm << perpangle << " " << 4;
		*sd.ostrm << " " << "ML " << lbl.buf() << std::endl;
	    }
	}
    }

    sd.close();
    strm << "Done" << std::endl;
    return true;
}


int GMTRandLines::factoryid_ = -1;

void GMTRandLines::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Random Lines", GMTRandLines::createInstance );
}

GMTPar* GMTRandLines::createInstance( const IOPar& iop )
{
    return new GMTRandLines( iop );
}


const char* GMTRandLines::userRef() const
{
    BufferString* str = new BufferString( "Random Lines: " );
    const char* nm = find( sKey::Name );
    *str += nm;
    return str->buf();
}


bool GMTRandLines::fillLegendPar( IOPar& par ) const
{
    BufferString str = find( sKey::Name );
    par.set( sKey::Name, str );
    par.set( ODGMT::sKeyShape, "Line" );
    par.set( sKey::Size, 1 );
    str = find( ODGMT::sKeyLineStyle );
    par.set( ODGMT::sKeyLineStyle, str );
    return true;
}


bool GMTRandLines::execute( std::ostream& strm, const char* fnm )
{
    MultiID id;
    get( sKey::ID, id );
    const IOObj* ioobj = IOM().get( id );
    if ( !ioobj ) mErrStrmRet("Cannot find lineset")

    BufferStringSet linenms;
    get( ODGMT::sKeyLineNames, linenms );
    strm << "Posting Random Lines " << ioobj->name() << " ...  ";

    Geometry::RandomLineSet inprls; BufferString msg;
    if ( !RandomLineSetTranslator::retrieve(inprls,ioobj,msg) )
	mErrStrmRet("Cannot read random lines")
	
    LineStyle ls;
    BufferString lsstr = find( ODGMT::sKeyLineStyle );
    ls.fromString( lsstr );
    bool postlabel = false;
    getYN( ODGMT::sKeyPostLabel, postlabel );

    BufferString comm = "@psxy ";
    BufferString rgstr; mGetRangeProjString( rgstr, "X" );
    comm += rgstr;
    comm += " -O -K -M -N";
    mGetLineStyleString( ls, lsstr );
    comm += " -W"; comm += lsstr;

    comm += " >> "; comm += fnm;
    StreamData sd = StreamProvider(comm).makeOStream();
    if ( !sd.usable() ) mErrStrmRet("Failed")

    for ( int idx=0; idx<inprls.size(); idx++ )
    {
	const Geometry::RandomLine* rdl = inprls.lines()[idx];
	if ( !rdl || linenms.indexOf(rdl->name()) < 0 )
	    continue;

	*sd.ostrm << "> " << rdl->name() << std::endl;
	for ( int tdx=0; tdx<rdl->nrNodes(); tdx++ )
	{
	    Coord pos = SI().transform( rdl->nodePosition(tdx) );
	    *sd.ostrm << pos.x << " " << pos.y << std::endl;
	}
    }

    sd.close();
    if ( !postlabel )
    {
	strm << "Done" << std::endl;
	return true;
    }

    int size = 10;
    get( ODGMT::sKeyFontSize, size );
    comm = "@pstext "; comm += rgstr;
    BufferString colstr; mGetColorString( ls.color_, colstr );
    comm += " -G"; comm += colstr;
    comm += " -O -K -N >> "; comm += fnm;
    sd = StreamProvider( comm ).makeOStream();
    if ( !sd.usable() )
	mErrStrmRet("Failed")
	    
    for ( int idx=0; idx<inprls.size(); idx++ )
    {
	const Geometry::RandomLine* rdl = inprls.lines()[idx];
	if ( !rdl || linenms.indexOf(rdl->name()) < 0 || rdl->nrNodes() < 2 )
	    continue;

	Coord pos = SI().transform( rdl->nodePosition(0) );
	Coord vec = SI().transform( rdl->nodePosition(1) ) - pos;
	float angle = atan2( vec.y, vec.x );
	const float dy = cos( angle );
	const float dx = sin( angle );
	angle *= 180 / M_PI;

	float rotangle = fabs(angle) > 90 ? 180+angle : angle;
	BufferString al = fabs(angle) > 90 ? "BR " : "BL ";
	const float distfactor = xrg.width() / 100;
	pos += Coord( -distfactor*dx, distfactor*dy );
	*sd.ostrm << pos.x << " " << pos.y << " " << size << " " ;
	*sd.ostrm << rotangle << " " << 4;
	*sd.ostrm << " " << al.buf() << rdl->name() << std::endl;
    }

    sd.close();
    strm << "Done" << std::endl;
    return true;
}


