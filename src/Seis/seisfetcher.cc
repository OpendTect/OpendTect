/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2016
________________________________________________________________________

-*/

#include "seisfetcher.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "prestackgather.h"
#include "seis2ddata.h"
#include "seisdatapack.h"
#include "seispreload.h"
#include "seisprovider.h"
#include "seistrctr.h"


void Seis::Fetcher::reset()
{
    uirv_.setOK();
    curlidx_ = -1;
    dp_ = nullptr;
}


RegularSeisDataPack& Seis::Fetcher::regSeisDP()
{ return *((RegularSeisDataPack*)dp_.ptr()); }
const RegularSeisDataPack& Seis::Fetcher::regSeisDP() const
{ return *((RegularSeisDataPack*)dp_.ptr()); }
GatherSetDataPack& Seis::Fetcher::gathDP()
{ return *((GatherSetDataPack*)dp_.ptr()); }
const GatherSetDataPack& Seis::Fetcher::gathDP() const
{ return *((GatherSetDataPack*)dp_.ptr()); }


void Seis::Fetcher::handleGeomIDChange( idx_type iln )
{
    curlidx_ = iln;
    provzsamp_ = prov_.zRange( curlidx_ );
    datachar_ = DataCharacteristics();

    const auto* trl = curTransl();
    if ( trl )
	datachar_ = trl->componentInfo().first()->datachar_;
    else
    {
	DataCharacteristics::UserType ut = OD::F32;
	const auto* ioobj = prov_.ioObj();
	if ( ioobj )
	    DataCharacteristics::getUserTypeFromPar( ioobj->pars(), ut );
	datachar_ = DataCharacteristics( ut );
    }

    dp_ = Seis::PLDM().getDP( prov_.dbKey(), prov_.geomID(curlidx_) );
    if ( dp_ )
    {
	const auto dpzsamp = isPS() ? gathDP().zRange() : regSeisDP().zRange();
	if ( !dpzsamp.includes(provzsamp_) )
	    dp_ = nullptr;
    }
}


void Seis::Fetcher::fillFromDP( const TrcKey& tk, SeisTrcInfo& ti,
				TraceData& td )
{
    const bool directfill = regSeisDP().isFullyCompat( provzsamp_, datachar_ );
    auto& filledti = directfill ? ti : worktrc_.info();
    auto& filledtd = directfill ? td : worktrc_.data();
    regSeisDP().fillTraceInfo( tk, filledti );
    regSeisDP().fillTraceData( tk, filledtd );
    if ( directfill )
	return;

    // our data needs to go from worktrc into ti and td
    const auto nrcomps = isPS() ? 1 : regSeisDP().nrComponents();
    const auto nrsamps = provzsamp_.nrSteps() + 1;
    td.setNrComponents( nrcomps, datachar_.userType() );
    td.convertTo( datachar_, false );
    td.reSize( nrsamps );
    for ( auto icomp=0; icomp<nrcomps; icomp++ )
    {
	for ( auto isamp=0; isamp<nrsamps; isamp++ )
	{
	    const auto z = provzsamp_.atIndex( isamp );
	    td.setValue( isamp, worktrc_.getValue(z,icomp), icomp );
	}
    }
    ti = filledti;
    ti.sampling_.start = provzsamp_.start;
    ti.sampling_.step = provzsamp_.step;
}


bool Seis::Fetcher3D::useDP( const BinID& bid ) const
{
    if ( !haveDP() )
	return false;
    if ( isPS() )
	return true; // for PS, we cannot predict, need to just try and see

    return regSeisDP().sampling().hsamp_.includes( bid );
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


bool Seis::Fetcher2D::useDP( const Bin2D& b2d ) const
{
    if ( !haveDP() )
	return false;
    if ( isPS() )
	return true; // for PS, we should just try and see

    return regSeisDP().sampling().hsamp_.includes( BinID(b2d.idxPair()) );
}


void Seis::Fetcher2D::getPossibleExtents()
{
    ensureDataSet();
    if ( !dataset_ )
	return;

    for ( auto idx=0; idx<dataset_->nrLines(); idx++ )
    {
	const auto geomid = dataset_->geomID( idx );

	PosInfo::Line2DData l2dd;
	dataset_->getGeometry( geomid, l2dd );
	auto* ld = new PosInfo::LineData( geomid.lineNr() );
	l2dd.getSegments( *ld );
	prov_.possiblepositions_.add( ld );

	const auto zsidx = prov_.allgeomids_.indexOf( geomid );
	prov_.allzsubsels_[zsidx].setOutputZRange( l2dd.zRange() );
    }
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
