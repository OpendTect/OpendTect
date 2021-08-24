/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		April 2010
________________________________________________________________________

-*/

#include "gmtfault.h"

#include "binidsurface.h"
#include "emfault3d.h"
#include "emfaultstickset.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "explfaultsticksurface.h"
#include "explplaneintersection.h"
#include "faulthorintersect.h"
#include "indexedshape.h"
#include "iopar.h"
#include "keystrs.h"
#include "multiid.h"
#include "position.h"
#include "positionlist.h"
#include "ptrman.h"
#include "refcount.h"
#include "string2.h"
#include "survinfo.h"
#include "typeset.h"

#include "od_ostream.h"


int GMTFault::factoryid_ = -1;

void GMTFault::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Fault", GMTFault::createInstance );
}


GMTPar* GMTFault::createInstance( const IOPar& iop, const char* workdir )
{
    return new GMTFault( iop, workdir );
}


const char* GMTFault::userRef() const
{
    mDeclStaticString( userref );
    userref = BufferString( "Fault: ", find(sKey::Name()) );
    return userref.buf();
}


bool GMTFault::fillLegendPar( IOPar& par ) const
{
    FixedString str = find( sKey::Name() );
    par.set( sKey::Name(), str );
    par.set( ODGMT::sKeyShape(), "Line" );
    par.set( sKey::Size(), 1 );
    str = find( ODGMT::sKeyLineStyle() );
    OD::LineStyle ls; ls.fromString( str );
    Color col;
    if ( get(ODGMT::sKeyFaultColor(),col) )
	ls.color_ = col;

    BufferString lsstr; ls.toString( lsstr );
    par.set( ODGMT::sKeyLineStyle(), lsstr );
    return true;
}

#define mErrRet(s) { deepUnRef( flts_ ); strm << s << od_endl; return false; }

bool GMTFault::doExecute( od_ostream& strm, const char* fnm )
{
    bool usecoloryn = false;
    getYN( ODGMT::sKeyUseFaultColorYN(), usecoloryn );

    uiString errmsg;
    if ( !loadFaults(errmsg) )
	mErrRet( errmsg )

    BufferStringSet styles;
    getLineStyles( styles );
    if ( (usecoloryn && styles.size() != flts_.size()) ||
	(!usecoloryn && styles.size() != 1) )
	mErrRet(tr("Failed to fetch OD::LineStyles"))

    BufferString mapprojstr, rgstr;
    mGetRangeString(rgstr)
    mGetProjString(mapprojstr,"X")

    OS::MachineCommand xymc( "psxy" );
    xymc.addArg( mapprojstr ).addArg( rgstr )
	.addArg( "-O" ).addArg( "-K" ).addArg( "-N" );
    od_ostream procstrm = makeOStream( xymc, strm, fnm );
    if ( !procstrm.isOK() ) mErrRet(tr("Failed to execute GMT command"));

    bool onzslice = false;
    getYN( ODGMT::sKeyZIntersectionYN(), onzslice );

    for ( int midx=0; midx<flts_.size(); midx++ )
    {
        EM::Fault3D* fault3d = flts_[midx];
	if ( !fault3d )
	    continue;

	EM::SectionID fltsid = fault3d->sectionID( 0 );
	Geometry::FaultStickSurface* fsssurf =
				fault3d->geometry().sectionGeometry(fltsid);
	PtrMan<Geometry::ExplFaultStickSurface> fltsurf =
	    new Geometry::ExplFaultStickSurface( fsssurf,
				  mCast(float,SI().zDomain().userFactor()) );
	fltsurf->setCoordList( new Coord3ListImpl, new Coord3ListImpl, 0 );
	if ( !fltsurf->update(true,0) )
	    continue;

	strm << "Posting Fault --> " << fault3d->name() << " ...\n";
	BufferString clr( " -W" );
	usecoloryn ? clr.add( styles.get(midx) ) : clr.add( styles.get(0) );
	procstrm << "> " << clr.buf() << "\n";

	if( onzslice )
	{
	    float zval;
	    get( ODGMT::sKeyZVals(), zval );
	    TypeSet<Coord3> corners = getCornersOfZSlice( zval );
	    Coord3 normal = ( corners[1] - corners[0] )
			     .cross( corners[3] - corners[0] ).normalize();

	    PtrMan<Geometry::ExplPlaneIntersection> insectn =
					new Geometry::ExplPlaneIntersection;
	    insectn->setShape( *fltsurf );
	    insectn->addPlane( normal, corners );
	    Geometry::IndexedShape* idxdshape = insectn;
	    idxdshape->setCoordList(new Coord3ListImpl, new Coord3ListImpl, 0);
	    if ( !idxdshape->update(true,0) )
		continue;

	    RefMan<Coord3List> clist = idxdshape->coordList();
	    if ( !clist )
		continue;

	    Geometry::IndexedGeometry* idxgeom =
						idxdshape->getGeometry()[0];
	    if ( !idxgeom )
		continue;

	    Geometry::PrimitiveSet* coordps = idxgeom->getCoordsPrimitiveSet();
	    const int sz = coordps->size();
	    if ( sz == 0 )
	    {
		uiString msg = tr("Selected ZSlice and %1 are not intersected")
                             .arg(fault3d->name());
		strm << '\t' << msg << '\n';
		continue;
	    }

	    for ( int cidx=1; cidx<sz; cidx+=2 )
	    {
		double x = clist->get( coordps->get(cidx-1) ).x;
		double y = clist->get( coordps->get(cidx-1) ).y;
		procstrm << x << " " << y << "\n";

		x = clist->get( coordps->get(cidx) ).x;
		y = clist->get( coordps->get(cidx) ).y;
		procstrm << x << " " << y << "\n";

		procstrm << "> " << "\n";
	    }
	}
	else
	{
	    Coord3ListImpl clist;
	    if ( !calcOnHorizon( *fltsurf, clist ) )
		continue;

	    const int csz = clist.size();
	    if ( csz < 1 )
	    {
		uiString msg = tr("Selected Horizon and %1 are"
                                  " not intersected").arg(fault3d->name());
		strm << '\t' << msg << '\n';
		continue;
	    }

	    for ( int idx=0; idx<csz; idx++ )
	    {
		double x = clist.get( idx ).x;
		double y = clist.get( idx ).y;
		procstrm << x << " " << y << "\n";
	    }
	}
    }

    strm << "Done" << od_endl;
    deepUnRef( flts_ );
    return true;
}


TypeSet<Coord3> GMTFault::getCornersOfZSlice( float zval ) const
{
    BinID b0( SI().inlRange(true).start, SI().crlRange(true).start );
    BinID b1( SI().inlRange(true).start, SI().crlRange(true).stop );
    BinID b2( SI().inlRange(true).stop, SI().crlRange(true).stop );
    BinID b3( SI().inlRange(true).stop, SI().crlRange(true).start );
    Coord3 p0( SI().transform( b0 ), zval );
    Coord3 p1( SI().transform( b1 ), zval );
    Coord3 p2( SI().transform( b2 ), zval );
    Coord3 p3( SI().transform( b3 ), zval );
    TypeSet<Coord3> pts;
    pts += p0; pts += p1; pts += p2; pts += p3;
    return pts;
}


bool GMTFault::calcOnHorizon( const Geometry::ExplFaultStickSurface& expfault,
			      Coord3ListImpl& clist ) const
{
    MultiID mid;
    get( ODGMT::sKeyHorizonID(), mid );
    RefMan<EM::EMObject> emobj = EM::EMM().loadIfNotFullyLoaded( mid );
    if ( !emobj )
	return false;

    mDynamicCastGet(EM::Horizon3D*,horizon3d,emobj.ptr())
    if ( !horizon3d )
	return false;

    Geometry::BinIDSurface* bidsurface =
			    horizon3d->geometry().sectionGeometry( 0 );
    if ( !bidsurface )
	return false;

    PtrMan<Geometry::FaultBinIDSurfaceIntersector> horfltinsec =
	    new Geometry::FaultBinIDSurfaceIntersector( (float)0, *bidsurface,
							expfault, clist );
    horfltinsec->compute();
    return true;
}


void GMTFault::getLineStyles( BufferStringSet& styles )
{
    bool usecoloryn = false;
    getYN( ODGMT::sKeyUseFaultColorYN(), usecoloryn );
    OD::LineStyle ls;
    BufferString lsstr = find( ODGMT::sKeyLineStyle() );
    ls.fromString( lsstr );
    if ( !usecoloryn )
    {
	Color clr;
	get( ODGMT::sKeyFaultColor(), clr );
	ls.color_ = clr;
	BufferString clrstr;
	mGetLineStyleString( ls, clrstr );
	styles.add( clrstr );
	return;
    }

    for ( int idx=0; idx<flts_.size(); idx++ )
    {
	const EM::Fault3D* fault3d = flts_[idx];
	if ( !fault3d )
	    continue;

	if ( usecoloryn )
	{
	    BufferString str;
	    const Color& fltclr = fault3d->preferredColor();
	    ls.color_ = fltclr;
	    if ( !idx )
		set( ODGMT::sKeyFaultColor(), fltclr );

	    mGetLineStyleString( ls, str );
	    styles.add( str );
	}
    }
}


bool GMTFault::loadFaults( uiString& errmsg )
{
    IOPar* fltpar = subselect( ODGMT::sKeyFaultID() );
    if ( !fltpar )
    {
	errmsg = tr("No faults selected");
	return false;
    }

    for ( int idx=0; idx<fltpar->size(); idx++ )
    {
	MultiID mid;
	if ( !fltpar->get( toString(idx), mid ) )
	    break;

	EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( mid );
	mDynamicCastGet(EM::Fault3D*,fault3d,emobj)
	if ( !fault3d )
	{
	    errmsg = tr("Failed to load faults");
	    return false;
	}

	fault3d->ref();
	flts_ += fault3d;
    }

    return true;
}
