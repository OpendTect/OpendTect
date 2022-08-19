/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gmt2dlines.h"

#include "bufstringset.h"
#include "draw.h"
#include "initgmtplugin.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "survgeom2d.h"
#include "od_ostream.h"
#include "survinfo.h"


#include <math.h>


int GMT2DLines::factoryid_ = -1;

void GMT2DLines::postText( const Coord& pos, int fontsz, float angle,
		      const char* justify, const char* txt, bool modern,
		      od_ostream& sd, int gmt4fontno )
{
    sd << pos.x << " " << pos.y ;
    if ( !modern )
	sd << " " << fontsz;
    sd << " " << angle;
    if ( !modern )
	sd << " " << gmt4fontno;
    sd << " " << justify << " " << txt << "\n";
}


void GMT2DLines::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "2D Lines", GMT2DLines::createInstance );
}

GMTPar* GMT2DLines::createInstance( const IOPar& iop, const char* workdir )
{
    return new GMT2DLines( iop, workdir );
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
    StringView str = find( sKey::Name() );
    par.set( sKey::Name(), str );
    par.set( ODGMT::sKeyShape(), "Line" );
    par.set( sKey::Size(), 1 );
    str = find( ODGMT::sKeyLineStyle() );
    par.set( ODGMT::sKeyLineStyle(), str );
    return true;
}


bool GMT2DLines::doExecute( od_ostream& strm, const char* fnm )
{
    StringView namestr = find( sKey::Name() );
    strm << "Posting 2D Lines " << namestr << " ...  ";

    TypeSet<Pos::GeomID> geomids;
    get( sKey::GeomID(), geomids );

    BufferStringSet linenms;
    OD::LineStyle ls;
    BufferString lsstr = find( ODGMT::sKeyLineStyle() );
    ls.fromString( lsstr );
    bool postlabel = false;
    getYN( ODGMT::sKeyPostLabel(), postlabel );

    BufferString mapprojstr, rgstr;
    mGetRangeString(rgstr)
    mGetProjString(mapprojstr,"X")
    mGetLineStyleString( ls, lsstr );

    OS::MachineCommand xymc( "psxy" );
    xymc.addArg( mapprojstr ).addArg( rgstr )
	.addArg( "-O" ).addArg( "-K" ).addArg( "-N" )
	.addArg( BufferString("-W",lsstr) );
    od_ostream procstrm = makeOStream( xymc, strm, fnm );
    if ( !procstrm.isOK() ) mErrStrmRet("Failed")

    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	mDynamicCastGet( const Survey::Geometry2D*, geom2d,
			 Survey::GM().getGeometry(geomids[idx]) );
	if ( !geom2d )
	    continue;

	const PosInfo::Line2DData& geom = geom2d->data();
	const TypeSet<PosInfo::Line2DPos>& posns = geom.positions();

	procstrm << "> " << geom2d->getName() << "\n";

	for ( int tdx=0; tdx<posns.size(); tdx++ )
	{
	    Coord pos = posns[tdx].coord_;
	    procstrm << pos.x << " " << pos.y << "\n";
	}
    }

    if ( !postlabel )
    {
	strm << "Done" << od_endl;
	return true;
    }

    int fontsz = 10;
    get( ODGMT::sKeyFontSize(), fontsz );
    BufferString colstr; mGetColorString( ls.color_, colstr );
    const bool modern = GMT::hasModernGMT();

    OS::MachineCommand textmc( "pstext" );
    textmc.addArg( mapprojstr ).addArg( rgstr )
	  .addArg( "-O" ).addArg( "-K" ).addArg( "-N" );
    if ( modern )
    {
	BufferString farg( "-F" );
	farg.add( "+f" ).add( fontsz ).add( "p,Helvetica," ).add( colstr );
	farg.add( "+a+j" );
	textmc.addArg( farg );
    }

    procstrm = makeOStream( textmc, strm, fnm );
    if ( !procstrm.isOK() )
	mErrStrmRet("Failed")

    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	mDynamicCastGet( const Survey::Geometry2D*, geom2d,
			 Survey::GM().getGeometry(geomids[idx]) );
	if ( !geom2d )
	    continue;

	const PosInfo::Line2DData& geom = geom2d->data();
	const TypeSet<PosInfo::Line2DPos>& posns = geom.positions();
	const int nrtrcs = posns.size();
	Coord pos = posns[0].coord_;
	Coord cvec = posns[1].coord_ - posns[0].coord_;
	float angle = mCast(float, Math::Atan2( cvec.y, cvec.x ) );
	float dy = sin( angle );
	float dx = cos( angle );
	angle = Math::toDegrees( angle );

	float rotangle = fabs(angle) > 90 ? 180+angle : angle;
	float perpangle = angle > 0 ? angle - 90 : angle + 90;
	BufferString al( fabs(angle) > 90 ? "ML" : "MR" );
	bool poststart = true;
	getYN( ODGMT::sKeyPostStart(), poststart );
	const float distfactor = xrg.width() / 100;
	if ( poststart )
	{
	    pos -= Coord( distfactor*dx, distfactor*dy );
	    postText( pos, fontsz, rotangle, al, geom2d->getName(),
		      modern, procstrm );
	}

	bool poststop = false;
	getYN( ODGMT::sKeyPostStop(), poststop );
	if ( poststop )
	{
	    pos = posns[nrtrcs-1].coord_;
	    cvec = posns[nrtrcs-2].coord_ - pos;
	    angle = mCast(float, Math::Atan2( cvec.y, cvec.x ) );
	    dy = sin( angle );
	    dx = cos( angle );
	    angle = Math::toDegrees( angle );
	    rotangle = fabs(angle) > 90 ? 180+angle : angle;
	    pos -= Coord( distfactor*dx, distfactor*dy );
	    al.set( fabs(angle) > 90 ? "ML" : "MR" );
	    postText( pos, fontsz, rotangle, al, geom2d->getName(),
		      modern, procstrm );
	}

	bool postnrs = true;
	getYN( ODGMT::sKeyPostTraceNrs(), postnrs );
	if ( postnrs )
	{
	    int labelintv = 100;
	    get( ODGMT::sKeyLabelIntv(), labelintv );
	    al.set( "ML" );
	    for ( int tdx=0; tdx<posns.size(); tdx+=labelintv )
	    {
		BufferString lbl = "- "; lbl += posns[tdx].nr_;
		Coord posc = posns[tdx].coord_;
		if ( tdx > 4 && tdx < posns.size()-5 )
		{
		    cvec = posns[tdx+5].coord_ - posns[tdx-5].coord_;
		    angle = mCast( float,
			   Math::toDegrees( Math::Atan2( cvec.y, cvec.x ) ) );
		    perpangle = angle > 0 ? angle - 90 : angle + 90;
		}

		postText( posc, fontsz, perpangle, al, lbl, modern, procstrm );
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

GMTPar* GMTRandLines::createInstance( const IOPar& iop, const char* workdir )
{
    return new GMTRandLines( iop, workdir );
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
    StringView str = find( sKey::Name() );
    par.set( sKey::Name(), str );
    par.set( ODGMT::sKeyShape(), "Line" );
    par.set( sKey::Size(), 1 );
    str = find( ODGMT::sKeyLineStyle() );
    par.set( ODGMT::sKeyLineStyle(), str );
    return true;
}


bool GMTRandLines::doExecute( od_ostream& strm, const char* fnm )
{
    MultiID id;
    get( sKey::ID(), id );
    const IOObj* ioobj = IOM().get( id );
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

    BufferString mapprojstr, rgstr;
    mGetRangeString(rgstr)
    mGetProjString(mapprojstr,"X")
    mGetLineStyleString( ls, lsstr );

    OS::MachineCommand xymc( "psxy" );
    xymc.addArg( mapprojstr ).addArg( rgstr )
	.addArg( "-O" ).addArg( "-K" ).addArg( "-N" )
	.addArg( BufferString("-W",lsstr) );
    od_ostream procstrm = makeOStream( xymc, strm, fnm );
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
	    procstrm << posc.x << " " << posc.y << "\n";
	}
    }

    if ( !postlabel )
    {
	strm << "Done" << od_endl;
	return true;
    }

    int fontsz = 10;
    get( ODGMT::sKeyFontSize(), fontsz );
    BufferString colstr; mGetColorString( ls.color_, colstr );
    const bool modern = GMT::hasModernGMT();

    OS::MachineCommand textmc( "pstext" );
    textmc.addArg( mapprojstr ).addArg( rgstr )
	  .addArg( "-O" ).addArg( "-K" ).addArg( "-N" );
    if ( modern )
    {
	BufferString farg( "-F" );
	farg.add( "+f" ).add( fontsz ).add( "p,Helvetica," ).add( colstr );
	farg.add( "+a+j" );
	textmc.addArg( farg );
    }

    procstrm = makeOStream( textmc, strm, fnm );
    if ( !procstrm.isOK() )
	mErrStrmRet("Failed")

    for ( int idx=0; idx<inprls.size(); idx++ )
    {
	const Geometry::RandomLine* rdl = inprls.lines()[idx];
	if ( !rdl || linenms.indexOf(rdl->name()) < 0 || rdl->nrNodes() < 2 )
	    continue;

	Coord posc = SI().transform( rdl->nodePosition(0) );
	const Coord cvec = SI().transform( rdl->nodePosition(1) ) - posc;
	float angle = mCast(float, Math::Atan2( cvec.y, cvec.x ) );
	const float dy = cos( angle );
	const float dx = sin( angle );
	angle = Math::toDegrees( angle );

	const float rotangle = fabs(angle) > 90 ? 180+angle : angle;
	const BufferString al( fabs(angle) > 90 ? "BR" : "BL" );
	const float distfactor = xrg.width() / 100;
	posc += Coord( -distfactor*dx, distfactor*dy );
	GMT2DLines::postText( posc, fontsz, rotangle, al, rdl->name(), modern,
			      procstrm );
    }

    strm << "Done" << od_endl;
    return true;
}
