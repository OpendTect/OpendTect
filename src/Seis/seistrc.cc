/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace functions
-*/

static const char* rcsID = "$Id$";

#include "seistrc.h"
#include "simpnumer.h"
#include "iopar.h"
#include "errh.h"
#include "valseriesinterpol.h"
#include <math.h>
#include <float.h>

const float SeisTrc::snapdist = 1e-3;


SeisTrc::~SeisTrc()
{
    delete intpol_;
}


SeisTrc& SeisTrc::operator =( const SeisTrc& trc )
{
    if ( &trc == this ) return *this;

    while ( nrComponents() > trc.nrComponents() )
	data_.delComponent( 0 );

    info_ = trc.info_;
    copyDataFrom( trc, -1, false );

    delete intpol_; intpol_ = 0;
    if ( trc.intpol_ )
	intpol_ = new ValueSeriesInterpolator<float>( *trc.intpol_ );

    return *this;
}


bool SeisTrc::reSize( int sz, bool copydata )
{
    for ( int idx=0; idx<nrComponents(); idx++ )
	data_.reSize( sz, idx, copydata );
    return data_.allOk();
}


const ValueSeriesInterpolator<float>& SeisTrc::interpolator() const
{
    static ValueSeriesInterpolator<float>* defintpol = 0;
    if ( !defintpol )
    {
	defintpol = new ValueSeriesInterpolator<float>();
	defintpol->snapdist_ = snapdist;
	defintpol->smooth_ = true;
	defintpol->extrapol_ = false;
	defintpol->udfval_ = 0;
    }
    ValueSeriesInterpolator<float>& ret
	= const_cast<ValueSeriesInterpolator<float>&>(
	    				intpol_ ? *intpol_ : *defintpol );
    ret.maxidx_ = size() - 1;
    return ret;
}


void SeisTrc::setInterpolator( ValueSeriesInterpolator<float>* intpol )
{
    delete intpol_; intpol_ = intpol;
}


bool SeisTrc::isNull( int icomp ) const
{
    if ( icomp >= nrComponents() )
	return true;

    Interval<int> comps( icomp, icomp );
    if ( icomp < 0 )
	{ comps.start = 0; comps.stop = nrComponents()-1; }

    const int sz = size();
    for ( icomp=comps.start; icomp<=comps.stop; icomp++ )
    {
	for ( int isamp=0; isamp<sz; isamp++ )
	{
	    if ( get(isamp,icomp) )
		return false;
	}
    }
    return true;
}


float SeisTrc::getValue( float t, int icomp ) const
{
    const int sz = size();
    int sampidx = nearestSample( t );
    if ( sampidx < 0 || sampidx >= sz )
	return interpolator().udfval_;

    const float pos = ( t - startPos() ) / info_.sampling.step;
    if ( sampidx-pos > -snapdist && sampidx-pos < snapdist )
	return get( sampidx, icomp );

    return interpolator().value( SeisTrcValueSeries(*this,icomp), pos );
}


SampleGate SeisTrc::sampleGate( const Interval<float>& tg, bool check ) const
{
    SampleGate sg( info_.sampleGate(tg) );
    if ( !check ) return sg;

    const int maxsz = size() - 1;
    if ( sg.start > maxsz ) sg.start = maxsz;
    if ( sg.stop > maxsz ) sg.stop = maxsz;
    return sg;
}


SeisTrc* SeisTrc::getRelTrc( const ZGate& zgate, float sr ) const
{
    const float pick = info_.pick;
    if ( mIsUdf(pick) )
	return 0;

    ZGate zg( zgate ); zg.sort();
    SeisTrc* ret = new SeisTrc;
    ret->info_ = info_;
    ret->info_.sampling.start = zg.start;
    if ( mIsUdf(sr) ) sr = info_.sampling.step;
    ret->info_.sampling.step = sr;
    ret->info_.pick = 0;

    const int nrsamps = (int)( (zg.stop - zg.start) / sr + 1.5);
    ret->reSize( nrsamps, false );

    while ( ret->nrComponents() < nrComponents() )
	ret->data_.addComponent( ret->size(),
		data_.getInterpreter( ret->nrComponents() )->dataChar() );

    for ( int idx=0; idx<nrsamps; idx++ )
    {
	const float curt = pick + zg.start + sr * idx;
	for ( int icomp=0; icomp<nrComponents(); icomp++ )
	    ret->set( idx, getValue(curt,icomp), icomp );
    }

    return ret;
}


SeisTrc* SeisTrc::getExtendedTo( const ZGate& zgate, bool usevals ) const
{
    const float fnrsamps = (zgate.stop-zgate.start) / info_.sampling.step + 1;
    const int outnrsamps = mNINT32( fnrsamps );
    const TraceDataInterpreter* tdi = data_.getInterpreter(0);
    DataCharacteristics dc( tdi ? tdi->dataChar() : DataCharacteristics() );
    SeisTrc* newtrc = new SeisTrc( outnrsamps, dc );
    while ( newtrc->nrComponents() < nrComponents() )
	newtrc->data_.addComponent( newtrc->size(),
		data_.getInterpreter( newtrc->nrComponents() )->dataChar() );
    if ( size() < 1 )
	{ newtrc->zero(); return newtrc; }

    newtrc->info_ = info_;
    newtrc->info_.sampling.start = zgate.start;
    const float z0 = info_.sampling.start - snapdist * info_.sampling.step;
    const float z1 = samplePos( size() - 1 ) + snapdist * info_.sampling.step;

    for ( int icomp=0; icomp<nrComponents(); icomp++ )
    {
	const float preval = usevals ? get(0,icomp) : 0;
	const float postval = usevals ? get(size()-1,icomp) : 0;
	for ( int isamp=0; isamp<newtrc->size(); isamp++ )
	{
	    const float z = newtrc->samplePos( isamp );
	    const float val = (z < z0 ? preval : (z > z1 ? postval
			    : getValue( z, icomp ) ) );
	    newtrc->set( isamp, val, icomp );
	}
    }

    return newtrc;
}


bool SeisTrc::isWriteReady( const SamplingData<float>& sampling, int ns ) const
{
    if ( !mIsUdf(ns) && ns != size() )
	return false;

    if ( mIsUdf(sampling.start) )
    {
	// sampling start must be N * sample rate
	const float nz0 = info_.sampling.start / info_.sampling.step;
	const float intnz0 = (float)( mNINT32(nz0) );
	return mIsEqual( nz0, intnz0, 0.0001 );
    }

    // sampling must be an almost exact match
    const float dstart = info_.sampling.start - sampling.start;
    const float dstep = info_.sampling.step - sampling.step;
    return mIsZero(dstart,1e-6) && mIsZero(dstep,1e-8);
}


void SeisTrc::getWriteReady( SeisTrc& trc, SamplingData<float>& sampling,
			     int& ns ) const
{
    if ( mIsUdf(ns) )
	ns = size();
    if ( mIsUdf(sampling.start) )
    {
	sampling = info_.sampling;
	const float nz0 = sampling.start / sampling.step;
	const float newstart = sampling.step * mNINT32( nz0 );
	if ( !mIsEqual(sampling.start,newstart,1e-6) )
	    sampling.start = newstart;
    }

    trc = *this;
    trc.info_.sampling = sampling;
    trc.reSize( ns, false ); trc.zero();
    const int nrcomps = nrComponents();
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	for ( int isamp=0; isamp<ns; isamp++ )
	{
	    const float z = sampling.atIndex( isamp );
	    trc.set( isamp, getValue(z,icomp), icomp );
	}
    }
}


void SeisTrc::copyDataFrom( const SeisTrc& trc, int tarcomp, bool forcefloats )
{
    int startcomp = tarcomp < 0 ? 0 : tarcomp;
    int stopcomp = tarcomp < 0 ? trc.nrComponents()-1 : tarcomp;
    const int sz = trc.size();
    for ( int icomp=startcomp; icomp<=stopcomp; icomp++ )
    {
	DataCharacteristics dc = forcefloats
	    			? DataCharacteristics()
				: trc.data().getInterpreter(icomp)->dataChar();

	if ( nrComponents() <= icomp )
	    data().addComponent( sz, dc );
	else
	{
	    if ( data().getInterpreter(icomp)->dataChar() != dc )
		data().setComponent(dc,icomp);
	    if ( data_.size(icomp) != sz )
		data().getComponent(icomp)->reSize( sz );
	}
	memcpy( data().getComponent(icomp)->data(),
		trc.data().getComponent(icomp)->data(),
		sz * (int)dc.nrBytes() );
    }
}


float* SeisTrcValueSeries::arr()
{
    float val;
    static DataCharacteristics dc( val );
    const TraceDataInterpreter* tdi = trc_.data().getInterpreter( icomp_ );
    if ( !tdi ) return 0;

    if ( tdi->dataChar()!=dc )
	return 0;

    return (float*) trc_.data().getComponent( icomp_ )->data();
}


const float* SeisTrcValueSeries::arr() const
{ return const_cast<SeisTrcValueSeries*>( this )->arr(); }
