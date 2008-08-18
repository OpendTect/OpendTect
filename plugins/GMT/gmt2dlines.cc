/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: gmt2dlines.cc,v 1.1 2008-08-18 11:23:18 cvsraman Exp $
________________________________________________________________________

-*/

#include "gmt2dlines.h"

#include "bufstringset.h"
#include "draw.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "posinfo.h"
#include "seis2dline.h"
#include "strmdata.h"
#include "strmprov.h"


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
    strm << "posting 2D Lines " << ioobj->name() << " ...  ";
    Seis2DLineSet lset( *ioobj );

    LineStyle ls;
    BufferString lsstr = find( ODGMT::sKeyLineStyle );
    ls.fromString( lsstr );
    bool postlabel = false;
    getYN( ODGMT::sKeyPostLabel, postlabel );

    BufferString comm = "@psxy ";
    BufferString rgstr; mGetRangeProjString( rgstr, "X" );
    comm += rgstr; comm += " -O -K -M";
    mGetLineStyleString( ls, lsstr );
    comm += " -W"; comm += lsstr;

    comm += " >> "; comm += fnm;
    StreamData sd = StreamProvider(comm).makeOStream();
    if ( !sd.usable() ) mErrStrmRet("Failed")

    TypeSet<float> labelrotns;
    TypeSet<Coord> labelposns;
    for ( int idx=0; idx<linenms.size(); idx++ )
    {
	PosInfo::Line2DData geom;
	const int lidx = lset.indexOf( linenms.get(idx) );
	if ( lidx<0  || !lset.getGeometry(lidx,geom) || geom.posns.size()<11)
	{
	    labelrotns += mUdf(float);
	    labelposns += Coord::udf();
	    continue;
	}

	*sd.ostrm << "> " << linenms.get(idx) << std::endl;
	Coord labelpos = geom.posns[0].coord_;
	Coord vec = geom.posns[10].coord_ - geom.posns[0].coord_;
	float angle = atan2( vec.y, vec.x );
	const float dy = fabs( cos(angle) );
	const float dx = fabs(angle) > M_PI / 2 ? sin( angle ) : -sin( angle );
	labelpos += Coord( 1000*dx, 1000*dy );
	labelposns += labelpos;
	labelrotns += angle * 180 / M_PI;

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

    int size = 12;
    get( sKey::Size, size );
    comm = "@pstext "; comm += rgstr;
    BufferString colstr; mGetColorString( ls.color_, colstr );
    comm += " -G"; comm += colstr;
    comm += " -O -K -N >> "; comm += fnm;
    sd = StreamProvider( comm ).makeOStream();
    if ( !sd.usable() )
	mErrStrmRet("Failed")
	
    for ( int idx=0; idx<linenms.size(); idx++ )
    {
	if ( mIsUdf(labelrotns[idx]) )
	    continue;

	Coord pos = labelposns[idx];
	const float rotangle = fabs(labelrotns[idx]) > 90 ? 180+labelrotns[idx]
	    						  : labelrotns[idx];
	const char* alstr = fabs(labelrotns[idx]) > 90 ? "BR" : "BL";
	*sd.ostrm << pos.x << " " << pos.y << " " << size << " " ;
	*sd.ostrm << rotangle << " " << 4;
	*sd.ostrm << " " << alstr << " " << linenms.get(idx) << std::endl;
    }

    sd.close();
    strm << "Done" << std::endl;
    return true;
}


