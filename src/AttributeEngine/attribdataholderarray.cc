/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribdataholderarray.h"
#include "attribdataholder.h"
#include "seisinfo.h"

namespace Attrib
{

DataHolderArray::DataHolderArray( const ObjectSet<DataHolder>& dh )
    : dh_(dh)
    , type_(0)
    , seriesidx_(-1)
{
    const int nrdh = dh_.size();
    int nrseries = 0;
    int nrsamples = 0;
    if ( nrdh>0 )
    {
	TypeSet<int> valididxs = dh_[0]->validSeriesIdx();
	nrseries = valididxs.size();
	nrsamples = dh_[0]->nrsamples_;
    }

    info_.setSize( 0, nrseries );
    info_.setSize( 1, nrdh );
    info_.setSize( 2, nrsamples );
}


DataHolderArray::DataHolderArray( const ObjectSet<DataHolder>& dh, int sidx,
				  int dim0sz, int dim1sz )
    : dh_(dh)
    , seriesidx_(sidx)
    , type_(1)
{
    info_.setSize( 0, dim0sz );
    info_.setSize( 1, dim1sz );
    info_.setSize( 2, !dh_.isEmpty() ? dh_[0]->nrsamples_ : 0 );
}


DataHolderArray::~DataHolderArray()
{
}


void DataHolderArray::set( int i0, int i1, int i2, float val )
{
    if ( type_==0 )
    {
	TypeSet<int> valididxs = dh_[0]->validSeriesIdx();
	const int sidx = valididxs[i0];
	ValueSeries<float>* vals = dh_[i1]->series( sidx );
	if ( vals )
	    vals->setValue( i2, val );
    }
    else
    {
	const int idx = (i0*info_.getSize(1)) + i1;
	ValueSeries<float>* vals = dh_[idx]->series( seriesidx_ );
	if ( vals )
	    vals->setValue( i2, val );
    }
}


float DataHolderArray::get( int i0, int i1, int i2 ) const
{
    if ( i0<0 || i1<0 || i2<0 ) return mUdf(float);

    if ( type_==0 )
    {
	TypeSet<int> valididxs = dh_[0]->validSeriesIdx();
	const int sidx = valididxs[i0];
	const ValueSeries<float>* valseries = dh_[i1]->series( sidx );
	return valseries ? valseries->value( i2 ) : mUdf(float);
    }
    else
    {
	const int idx = (i0*info_.getSize(1)) + i1;
	const ValueSeries<float>* vals = dh_[idx]->series( seriesidx_ );
	return vals ? vals->value(i2) : mUdf(float);
    }
}


void DataHolderArray::getAll( float* ptr ) const
{
    if ( !ptr )
	return;

    if ( type_==0 )
    {
	const int nrseries = info_.getSize( 0 );
	const int nrdataholders = info_.getSize( 1 );
	const int nrsamples = info_.getSize( 2 );
	TypeSet<int> valididxs = dh_[0]->validSeriesIdx();

	for ( int seridx=0; seridx<nrseries; seridx++ )
	{
	    const int sidx = valididxs[seridx];
	    for ( int dhidx=0; dhidx<nrdataholders; dhidx++ )
	    {
		const ValueSeries<float>* vs = dh_[dhidx]->series( sidx );
		const float* srcptr = vs ? vs->arr() : 0;
		if ( srcptr )
		    OD::sysMemCopy( ptr, srcptr, nrsamples*sizeof(float) );
		else if ( vs )
		    vs->getValues( ptr, nrsamples );

		ptr += nrsamples;
	    }
	}
    }
    else
    {
	const int sz0 = info_.getSize( 0 );
	const int sz1 = info_.getSize( 1 );
	const int sz2 = info_.getSize( 2 );
	for ( int i0=0; i0<sz0; i0++ )
	{
	    for ( int i1=0; i1<sz1; i1++ )
	    {
		const int dhidx = (i0*sz1) + i1;
		const ValueSeries<float>* vs = dh_[dhidx]->series( seriesidx_ );
		const float* srcptr = vs ? vs->arr() : 0;
		if ( srcptr )
		    OD::sysMemCopy( ptr, srcptr, sz2*sizeof(float) );
		else if ( vs )
		    vs->getValues( ptr, sz2 );

		ptr += sz2;
	    }
	}
    }
}


void DataHolderArray::getAll( ValueSeries<float>& outvs ) const
{
    float* outptr = outvs.arr();
    if ( outptr )
    {
	getAll( outptr );
	return;
    }

    od_int64 outidx = 0;
    if( type_==0 )
    {
	const int nrseries = info_.getSize( 0 );
	const int nrdataholders = info_.getSize( 1 );
	const int nrsamples = info_.getSize( 2 );
	TypeSet<int> valididxs = dh_[0]->validSeriesIdx();

	for( int seridx = 0; seridx<nrseries; seridx++ )
	{
	    const int sidx = valididxs[seridx];
	    for( int dhidx = 0; dhidx<nrdataholders; dhidx++ )
	    {
		const ValueSeries<float>* srcvs = dh_[dhidx]->series( sidx );
		const float* srcptr = srcvs ? srcvs->arr() : 0;
		if( srcptr )
		{
		    for ( int idx=0; idx<nrsamples; idx++ )
		    {
			outvs.setValue( outidx, srcptr[idx] );
			outidx++;
		    }
		}
		else if ( srcvs )
		{
		    for( int idx = 0; idx<nrsamples; idx++)
		    {
			outvs.setValue( outidx, srcvs->value(idx) );
			outidx++;
		    }
		}
		else
		{
		    for( int idx = 0; idx<nrsamples; idx++ )
		    {
			outvs.setValue(outidx, mUdf(float) );
			outidx++;
		    }
		}
	    }
	}
    }
    else
    {
	const int sz0 = info_.getSize( 0 );
	const int sz1 = info_.getSize( 1 );
	const int sz2 = info_.getSize( 2 );
	for( int i0 = 0; i0<sz0; i0++ )
	{
	    for( int i1 = 0; i1<sz1; i1++ )
	    {
		const int dhidx = ( i0*sz1 ) + i1;
		const ValueSeries<float>* srcvs = 
		    dh_[dhidx]->series( seriesidx_ );
		const float* srcptr = srcvs ? srcvs->arr() : 0;

		if( srcptr )
		{
		    for( int idx = 0; idx<sz2; idx++ )
		    {
			outvs.setValue( outidx,srcptr[idx] );
			outidx++;
		    }
		}
		else if( srcvs )
		{
		    for( int idx = 0; idx<sz2; idx++ )
		    {
			outvs.setValue( outidx,srcvs->value(idx) );
			outidx++;
		    }
		}
		else
		{
		    for( int idx = 0; idx<sz2; idx++ )
		    {
			outvs.setValue( outidx,mUdf(float) );
			outidx++;
		    }
		}
	    }
	}
    }
}
 
} // namespace Attrib
