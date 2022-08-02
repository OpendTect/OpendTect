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
#include "ioman.h"
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

    liop_ = nullptr;
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
    const Pos::GeomID geomid = Survey::GM().getGeomID( linename );
    return indexOf( geomid );
}

bool Seis2DDataSet::isPresent( Pos::GeomID geomid ) const
{ return indexOf( geomid ) >= 0; }

bool Seis2DDataSet::isPresent( const char* linename ) const
{ return indexOf( linename ) >= 0; }

bool Seis2DDataSet::getGeometry( Pos::GeomID geomid,
				 PosInfo::Line2DData& geom ) const
{
    if ( !liop_ )
    {
	ErrMsg("No suitable 2D line information object found");
	return 0;
    }

    if ( !isPresent(geomid) )
    {
	ErrMsg("Requested line not found in Dataset");
	return 0;
    }

    return liop_->getGeometry( ioobj_, geomid, geom );
}


void Seis2DDataSet::getLineNames( BufferStringSet& nms ) const
{
    nms.erase();
    for ( int idx=0; idx<nrLines(); idx++ )
	nms.add( lineName(idx) );
}


void Seis2DDataSet::getGeomIDs( TypeSet<Pos::GeomID>& geomids ) const
{ geomids = geomids_; }

Executor* Seis2DDataSet::lineFetcher( Pos::GeomID geomid, SeisTrcBuf& tbuf,
				      int ntps, const Seis::SelData* sd ) const
{
    if ( !liop_ )
    {
	ErrMsg("No suitable 2D line extraction object found");
	return nullptr;
    }

    if ( !isPresent(geomid) )
    {
	ErrMsg("Requested line not found in Dataset");
	return nullptr;
    }

    Executor* fetcher = liop_->getFetcher( ioobj_, geomid, tbuf, ntps, sd );
    auto* getter = dynamic_cast<Seis2DLineGetter*>(fetcher);
    const SeisTrcTranslator* trl = getter ? getter->translator() : nullptr;
    if ( trl )
    {
	if ( trl->curGeomID() != geomid )
	{
	    pErrMsg("Should not happen");
	    const_cast<SeisTrcTranslator*>(trl)->setCurGeomID( geomid );
	}
    }

    return fetcher;
}


Seis2DLinePutter* Seis2DDataSet::linePutter( Pos::GeomID newgeomid )
{
    if ( !liop_ )
    {
	ErrMsg("No suitable 2D line creation object found");
	return 0;
    }

    if ( !isPresent(newgeomid) )
    {
	if ( !readonly_ )
	    geomids_ += newgeomid;
	else
	{
	    BufferString msg( "Read-only Dataset chg req: " );
	    msg += Survey::GM().getName(newgeomid); msg += " not yet in set ";
	    msg += name();
	    pErrMsg( msg );
	    return 0;
	}
    }

    return liop_->getPutter( ioobj_, newgeomid );
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
    const Survey::Geometry* geometry = Survey::GM().getGeometry( geomid );
    mDynamicCastGet( const Survey::Geometry2D*, geom2d, geometry )
    if ( !geom2d ) return false;

    const PosInfo::Line2DData& geom = geom2d->data();
    for ( int idx=0; idx<geom.positions().size(); idx++ )
    {
	if ( bivs.includes( SI().transform(geom.positions()[idx].coord_) ) )
	    return true;
    }

    return false;
}


void Seis2DDataSet::getDataSetsOnLine( const char* lnm, BufferStringSet& ds )
{ Seis2DDataSet::getDataSetsOnLine( Survey::GM().getGeomID(lnm), ds ); }


void Seis2DDataSet::getDataSetsOnLine( Pos::GeomID geomid,
				       BufferStringSet& ds )
{ SeisIOObjInfo::getDataSetNamesForLine( geomid, ds ); }
