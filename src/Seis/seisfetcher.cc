/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2016
________________________________________________________________________

-*/

#include "seisfetcher.h"
#include "seisprovider.h"
#include "seisioobjinfo.h"
#include "seisselectionimpl.h"
#include "seis2ddata.h"
#include "posinfo2d.h"
#include "dbman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "uistrings.h"


Seis::Fetcher::Fetcher( Provider& p )
    : prov_(p)
    , ioobj_(0)
{
}


Seis::Fetcher::~Fetcher()
{
    delete ioobj_;
}


void Seis::Fetcher::reset()
{
    uirv_.setEmpty();
    delete ioobj_; ioobj_ = 0;
}


IOObj* Seis::Fetcher::getIOObj() const
{
    return DBM().get( prov_.dbky_ );
}


bool Seis::Fetcher::fillIOObj()
{
    delete ioobj_;
    ioobj_ = getIOObj();
    if ( !ioobj_ )
    {
	uirv_ = uiStrings::phrCannotFindDBEntry( prov_.dbky_.toUiString() );
	return false;
    }
    return true;
}


Seis::Provider3D& Seis::Fetcher3D::prov3D()
{
    return static_cast<Provider3D&>( prov_ );
}


const Seis::Provider3D& Seis::Fetcher3D::prov3D() const
{
    return static_cast<const Provider3D&>( prov_ );
}


void Seis::Fetcher3D::reset()
{
    Fetcher::reset();

    getReqCS();

    nextbid_ = reqcs_.hsamp_.start_ - reqcs_.hsamp_.step_;
    if ( !moveNextBinID() )
	uirv_.set( tr("No traces available for current selection") );
}


TrcKeyZSampling Seis::Fetcher3D::getDefaultCS() const
{
    return TrcKeyZSampling( true );
}


void Seis::Fetcher3D::getReqCS()
{
    // set the default to everything; also sets proper steps
    SeisIOObjInfo objinf( prov_.dbky_ );
    if ( !objinf.getRanges(reqcs_) )
	reqcs_ = getDefaultCS();

    if ( prov_.seldata_ && !prov_.seldata_->isAll() )
    {
	const Seis::SelData& seldata = *prov_.seldata_;
	const Interval<float> reqzrg( seldata.zRange() );
	const Interval<int> reqinlrg( seldata.inlRange() );
	const Interval<int> reqcrlrg( seldata.crlRange() );
	reqcs_.zsamp_.start = reqzrg.start;
	reqcs_.zsamp_.stop = reqzrg.stop;
	reqcs_.hsamp_.start_.inl() = reqinlrg.start;
	reqcs_.hsamp_.stop_.inl() = reqinlrg.stop;
	reqcs_.hsamp_.start_.crl() = reqcrlrg.start;
	reqcs_.hsamp_.stop_.crl() = reqcrlrg.stop;
    }
}


bool Seis::Fetcher3D::isSelectedBinID( const BinID& bid ) const
{
    return !prov_.seldata_ || prov_.seldata_->isOK(bid);
}


bool Seis::Fetcher3D::moveNextBinID()
{
    while ( true )
    {
	if ( !reqcs_.hsamp_.toNext(nextbid_) )
	    return false;
	if ( isSelectedBinID(nextbid_) )
	    return true;
    }
}


Seis::Fetcher2D::~Fetcher2D()
{
    delete dataset_;
}


Seis::Provider2D& Seis::Fetcher2D::prov2D()
{
    return static_cast<Provider2D&>( prov_ );
}


const Seis::Provider2D& Seis::Fetcher2D::prov2D() const
{
    return static_cast<const Provider2D&>( prov_ );
}


void Seis::Fetcher2D::reset()
{
    Fetcher::reset();
    delete dataset_; dataset_ = 0;
    nexttrcky_.setGeomID( mUdfGeomID );
    curlidx_ = -1;
}


void Seis::Fetcher2D::openDataSet()
{
    if ( !fillIOObj() )
	return;

    dataset_ = new Seis2DDataSet( *ioobj_ );
    if ( dataset_->isEmpty() )
    {
	uirv_ = tr( "Cannot find any data for this attribute" );
	delete dataset_; dataset_ = 0;
    }
}


bool Seis::Fetcher2D::toNextLine()
{
    if ( !dataset_ )
	return false;

    curlidx_++;
    if ( curlidx_ >= dataset_->nrLines() )
	return false;

    const Seis::SelData* sd = prov2D().seldata_;
    const bool issingleline = mIsSingleLine( sd );
    const bool istable = sd && sd->type() == Seis::Table;

    if ( issingleline )
    {
	curlidx_ = lineIdxFor( sd->geomID() );
	if ( curlidx_ < 0 )
	    return false;
    }
    else if ( istable )
    {
	mDynamicCastGet(const Seis::TableSelData*,tsd,sd)
	while ( !dataset_->haveMatch(dataset_->geomID(curlidx_),
				     tsd->binidValueSet()) )
	{
	    curlidx_++;
	    if ( curlidx_ >= dataset_->nrLines() )
		return false;
	}
    }

    return true;
}


Seis2DDataSet* Seis::Fetcher2D::mkDataSet() const
{
    PtrMan<IOObj> ioobj = getIOObj();
    return ioobj ? new Seis2DDataSet( *ioobj ) : 0;
}


int Seis::Fetcher2D::lineIdxFor( Pos::GeomID geomid ) const
{
    const int nrlines = dataset_ ? dataset_->nrLines() : 0;
    for ( int lidx=0; lidx<nrlines; lidx++ )
    {
	if ( dataset_->geomID(lidx) == geomid )
	    return lidx;
    }
    return -1;
}


Pos::GeomID Seis::Fetcher2D::curGeomID() const
{
    return dataset_ && curlidx_>=0 ? dataset_->geomID( curlidx_ ) : mUdfGeomID;
}


uiRetVal Seis::Fetcher2D::gtComponentInfo( BufferStringSet& nms,
				       TypeSet<Seis::DataType>& dts ) const
{
    Seis2DDataSet* ds = dataset_;
    PtrMan<Seis2DDataSet> ptrds;
    if ( !ds )
	{ ds = mkDataSet(); ptrds = ds; }
    if ( !ds )
	return uirv_;

    const BufferString dtyp = ds->dataType();
    nms.add( ds->name() );
    if ( dtyp != sKey::Steering() )
	dts += Seis::UnknowData;
    else
    {
	nms.add( ds->name() );
	dts += Seis::Dip; dts += Seis::Dip;
    }

    uirv_.setEmpty();
    return uirv_;
}


int Seis::Fetcher2D::gtNrLines() const
{
    if ( dataset_ )
	return dataset_->nrLines();
    else
    {
	PtrMan<Seis2DDataSet> ds = mkDataSet();
	return ds ? ds->nrLines() : 0;
    }
}


Pos::GeomID Seis::Fetcher2D::gtGeomID( int iln ) const
{
    if ( dataset_ )
	return dataset_->geomID( iln );
    else
    {
	PtrMan<Seis2DDataSet> ds = mkDataSet();
	return ds ? ds->geomID(iln) : mUdfGeomID;
    }
}


int Seis::Fetcher2D::gtLineNr( Pos::GeomID geomid ) const
{
    if ( dataset_ )
	return dataset_->indexOf( geomid );
    else
    {
	PtrMan<Seis2DDataSet> ds = mkDataSet();
	return ds ? ds->indexOf(geomid) : -1;
    }
}


BufferString Seis::Fetcher2D::gtLineName( int lnr ) const
{
    if ( dataset_ )
	return dataset_->lineName( lnr );
    else
    {
	PtrMan<Seis2DDataSet> ds = mkDataSet();
	return BufferString( ds ? ds->lineName(lnr) : "" );
    }
}


void Seis::Fetcher2D::gtGeometryInfo( int iln, PosInfo::Line2DData& l2dd ) const
{
    l2dd.setEmpty();

    if ( dataset_ )
	dataset_->getGeometry( dataset_->geomID(iln), l2dd );
    else
    {
	PtrMan<Seis2DDataSet> ds = mkDataSet();
	if ( ds )
	    ds->getGeometry( ds->geomID(iln), l2dd );
    }
}


bool Seis::Fetcher2D::gtRanges( int iln, StepInterval<int>& trcrg,
				ZSampling& zsamp ) const
{
    if ( dataset_ )
	return dataset_->getRanges( dataset_->geomID(iln),
					    trcrg, zsamp );
    else
    {
	PtrMan<Seis2DDataSet> ds = mkDataSet();
	if ( ds )
	    return ds->getRanges( ds->geomID(iln), trcrg, zsamp );
    }

    return false;
}
