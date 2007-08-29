/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2006
-*/


static const char* rcsID = "$Id: attribdataholder.cc,v 1.9 2007-08-29 19:49:14 cvskris Exp $";

#include "attribdataholder.h"

#include "arraynd.h"
#include "attribdatacubes.h"
#include "cubesampling.h"
#include "genericnumer.h"
#include "seisinfo.h"

namespace Attrib
{


DataHolder::DataHolder( int z0, int nrsamples )
    : z0_(z0), nrsamples_(nrsamples)
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
    }

    return dh;
}


ValueSeries<float>* DataHolder::add( bool addnull )
{
    ValueSeries<float>* res = addnull ? 0
	: new ArrayValueSeries<float,float>( new float[nrsamples_], true );
    data_ += res;
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

    if ( arrptr )
    {
	for ( int idx=0; idx<trcinfoset_.size(); idx++ )
	{
	    const int offset = array.info().getOffset( 0,
		    trcrange.nearestIndex( trcinfoset_[idx]->nr),
		    dataset_[idx]->z0_-mNINT(cs.zrg.start/cs.zrg.step) );

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

	    memcpy( arrptr+offset, srcptr,
		    dataset_[idx]->nrsamples_*sizeof(float) );
	}
    }
    else
    {
	pErrMsg( "Not implemented" );
	return false;
    }

    return true;
}


}; // namespace Attrib
