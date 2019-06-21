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
#include "seispreload.h"
#include "seisrangeseldata.h"
#include "seistableseldata.h"
#include "seis2ddata.h"
#include "posinfo2d.h"
#include "ioobj.h"
#include "keystrs.h"
#include "uistrings.h"


void Seis::Fetcher::getDataPack()
{
    dp_ = Seis::PLDM().getDP( prov_.dbky_, prov_.curGeomID() );
}


void Seis::Fetcher::reset()
{
    uirv_.setOK();
    dp_ = nullptr;
}


Seis::Provider3D& Seis::Fetcher3D::prov3D()
{
    return static_cast<Provider3D&>( prov_ );
}


const Seis::Provider3D& Seis::Fetcher3D::prov3D() const
{
    return static_cast<const Provider3D&>( prov_ );
}


BinID Seis::Fetcher3D::curBid() const
{
    return static_cast<Provider3D&>( prov_ ).curBinID();
}


bool Seis::Fetcher3D::isSelectedBinID( const BinID& bid ) const
{
    return !prov_.selData() || prov_.selData()->isOK(bid);
}



Seis::Fetcher2D::~Fetcher2D()
{
    delete dataset_;
}


void Seis::Fetcher2D::reset()
{
    Fetcher::reset();
    deleteAndZeroPtr( dataset_ );
}


Seis::Provider2D& Seis::Fetcher2D::prov2D()
{
    return static_cast<Provider2D&>( prov_ );
}


const Seis::Provider2D& Seis::Fetcher2D::prov2D() const
{
    return static_cast<const Provider2D&>( prov_ );
}


bool Seis::Fetcher2D::isSelectedPosition( GeomID gid, int trcnr ) const
{
    return !prov_.selData() || prov_.selData()->isOK( gid, trcnr );
}


void Seis::Fetcher2D::ensureDataSet() const
{
    if ( !dataset_ )
	mSelf().dataset_ = new Seis2DDataSet( *prov_.ioObj() );
}


void Seis::Fetcher2D::getLineData( Line2DDataSet& l2dds ) const
{
    uirv_.setEmpty();
    l2dds.setEmpty();
    ensureDataSet();

    for ( auto iln=0; iln<dataset_->nrLines(); iln++ )
    {
	const auto geomid = dataset_->geomID( iln );
	auto* l2dd = new Line2DData;
	uiRetVal uirv = dataset_->getGeometry( geomid, *l2dd );
	if ( uirv.isOK() )
	    l2dds.add( l2dd );
	else
	    delete l2dd;
    }
}


bool Seis::Fetcher2D::toNextTrace()
{
    if ( prov2D().lineidx_ < 0 )
	return toNextLine();

    prov2D().trcidx_++;
    if ( !prov2D().l2dds_.get(prov2D().lineidx_)->validIdx(prov2D().trcidx_) )
	return toNextLine();
    return true;
}


bool Seis::Fetcher2D::toNextLine()
{
    prov2D().lineidx_++;
    if ( prov2D().lineidx_ >= prov2D().nrLines() )
	return false;

    const auto* sd = prov2D().selData();
    if ( sd && !sd->hasGeomID(prov2D().curGeomID()) )
	return toNextLine();

    prov2D().trcidx_ = 0;
    getDataPack();
    return true;
}


bool Seis::Fetcher2D::selectPosition( GeomID gid, trcnr_type tnr )
{
    if ( !gid.isValid() )
	{ uirv_.set( tr("Invalid position requested") ); return false; }

    int newlineidx = prov2D().l2dds_.lineIndexOf( gid );
    if ( newlineidx < 0 )
    {
	uirv_.set( tr("%1 not available on %2")
		.arg( prov2D().dbKey().name() ).arg( gid.name() ) );
	return false;
    }
    const auto& l2dd = *prov2D().l2dds_.get( newlineidx );
    const auto newtrcidx = l2dd.indexOf( tnr );
    if ( newtrcidx < 0 )
    {
	uirv_.set( tr("No data for trace %1 on %2")
			.arg( tnr ).arg( gid.name() ) );
	return false;
    }

    prov2D().lineidx_ = newlineidx;
    prov2D().trcidx_ = newtrcidx;
    return true;
}


void Seis::Fetcher2D::getComponentInfo( BufferStringSet& nms,
					DataType& dt ) const
{
    ensureDataSet();

    nms.add( dataset_->name() );
    const FixedString dtyp = dataset_->dataType();
    if ( dtyp != sKey::Steering() )
	dt = Seis::UnknownData;
    else
    {
	nms.get(0).set( "Inline dip" );
	nms.add( "Crossline dip" );
	dt = Seis::Dip;
    }
}
