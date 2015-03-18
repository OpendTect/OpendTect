/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		February 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "seisdatapack.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "bufstringset.h"
#include "convmemvalseries.h"
#include "flatposdata.h"
#include "survinfo.h"

#include <limits.h>


// SeisDataPack
SeisDataPack::SeisDataPack( const char* cat, const BinDataDesc* bdd )
    : DataPack(cat)
    , zdomaininfo_(new ZDomain::Info(ZDomain::SI()))
    , desc_( bdd ? *bdd : BinDataDesc(false,true,sizeof(float)) )
{}


SeisDataPack::~SeisDataPack()
{
    deepErase( arrays_ );
    deleteAndZeroPtr( zdomaininfo_ );
}


const OffsetValueSeries<float>
SeisDataPack::getTrcStorage(int comp, int globaltrcidx) const
{
    const Array3DImpl<float>* array = arrays_[comp];
    return OffsetValueSeries<float>( *array->getStorage(),
				globaltrcidx * array->info().getSize(2) );
}


const float* SeisDataPack::getTrcData( int comp, int globaltrcidx ) const
{
    const Array3DImpl<float>* array = arrays_[comp];
    return array->getData() + globaltrcidx * array->info().getSize(2);
}


const char* SeisDataPack::getComponentName( int component ) const
{
    return componentnames_.validIdx(component)
	? componentnames_[component]->buf() : 0;
}


void SeisDataPack::setZDomain( const ZDomain::Info& zinf )
{
    delete zdomaininfo_;
    zdomaininfo_ = new ZDomain::Info( zinf );
}


float SeisDataPack::nrKBytes() const
{
    const int nrcomps = nrComponents();
    if ( nrcomps == 0 ) return 0.0f;
    return nrcomps * arrays_[0]->info().getTotalSz() * desc_.nrBytes() / 1024.f;
}


bool SeisDataPack::addArray( int sz0, int sz1, int sz2 )
{
    float dummy; const BinDataDesc floatdesc( dummy );
    Array3DImpl<float>* arr = 0;
    if ( desc_ == floatdesc )
    {
	arr = new Array3DImpl<float>( sz0, sz1, sz2 );
	if ( !arr->isOK() )
	{
	    delete arr;
	    return false;
	}
    }
    else
    {
	arr = new Array3DImpl<float>( 0, 0, 0 );
	ConvMemValueSeries<float>* stor =
		new ConvMemValueSeries<float>( 0, desc_ );
	arr->setStorage( stor );
	arr->setSize( sz0, sz1, sz2 );
	if ( !stor->storArr() )
	{
	    delete arr;
	    return false;
	}
    }

    arr->setAll( mUdf(float) );
    arrays_ += arr;
    return true;
}


const Array3DImpl<float>& SeisDataPack::data( int component ) const
{ return *arrays_[component]; }

Array3DImpl<float>& SeisDataPack::data( int component )
{ return *arrays_[component]; }


// RegularSeisDataPack
RegularSeisDataPack::RegularSeisDataPack( const char* cat,
					  const BinDataDesc* bdd )
    : SeisDataPack(cat,bdd)
{
    sampling_.init( false );
}


TrcKey RegularSeisDataPack::getTrcKey( int globaltrcidx ) const
{
    return sampling_.hsamp_.trcKeyAt( globaltrcidx );
}


bool RegularSeisDataPack::is2D() const
{
    return sampling_.hsamp_.survid_ == Survey::GM().get2DSurvID();
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



RegularFlatDataPack::RegularFlatDataPack(
		const RegularSeisDataPack& source, int comp )
    : FlatDataPack("")
    , source_(source)
    , comp_(comp)
    , sampling_(source.sampling())
    , dir_(sampling_.defaultDir())
{
    DPM(DataPackMgr::SeisID()).obtain( source_.id() );
    setName( source_.getComponentName(comp_) );
    setSourceData();
}


RegularFlatDataPack::~RegularFlatDataPack()
{
    DPM(DataPackMgr::SeisID()).release( source_.id() );
}


Coord3 RegularFlatDataPack::getCoord( int i0, int i1 ) const
{
    const bool isvertical = dir_ != TrcKeyZSampling::Z;
    const int trcidx = isvertical ? i0 : i0*sampling_.nrTrcs()+i1;
    const Coord c = Survey::GM().toCoord( getTrcKey(trcidx) );
    return Coord3( c.x, c.y, sampling_.zsamp_.atIndex(isvertical ? i1 : 0) );
}


float RegularFlatDataPack::nrKBytes() const
{
    return source_.nrKBytes() / source_.nrComponents();
}


#define mStepIntvD( rg ) \
    StepInterval<double>( rg.start, rg.stop, rg.step )

void RegularFlatDataPack::setSourceData()
{
    if ( !is2D() )
    {
	const bool isinl = dir_==TrcKeyZSampling::Inl;
	const bool isz = dir_==TrcKeyZSampling::Z;
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
    : FlatDataPack("")
    , source_(source)
    , comp_(comp)
    , path_(source.getPath())
    , zsamp_(source.getZRange())
{
    DPM(DataPackMgr::SeisID()).obtain( source_.id() );
    setName( source_.getComponentName(comp_) );
    setSourceData();
}


RandomFlatDataPack::~RandomFlatDataPack()
{
    DPM(DataPackMgr::SeisID()).release( source_.id() );
}


Coord3 RandomFlatDataPack::getCoord( int i0, int i1 ) const
{
    const Coord coord = path_.validIdx(i0) ? Survey::GM().toCoord(path_[i0])
					   : Coord::udf();
    return Coord3( coord, zsamp_.atIndex(i1) );
}


float RandomFlatDataPack::nrKBytes() const
{
    return source_.nrKBytes() / source_.nrComponents();
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

