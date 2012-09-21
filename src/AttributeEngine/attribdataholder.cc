/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2006
-*/


static const char* rcsID mUnusedVar = "$Id$";

#include "attribdataholder.h"

#include "attribdataholderarray.h"
#include "arraynd.h"
#include "arrayndslice.h"
#include "attribdatacubes.h"
#include "cubesampling.h"
#include "genericnumer.h"
#include "interpol1d.h"
#include "seisinfo.h"
#include "simpnumer.h"
#include "survinfo.h"

namespace Attrib
{


DataHolder::DataHolder( int z0, int nrsamples )
    : z0_(z0), nrsamples_(nrsamples), extrazfromsamppos_(0)
{ data_.allowNull(true); }


DataHolder::~DataHolder()
{ deepErase(data_); }


DataHolder* DataHolder::clone() const
{
    DataHolder* dh = new DataHolder( z0_, nrsamples_ );
    dh->data_.allowNull(true);

    for ( int idx=0; idx<nrSeries(); idx++ )
    {
	if ( !data_[idx] )
	    dh->add( true );
	else if ( !data_[idx]->arr() )
	{
	    dh->add();
	    for ( int validx=0; validx<nrsamples_; validx++ )
		dh->data_[idx]->setValue( validx, data_[idx]->value(validx) );
	}
	else
	{
	    dh->add();
	    memcpy( dh->data_[idx]->arr(), data_[idx]->arr(),
		    nrsamples_*sizeof(float) );
	}
	dh->classstatus_ += classstatus_[idx];
    }

    dh->extrazfromsamppos_ = extrazfromsamppos_;
    return dh;
}


ValueSeries<float>* DataHolder::add( bool addnull )
{
    ValueSeries<float>* res = addnull ? 0
	: new ArrayValueSeries<float,float>( new float[nrsamples_], true );
    data_ += res;
    classstatus_ += -1;
    return res;
}


bool DataHolder::dataPresent( int samplenr ) const
{
    if ( samplenr >= z0_ && samplenr < (z0_ + nrsamples_) )
	return true;

    return false;
}


TypeSet<int> DataHolder::validSeriesIdx() const
{
    TypeSet<int> seriesidset;
    for( int idx=0; idx<nrSeries(); idx++ )
    {
	if ( data_[idx] )
	    seriesidset += idx;
    }

    return seriesidset;
}


void DataHolder::replace( int idx, ValueSeries<float>* valseries )
{
    ValueSeries<float>* ptr = data_.replace( idx, valseries );
    if ( ptr ) delete ptr;
}


float DataHolder::getValue( int serieidx, float exactz, float refstep ) const
{
    if ( !series( serieidx ) ) return mUdf(float);

    int lowz;
    float disttosamppos = getExtraZAndSampIdxFromExactZ( exactz, refstep, lowz);
    if ( mIsZero( disttosamppos - extrazfromsamppos_, 1e-6 ) )
    {
	if ( lowz<z0_ || lowz>=z0_+nrsamples_ ) return mUdf(float);
	return series( serieidx )->value( lowz-z0_ );
    }

    //Remark: do not use ValueSeriesInterpolator because of classstatus
    const Interval<int> datarg( z0_, z0_+nrsamples_-1 );
    const int highz = lowz + 1;
    float p0 = lowz-1 < z0_ ? mUdf(float) : series(serieidx)->value(lowz-1-z0_);
    float p1 = series(serieidx)->value(lowz-z0_);
    float p2 = series(serieidx)->value(highz-z0_);
    float p3 = !datarg.includes(highz+1,false) ? mUdf(float)
					 : series(serieidx)->value(highz+1-z0_);
    if ( classstatus_[serieidx] ==-1 || classstatus_[serieidx] == 1 )
    {
	TypeSet<float> tset;
	tset += p0; tset += p1; tset += p2; tset += p3;
	const_cast<DataHolder*>(this)->classstatus_[serieidx] =
	    			holdsClassValues( tset.arr(), 4 ) ? 1 : 0;
    }

    float val;
    if ( classstatus_[serieidx] == 0 )
    {
	float disttop1 = (disttosamppos - extrazfromsamppos_)/refstep;
	val = Interpolate::polyReg1DWithUdf( p0, p1, p2, p3, disttop1 );
    }
    else
	val = mNINT32( (exactz/refstep) )==lowz ? p1 : p2;

    return val;
}


float DataHolder::getExtraZFromSampPos( float exactz, float refzstep )
{
    if ( !(int)(refzstep*SI().zDomain().userFactor()) )
	return ((exactz/refzstep)-(int)(exactz/refzstep))*refzstep;

    //Workaround to avoid conversion problems, 1e7 to get 1e6 precision
    //in case the survey is in depth z*1e7 might exceed maximum int: we use 1e4
    const int fact = SI().zIsTime() ? (int)1e7 : (int)1e4;
    const int extrazem7 = (int)(exactz*fact)%(int)(refzstep*fact);
    const int extrazem7noprec = (int)(refzstep*fact) - 5;
    const int leftem3 = (int)(exactz*fact) - extrazem7;
    const int extrazem3 =
	(int)(leftem3*1e-3)%(int)(refzstep*SI().zDomain().userFactor());
    if ( extrazem7 <= extrazem7noprec || extrazem3 != 0 ) //below precision
	return (float) (extrazem3 * 1e-3 + extrazem7 * 
											(SI().zIsTime() ? 1e-7 : 1e-4));

    return 0;
}


float DataHolder::getExtraZAndSampIdxFromExactZ( float exactz,
						 float refzstep, int& idx )
{
    float extraz = getExtraZFromSampPos( exactz, refzstep );
    //Do not use mNINT32: we want to get previous sample
    //0.05 to deal with float precision pb
    int lowidx = (int)( (exactz/refzstep));
    int highidx = (int)( (exactz/refzstep)+0.05 );
    if ( lowidx != highidx )
	idx = extraz > 0.00005 ? lowidx : highidx;
    else
	idx = highidx;

    return extraz;
}


Data2DHolder::~Data2DHolder()
{
    deepErase( dataset_ );
    deepErase( trcinfoset_ );
}


CubeSampling Data2DHolder::getCubeSampling() const
{
    CubeSampling res;
    if ( trcinfoset_.isEmpty() )
	return res;

    StepInterval<int> trcrange;
    Interval<int> zrange;
    int prevtrcnr;
    float zstep;

    for ( int idx=0; idx<trcinfoset_.size(); idx++ )
    {
	const int curtrcnr = trcinfoset_[idx]->nr;
	const int start = dataset_[idx]->z0_;
	const int stop = dataset_[idx]->z0_ + dataset_[idx]->nrsamples_-1;

	if ( !idx )
	{
	    trcrange.start = trcrange.stop = curtrcnr;
	    zrange.start = start;
	    zrange.stop = stop;
	    zstep = trcinfoset_[idx]->sampling.step;
	}
	else
	{
	    const int step = curtrcnr - prevtrcnr;
	    if ( idx==1 )
		trcrange.step = step;
	    else if ( trcrange.step!=step )
		trcrange.step = greatestCommonDivisor( step, trcrange.step );

	    trcrange.include( curtrcnr );
	    zrange.include( start );
	    zrange.include( stop );
	}

	prevtrcnr = curtrcnr;
    }

    res.hrg.start = BinID( 0, trcrange.start );
    res.hrg.stop = BinID( 0, trcrange.stop );
    res.hrg.step = BinID( 1, trcrange.step );

    res.zrg.start = zrange.start*zstep;
    res.zrg.stop = zrange.stop*zstep;
    res.zrg.step = zstep;

    return res;
}



bool Data2DHolder::fillDataCube( DataCubes& res ) const
{
    if ( dataset_.isEmpty() )
	return false;

    const CubeSampling cs = getCubeSampling();
    const StepInterval<int> trcrange( cs.hrg.start.crl, cs.hrg.stop.crl,
				      cs.hrg.step.crl );
    res.setSizeAndPos( getCubeSampling() );
    if ( res.nrCubes() == 0 )
	res.addCube( mUdf(float) );
    else
	res.setValue( 0, mUdf(float) );

    Array3D<float>& array = res.getCube( 0 );
    float* arrptr = array.getData();

    for ( int idx=0; idx<trcinfoset_.size(); idx++ )
    {
	const float* srcptr = 0;
	const int nrseries = dataset_[idx]->nrSeries();
	for ( int idy=0; idy<nrseries; idy++ )
	{
	    if ( dataset_[idx]->series(idy) )
	    {
		srcptr = dataset_[idx]->series(idy)->arr();
		break;
	    }
	}
	if ( !srcptr )
	    continue;

	const int trcidx = trcrange.nearestIndex( trcinfoset_[idx]->nr );
	const int zpos = dataset_[idx]->z0_ - mNINT32(cs.zrg.start/cs.zrg.step);
	if ( arrptr )
	{
	    const od_int64 offset = array.info().getOffset( 0, trcidx, zpos );
	    memcpy( arrptr+offset, srcptr,
		    dataset_[idx]->nrsamples_*sizeof(float) );
	}
	else
	{
	    const int ns = dataset_[idx]->nrsamples_;
	    for ( int isamp=0; isamp<ns; isamp++ )
		array.set( 0, trcidx, isamp+zpos, srcptr[isamp] );
	}
    }

    return true;
}


int Data2DHolder::getDataHolderIndex( int trcno ) const
{
    if ( trcinfoset_.isEmpty() )
	return -1;

    const int guessedidx = trcno-trcinfoset_[0]->nr;
    if ( trcinfoset_.validIdx(guessedidx) &&
		trcno == trcinfoset_[guessedidx]->nr )
	return guessedidx;

    for ( int idx=0; idx<trcinfoset_.size(); idx++ )
    {
	if ( trcno == trcinfoset_[idx]->nr )
	    return idx;
    }

    return -1;
}


Data2DArray::Data2DArray( const Data2DHolder& dh )
    : dataset_( 0 )
{
    dh.ref();

    DataHolderArray array3d( dh.dataset_, false );
    mTryAlloc( dataset_, Array3DImpl<float>( array3d ) );
    if ( !dataset_ || !dataset_->isOK() )
    {
	delete dataset_;
	dataset_ = 0;
    }

    for ( int idx=0; idx<dh.trcinfoset_.size(); idx++ )
    {
	SeisTrcInfo* ni = new SeisTrcInfo( *dh.trcinfoset_[idx] );
	ni->sampling.start = dh.dataset_[idx]->z0_ * ni->sampling.step;
	trcinfoset_ += ni;
    }

    cubesampling_ = dh.getCubeSampling();

    dh.unRef();
}


Data2DArray::~Data2DArray()
{
    delete dataset_;
}


bool Data2DArray::isOK() const
{ return dataset_ && dataset_->isOK(); }


int Data2DArray::indexOf( int trcnr ) const
{
    if ( trcinfoset_.isEmpty() )
	return -1;

    const int guessedidx = trcnr-trcinfoset_[0]->nr;
    if ( trcinfoset_.validIdx(guessedidx) &&
	 trcnr == trcinfoset_[guessedidx]->nr )
	return guessedidx;

    for ( int idx=0; idx<trcinfoset_.size(); idx++ )
    {
	if ( trcnr == trcinfoset_[idx]->nr )
	    return idx;
    }

    return -1;
}


int Data2DArray::nrTraces() const
{ return dataset_->info().getSize(0); }


} // namespace Attrib
