/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		April 2010
RCS:		$Id: gmtfault.cc,v 1.7 2011-04-25 17:48:42 cvsyuancheng Exp $
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
#include "strmdata.h"
#include "survinfo.h"
#include "typeset.h"


int GMTFault::factoryid_ = -1;

void GMTFault::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Fault", GMTFault::createInstance );
}


GMTPar* GMTFault::createInstance( const IOPar& iop )
{
    return new GMTFault( iop );
}


const char* GMTFault::userRef() const
{
    static BufferString userref;
    userref = BufferString( "Fault: ", find(sKey::Name) );
    return userref.buf();
}


bool GMTFault::fillLegendPar( IOPar& par ) const
{
    FixedString str = find( sKey::Name );
    par.set( sKey::Name, str );
    par.set( ODGMT::sKeyShape, "Line" );
    par.set( sKey::Size, 1 );
    str = find( ODGMT::sKeyLineStyle );
    par.set( ODGMT::sKeyLineStyle, str );
    return true;
}


bool GMTFault::execute( std::ostream& strm, const char* fnm )
{
    bool onzslice = false;
    getYN( ODGMT::sKeyZIntersectionYN, onzslice );

    MultiID mid;
    get( ODGMT::sKeyFaultID, mid );

    RefMan<EM::EMObject> emobj = EM::EMM().loadIfNotFullyLoaded( mid );
    if ( !emobj )
	return false;

    mDynamicCastGet(EM::Fault3D*,fault3d,emobj.ptr())
    if ( !fault3d )
	return false;

    strm << "Creating Fault " << emobj->name() << " ... ";

    LineStyle ls;
    BufferString lsstr = find( ODGMT::sKeyLineStyle ).str();
    ls.fromString( lsstr );
    
    BufferString comm = "@psxy ";
    BufferString rgstr; mGetRangeProjString( rgstr, "X" );
    comm += rgstr;
    comm += " -O -K -M -N";
    mGetLineStyleString( ls, lsstr );
    comm += " -W"; comm += lsstr;

    comm += " 1>> "; comm += fileName( fnm );
    StreamData sd = makeOStream( comm, strm );
    if ( !sd.usable() ) mErrStrmRet("Failed");

    EM::SectionID fltsid = fault3d->sectionID( 0 );
    PtrMan<Geometry::ExplFaultStickSurface> fltsurf = 
	new Geometry::ExplFaultStickSurface(
		fault3d->geometry().sectionGeometry(fltsid), SI().zFactor() );
    fltsurf->setCoordList( new Coord3ListImpl, new Coord3ListImpl, 0 );
    if ( !fltsurf->update(true,0) )
	return false;

    if( onzslice )
    {
	float zval;
	get( ODGMT::sKeyZVals, zval );
	TypeSet<Coord3> corners = getCornersOfZSlice( zval );
	Coord3 normal = ( corners[1] - corners[0] )
	    		.cross( corners[3] - corners[0] ).normalize();


	PtrMan<Geometry::ExplPlaneIntersection> insectn =
					    new Geometry::ExplPlaneIntersection;
	insectn->setShape( *fltsurf );
	insectn->addPlane( normal, corners );
	Geometry::IndexedShape* idxdshape = insectn;
	idxdshape->setCoordList( new Coord3ListImpl, new Coord3ListImpl, 0 );
	if ( !idxdshape->update(true,0) )
	    return false;

	RefMan<Coord3List> clist = idxdshape->coordList();
	if ( !clist )
	    return false;

	const Geometry::IndexedGeometry* idxgeom = idxdshape->getGeometry()[0];
	if ( !idxgeom )
	    return false;

	const int sz = idxgeom->coordindices_.size();
	if ( sz == 0 )
	{
	    strm << "Selected ZSlice and Fault are not intersected\n";
	    return false;
	}

	*sd.ostrm << "> " << std::endl;
	for ( int cidx=0; cidx<sz; cidx++ )
	{
	    if ( idxgeom->coordindices_[cidx] == -1 )
		*sd.ostrm << "> " << std::endl;
	    else	
	    {
		double x = clist->get( idxgeom->coordindices_[cidx] ).x;
		double y = clist->get( idxgeom->coordindices_[cidx] ).y;
		*sd.ostrm << x << " " << y << std::endl;
	    }
	}
    }
    else
    {
	Coord3ListImpl clist;
	if ( !calcOnHorizon( *fltsurf, clist ) )
	    return false;

	if ( clist.getSize() == 0 )
	{
	    strm << "Selected Horizon and Fault are not intersected\n";
	    return false;
	}

	*sd.ostrm << "> " << std::endl;
	for ( int idx=0; idx<clist.getSize(); idx++ )
	{
	    double x = clist.get( idx ).x;
	    double y = clist.get( idx ).y;
	    *sd.ostrm << x << " " << y << std::endl;
	}
    }

    sd.close();
    strm << "Done" << std::endl;

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
    get( ODGMT::sKeyHorizonID, mid );
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
    if ( !horfltinsec )
	return false;

    horfltinsec->compute();
    return true;
}
