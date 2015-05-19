/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		February 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: seisdatapack.cc 38551 2015-03-18 05:38:02Z mahant.mothey@dgbes.com $";

#include "seisdatapack.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "bufstringset.h"
#include "flatposdata.h"
#include "seisinfo.h"
#include "survinfo.h"

#include <limits.h>


// RegularSeisDataPack
RegularSeisDataPack::RegularSeisDataPack( const char* cat,
					  const BinDataDesc* bdd )
    : SeisDataPack(cat,bdd)
{ sampling_.init( false ); }

TrcKey RegularSeisDataPack::getTrcKey( int globaltrcidx ) const
{ return sampling_.hsamp_.trcKeyAt( globaltrcidx ); }

bool RegularSeisDataPack::is2D() const
{ return sampling_.hsamp_.survid_ == Survey::GM().get2DSurvID(); }

int RegularSeisDataPack::getGlobalIdx( const TrcKey& tk ) const
{
    const int ret = mCast(int,sampling_.hsamp_.globalIdx(tk));
    return ret < nrTrcs() ? ret : -1;
}


bool RegularSeisDataPack::addComponent( const char* nm )
{
    if ( !sampling_.isDefined() || sampling_.hsamp_.totalNr()>INT_MAX )
	return false;

    if ( !addArray(sampling_.nrLines(),sampling_.nrTrcs(),sampling_.nrZ()) )
	return false;

    componentnames_.add( nm );
    return true;
}


void RegularSeisDataPack::dumpInfo( IOPar& par ) const
{
    DataPack::dumpInfo( par );

    const TrcKeySampling& tks = sampling_.hsamp_;
    // TODO: Change for 2D
    par.set( sKey::InlRange(), tks.start_.lineNr(), tks.stop_.lineNr(),
			       tks.step_.lineNr() );
    par.set( sKey::CrlRange(), tks.start_.trcNr(), tks.stop_.trcNr(),
			       tks.step_.trcNr() );
    par.set( sKey::ZRange(), sampling_.zsamp_.start, sampling_.zsamp_.stop,
			     sampling_.zsamp_.step );
}


// RandomSeisDataPack
RandomSeisDataPack::RandomSeisDataPack( const char* cat,
					const BinDataDesc* bdd )
    : SeisDataPack(cat,bdd)
{
}


TrcKey RandomSeisDataPack::getTrcKey( int trcidx ) const
{
    return path_.validIdx(trcidx) ? path_[trcidx] : TrcKey::udf();
}


bool RandomSeisDataPack::addComponent( const char* nm )
{
    if ( path_.isEmpty() || zsamp_.isUdf() )
	return false;

    if ( !addArray(1,nrTrcs(),zsamp_.nrSteps()+1) )
	return false;

    componentnames_.add( nm );
    return true;
}


int RandomSeisDataPack::getGlobalIdx( const TrcKey& tk ) const
{ return path_.indexOf( tk ); }



// SeisFlatDataPack
SeisFlatDataPack::SeisFlatDataPack( const SeisDataPack& source,int comp)
    : FlatDataPack("")
    , source_(source)
    , comp_(comp)
{
    DPM(DataPackMgr::SeisID()).obtain( source_.id() );
    setName( source_.getComponentName(comp_) );
}


SeisFlatDataPack::~SeisFlatDataPack()
{
    DPM(DataPackMgr::SeisID()).release( source_.id() );
}


bool SeisFlatDataPack::isAltDim0InInt( const char* keystr ) const
{
    const FixedString key( keystr );
    return key == SeisTrcInfo::getFldString(SeisTrcInfo::BinIDInl) ||
	   key == SeisTrcInfo::getFldString(SeisTrcInfo::BinIDCrl) ||
	   key == SeisTrcInfo::getFldString(SeisTrcInfo::TrcNr);
}


void SeisFlatDataPack::getAuxInfo( int i0, int i1, IOPar& iop ) const
{
    const Coord3 crd = getCoord( i0, i1 );
    iop.set( "X-coordinate", crd.x );
    iop.set( "Y-coordinate", crd.y );
    iop.set( "Z-Coord", crd.z*SI().zDomain().userFactor() );

    if ( is2D() )
	iop.set( sKey::TraceNr(), getTrcKey(i0).trcNr() );
    else
    {
	const BinID bid = SI().transform( crd );
	iop.set( "Inline", bid.inl() );
	iop.set( "Crossline", bid.crl() );
    }
}


float SeisFlatDataPack::nrKBytes() const
{
    return source_.nrKBytes() / source_.nrComponents();
}



RegularFlatDataPack::RegularFlatDataPack(
		const RegularSeisDataPack& source, int comp )
    : SeisFlatDataPack(source,comp)
    , sampling_(source.sampling())
    , dir_(sampling_.defaultDir())
{
    setSourceData();
}


bool RegularFlatDataPack::is2D() const
{
    mDynamicCastGet(const RegularSeisDataPack*,regsdp,&source_);
    return regsdp && regsdp->is2D();
}


Coord3 RegularFlatDataPack::getCoord( int i0, int i1 ) const
{
    const bool isvertical = dir_ != TrcKeyZSampling::Z;
    const int trcidx = isvertical ? i0 : i0*sampling_.nrTrcs()+i1;
    const Coord c = Survey::GM().toCoord( getTrcKey(trcidx) );
    return Coord3( c.x, c.y, sampling_.zsamp_.atIndex(isvertical ? i1 : 0) );
}


void RegularFlatDataPack::getAltDim0Keys( BufferStringSet& keys ) const
{
    if ( is2D() )
	keys.add( SeisTrcInfo::getFldString(SeisTrcInfo::TrcNr) );
    else
    {
	if ( dir_ == TrcKeyZSampling::Crl )
	    keys.add( SeisTrcInfo::getFldString(SeisTrcInfo::BinIDInl) );
	else if ( dir_ == TrcKeyZSampling::Inl )
	    keys.add( SeisTrcInfo::getFldString(SeisTrcInfo::BinIDCrl) );
    }

    keys.add( SeisTrcInfo::getFldString(SeisTrcInfo::CoordX) );
    keys.add( SeisTrcInfo::getFldString(SeisTrcInfo::CoordY) );
}


#define mStepIntvD( rg ) \
    StepInterval<double>( rg.start, rg.stop, rg.step )

void RegularFlatDataPack::setSourceData()
{
    const bool isz = dir_==TrcKeyZSampling::Z;
    if ( !isz )
    {
	for ( int idx=0; idx<source_.nrTrcs(); idx++ )
	    path_ += source_.getTrcKey( idx );
    }

    if ( !is2D() )
    {
	const bool isinl = dir_==TrcKeyZSampling::Inl;
	posdata_.setRange( true, isinl ? mStepIntvD(sampling_.hsamp_.crlRange())
				: mStepIntvD(sampling_.hsamp_.inlRange()) );
	posdata_.setRange( false, isz ? mStepIntvD(sampling_.hsamp_.crlRange())
				      : mStepIntvD(sampling_.zsamp_) );
    }
    else
    {
	const int nrtrcs = sampling_.nrTrcs();
	float* pos = new float[nrtrcs];
	pos[0] = 0;

	TrcKey prevtk = source_.getTrcKey( 0 );
	for ( int idx=1; idx<nrtrcs; idx++ )
	{
	    const TrcKey trckey = source_.getTrcKey( idx );
	    if ( trckey.isUdf() )
		pos[idx] = mCast(float,(pos[idx-1]));
	    else
	    {
		pos[idx] = mCast(float,(pos[idx-1] + prevtk.distTo(trckey)));
		prevtk = trckey;
	    }
	}

	posData().setX1Pos( pos, nrtrcs, 0 );
	posData().setRange( false, mStepIntvD(sampling_.zsamp_) );
    }

    const int dim0 = dir_==TrcKeyZSampling::Inl ? 1 : 0;
    const int dim1 = dir_==TrcKeyZSampling::Z ? 1 : 2;
    Array2DSlice<float>* slice2d = new Array2DSlice<float>(source_.data(comp_));
    slice2d->setDimMap( 0, dim0 );
    slice2d->setDimMap( 1, dim1 );
    slice2d->setPos( dir_, 0 );
    slice2d->init();
    arr2d_ = slice2d;
}



RandomFlatDataPack::RandomFlatDataPack(
		const RandomSeisDataPack& source, int comp )
    : SeisFlatDataPack(source,comp)
    , path_(source.getPath())
    , zsamp_(source.getZRange())
{
    setSourceData();
}


Coord3 RandomFlatDataPack::getCoord( int i0, int i1 ) const
{
    const Coord coord = path_.validIdx(i0) ? Survey::GM().toCoord(path_[i0])
					   : Coord::udf();
    return Coord3( coord, zsamp_.atIndex(i1) );
}


void RandomFlatDataPack::setSourceData()
{
    const int nrtrcs = path_.size();
    float* pos = new float[nrtrcs];
    pos[0] = 0;

    TrcKey prevtk = path_[0];
    for ( int idx=1; idx<nrtrcs; idx++ )
    {
	const TrcKey& trckey = path_[idx];
	if ( trckey.isUdf() )
	    pos[idx] = mCast(float,(pos[idx-1]));
	else
	{
	    pos[idx] = mCast(float,(pos[idx-1] + prevtk.distTo(trckey)));
	    prevtk = trckey;
	}
    }

    posData().setX1Pos( pos, nrtrcs, 0 );
    posData().setRange( false, mStepIntvD(zsamp_) );

    Array2DSlice<float>* slice2d = new Array2DSlice<float>(source_.data(comp_));
    slice2d->setDimMap( 0, 1 );
    slice2d->setDimMap( 1, 2 );
    slice2d->setPos( 0, 0 );
    slice2d->init();
    arr2d_ = slice2d;
}

