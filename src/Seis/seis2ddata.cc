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
#include "safefileio.h"
#include "staticstring.h"
#include "ascstream.h"
#include "binnedvalueset.h"
#include "posinfo2dsurv.h"
#include "dirlist.h"
#include "survinfo.h"
#include "survgeom3d.h"
#include "file.h"
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
{
    mDeclStaticString( ret );
    ret = geomID(idx).name();
    return ret.str();
}

Pos::GeomID Seis2DDataSet::geomID( int idx ) const
{ return geomids_.validIdx(idx) ? geomids_[idx] : mUdfGeomID; }

int Seis2DDataSet::indexOf( GeomID geomid ) const
{ return geomids_.indexOf( geomid ); }

int Seis2DDataSet::indexOf( const char* linename ) const
{
    return indexOf( SurvGeom::getGeomID(linename) );
}

bool Seis2DDataSet::isPresent( GeomID geomid ) const
{ return indexOf( geomid ) >= 0; }

bool Seis2DDataSet::isPresent( const char* linename ) const
{ return indexOf( linename ) >= 0; }

uiRetVal Seis2DDataSet::getGeometry( GeomID geomid,
				 PosInfo::Line2DData& geom ) const
{
    if ( !liop_ )
	return uiRetVal( tr("No suitable 2D line information object found") );

    if ( !isPresent(geomid) )
	return uiRetVal( tr("Requested line not found in Dataset") );

    const uiRetVal uirv = liop_->getGeometry( ioobj_, geomid, geom );
    if ( uirv.isOK() )
	geom.setGeomID( geomid );

    return uirv;
}


void Seis2DDataSet::getLineNames( BufferStringSet& nms ) const
{
    nms.erase();
    for ( int idx=0; idx<nrLines(); idx++ )
	nms.add( lineName(idx) );
}


void Seis2DDataSet::getGeomIDs( GeomIDSet& geomids ) const
{
    geomids = geomids_;
}


Seis2DTraceGetter* Seis2DDataSet::traceGetter( GeomID geomid,
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


Seis2DLinePutter* Seis2DDataSet::linePutter( GeomID geomid,
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
		    .arg( name() ).arg( geomid.name() ) );
	    return 0;
	}
    }

    return liop_->getPutter( ioobj_, geomid, uirv );
}


bool Seis2DDataSet::isEmpty( GeomID geomid ) const
{
    return liop_ ? liop_->isEmpty( ioobj_, geomid ) : true;
}


bool Seis2DDataSet::remove( GeomID geomid )
{
    if ( readonly_ || !liop_ || !isPresent(geomid) )
	return false;

    liop_->removeImpl( ioobj_, geomid );
    geomids_ -= geomid;
    return true;
}


bool Seis2DDataSet::rename( const char* newname )
{ return liop_ && liop_->renameImpl( ioobj_, newname ); }

bool Seis2DDataSet::getTxtInfo( GeomID geomid, BufferString& uinf,
				BufferString& stdinf ) const
{ return liop_ && liop_->getTxtInfo( ioobj_, geomid, uinf, stdinf ); }

bool Seis2DDataSet::getRanges( GeomID geomid, StepInterval<int>& sii,
			       StepInterval<float>& sif ) const
{ return liop_ && liop_->getRanges( ioobj_, geomid, sii, sif ); }

bool Seis2DDataSet::haveMatch( GeomID geomid,
			       const BinnedValueSet& bivs ) const
{
    if ( bivs.is2D() )
	return bivs.hasInl( geomid.lineNr() );

    const auto& geom2d = SurvGeom::get2D( geomid );
    if ( geom2d.isEmpty() )
	return false;
    const auto& geom3d = SurvGeom::get3D();

    const PosInfo::Line2DData& l2dd = geom2d.data();
    for ( int idx=0; idx<l2dd.positions().size(); idx++ )
    {
	if ( bivs.includes( geom3d.transform(l2dd.positions()[idx].coord_) ) )
	    return true;
    }

    return false;
}


void Seis2DDataSet::getDataSetsOnLine( const char* lnm, BufferStringSet& ds )
{
    Seis2DDataSet::getDataSetsOnLine( SurvGeom::getGeomID(lnm), ds );
}


void Seis2DDataSet::getDataSetsOnLine( GeomID geomid, BufferStringSet& ds )
{
    SeisIOObjInfo::getDataSetNamesForLine( geomid, ds );
}
