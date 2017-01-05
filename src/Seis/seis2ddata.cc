/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Dec 2012
-*/


#include "seis2ddata.h"

#include "seis2dlineio.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "survgeom2d.h"
#include "survgeom.h"
#include "linesetposinfo.h"
#include "dbman.h"
#include "safefileio.h"
#include "ascstream.h"
#include "binidvalset.h"
#include "posinfo2dsurv.h"
#include "dirlist.h"
#include "survinfo.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "od_ostream.h"



Seis2DDataSet::Seis2DDataSet( const IOObj& ioobj )
    : NamedObject(ioobj.name())
    , ioobj_(*ioobj.clone())
{
    ioobj.pars().get( sKey::Type(), datatype_ );
    init();
}


Seis2DDataSet::Seis2DDataSet( const Seis2DDataSet& s2dds )
    : NamedObject(s2dds.name())
    , datatype_(s2dds.dataType())
    , ioobj_(*s2dds.ioobj_.clone())
{
    init();
}


Seis2DDataSet::~Seis2DDataSet()
{ delete &ioobj_; }


void Seis2DDataSet::init()
{
    readonly_ = false;
    const BufferString typestr = ioobj_.translator();

    liop_ = 0;
    geomids_.erase();
    const ObjectSet<Seis2DLineIOProvider>& liops = S2DLIOPs();
    for ( int idx=0; idx<liops.size(); idx++ )
    {
	const Seis2DLineIOProvider* liop = liops[idx];
	if ( typestr == liop->type() )
	    { liop_ = const_cast<Seis2DLineIOProvider*>(liop); break; }
    }

    if ( liop_ )
	liop_->getGeomIDs( ioobj_, geomids_ );
}


const char* Seis2DDataSet::type() const
{ return liop_ ? liop_->type() : ioobj_.translator(); }

bool Seis2DDataSet::isEmpty() const
{ return !nrLines(); }

const char* Seis2DDataSet::lineName( int idx ) const
{ return Survey::GM().getName( geomID(idx) ); }

Pos::GeomID Seis2DDataSet::geomID( int idx ) const
{ return geomids_.validIdx(idx) ? geomids_[idx] : mUdfGeomID; }

int Seis2DDataSet::indexOf( Pos::GeomID geomid ) const
{ return geomids_.indexOf( geomid ); }

int Seis2DDataSet::indexOf( const char* linename ) const
{
    const int geomid = Survey::GM().getGeomID( linename );
    return indexOf( geomid );
}

bool Seis2DDataSet::isPresent( Pos::GeomID geomid ) const
{ return indexOf( geomid ) >= 0; }

bool Seis2DDataSet::isPresent( const char* linename ) const
{ return indexOf( linename ) >= 0; }

uiRetVal Seis2DDataSet::getGeometry( Pos::GeomID geomid,
				 PosInfo::Line2DData& geom ) const
{
    if ( !liop_ )
	return uiRetVal( tr("No suitable 2D line information object found") );

    if ( !isPresent(geomid) )
	return uiRetVal( tr("Requested line not found in Dataset") );

    return liop_->getGeometry( ioobj_, geomid, geom );
}


void Seis2DDataSet::getLineNames( BufferStringSet& nms ) const
{
    nms.erase();
    for ( int idx=0; idx<nrLines(); idx++ )
	nms.add( lineName(idx) );
}


void Seis2DDataSet::getGeomIDs( TypeSet<Pos::GeomID>& geomids ) const
{
    geomids = geomids_;
}


Seis2DTraceGetter* Seis2DDataSet::traceGetter( Pos::GeomID geomid,
			const Seis::SelData* sd, uiRetVal& uirv ) const
{
    if ( !liop_ )
    {
	uirv.set( tr("No suitable 2D line extraction object found") );
	return 0;
    }

    if ( !isPresent(geomid) )
    {
	uirv.set( tr("Requested line not found in Dataset") );
	return 0;
    }

    return liop_->getTraceGetter( ioobj_, geomid, sd, uirv );
}


Executor* Seis2DDataSet::lineGetter( Pos::GeomID geomid, SeisTrcBuf& tbuf,
	      const Seis::SelData* sd, uiRetVal& uirv, int npts ) const
{
    if ( !liop_ )
    {
	uirv.set( tr("No suitable 2D line extraction object found") );
	return 0;
    }

    if ( !isPresent(geomid) )
    {
	uirv.set( tr("Requested line not found in Dataset") );
	return 0;
    }

    return liop_->getLineGetter( ioobj_, geomid, tbuf, sd, uirv, npts );
}


Seis2DLinePutter* Seis2DDataSet::linePutter( Pos::GeomID geomid,
					     uiRetVal& uirv )
{
    if ( !liop_ )
    {
	uirv.set( tr("No suitable 2D line creation object found") );
	return 0;
    }

    if ( !isPresent(geomid) )
    {
	if ( !readonly_ )
	    geomids_ += geomid;
	else
	{
	    uirv.set( tr("Read-only 2D seismic data set %1: %2 not in set")
		    .arg( name() ).arg( Survey::GM().getName(geomid) ) );
	    return 0;
	}
    }

    return liop_->getPutter( ioobj_, geomid, uirv );
}


bool Seis2DDataSet::isEmpty( Pos::GeomID geomid ) const
{
    return liop_ ? liop_->isEmpty( ioobj_, geomid ) : true;
}


bool Seis2DDataSet::remove( Pos::GeomID geomid )
{
    if ( readonly_ || !liop_ || !isPresent(geomid) )
	return false;

    liop_->removeImpl( ioobj_, geomid );
    geomids_ -= geomid;
    return true;
}


bool Seis2DDataSet::rename( const char* newname )
{ return liop_ && liop_->renameImpl( ioobj_, newname ); }

bool Seis2DDataSet::getTxtInfo( Pos::GeomID geomid, BufferString& uinf,
				BufferString& stdinf ) const
{ return liop_ && liop_->getTxtInfo( ioobj_, geomid, uinf, stdinf ); }

bool Seis2DDataSet::getRanges( Pos::GeomID geomid, StepInterval<int>& sii,
			       StepInterval<float>& sif ) const
{ return liop_ && liop_->getRanges( ioobj_, geomid, sii, sif ); }

bool Seis2DDataSet::haveMatch( Pos::GeomID geomid,
			       const BinIDValueSet& bivs ) const
{
    if ( TrcKey::is2D(bivs.survID()) )
	return bivs.hasInl( geomid );

    const Survey::Geometry* geometry = Survey::GM().getGeometry( geomid );
    mDynamicCastGet( const Survey::Geometry2D*, geom2d, geometry )
    if ( !geom2d ) return false;

    geometry = Survey::GM().getGeometry( bivs.survID() );
    mDynamicCastGet( const Survey::Geometry3D*, geom3d, geometry )
    if ( !geom3d ) return false;

    const PosInfo::Line2DData& geom = geom2d->data();
    for ( int idx=0; idx<geom.positions().size(); idx++ )
    {
	if ( bivs.includes( geom3d->transform(geom.positions()[idx].coord_) ) )
	    return true;
    }

    return false;
}


void Seis2DDataSet::getDataSetsOnLine( const char* lnm, BufferStringSet& ds )
{ Seis2DDataSet::getDataSetsOnLine( Survey::GM().getGeomID(lnm), ds ); }


void Seis2DDataSet::getDataSetsOnLine( Pos::GeomID geomid,
				       BufferStringSet& ds )
{ SeisIOObjInfo::getDataSetNamesForLine( geomid, ds ); }
