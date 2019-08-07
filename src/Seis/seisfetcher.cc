/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2016
________________________________________________________________________

-*/

#include "seisfetcher.h"
#include "keystrs.h"
#include "prestackgather.h"
#include "seis2ddata.h"
#include "seisdatapack.h"
#include "seispreload.h"
#include "seisprovider.h"


void Seis::Fetcher::reset()
{
    uirv_.setOK();
    dplidx_ = -1;
    dp_ = nullptr;
}


RegularSeisDataPack& Seis::Fetcher::regSeisDP()
{ return *((RegularSeisDataPack*)dp_->ptr()); }
const RegularSeisDataPack& Seis::Fetcher::regSeisDP() const
{ return *((RegularSeisDataPack*)dp_->ptr()); }
GatherSetDataPack& Seis::Fetcher::gathDP()
{ return *((GatherSetDataPack*)dp_->ptr()); }
const GatherSetDataPack& Seis::Fetcher::gathDP() const
{ return *((GatherSetDataPack*)dp_->ptr()); }


void Seis::Fetcher::ensureDPIfAvailable( idx_type iln )
{
    if ( dplidx_ == iln )
	return;

    dplidx_ = iln;
    dp_ = Seis::PLDM().getDP( prov_.dbKey(), prov_.geomID(iln) );
    if ( !dp_ )
	return;

    const auto dpzsamp = isPS() ? gathDP().zRange() : regSeisDP().zRange();
    const auto provzsamp = prov_.zRange();
    if ( !dpzsamp.includes(provzsamp) )
	dp_ = nullptr;
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


void Seis::Fetcher2D::ensureDataSet() const
{
    if ( !dataset_ )
	mSelf().dataset_ = new Seis2DDataSet( *prov_.ioObj() );
}
