/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seistrc.h"

#include "arraynd.h"
#include "iopar.h"
#include "seiscommon.h"
#include "simpnumer.h"
#include "unitofmeasure.h"
#include "veldescimpl.h"
#include "zvalseriesimpl.h"

#include <float.h>
#include <math.h>

const float SeisTrc::snapdist = Seis::cDefSampleSnapDist();

SeisTrc::SeisTrc( int ns, const DataCharacteristics& dc )
    : intpol_(nullptr)
{
    data_.addComponent(ns,dc);
}


SeisTrc::SeisTrc( const SeisTrc& oth )
    : intpol_(nullptr)
{
    *this = oth;
}


SeisTrc::~SeisTrc()
{
}


SeisTrc& SeisTrc::operator =( const SeisTrc& trc )
{
    if ( &trc == this ) return *this;

    while ( nrComponents() > trc.nrComponents() )
	data_.delComponent( 0 );

    info_ = trc.info_;
    copyDataFrom( trc, -1, false );

    intpol_ = 0;
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
    if ( !intpol_ )
    {
	ValueSeriesInterpolator<float>* newintpol =
				new ValueSeriesInterpolator<float>();
	newintpol->snapdist_ = snapdist;
	newintpol->smooth_ = true;
	newintpol->extrapol_ = false;
	newintpol->udfval_ = 0;

	intpol_.setIfNull(newintpol,true);
    }

    intpol_->maxidx_ = size()-1;

    return *intpol_;
}


void SeisTrc::setInterpolator( ValueSeriesInterpolator<float>* intpol )
{
    intpol_ = intpol;
}


bool SeisTrc::isNull( int icomp ) const
{
    if ( icomp >= nrComponents() )
	return true;

    Interval<int> comps( icomp, icomp );
    if ( icomp < 0 )
	comps.set( 0, nrComponents()-1 );

    for ( icomp=comps.start; icomp<=comps.stop; icomp++ )
    {
	if ( !data_.isZero(icomp) )
	    return false;
    }
    return true;
}


bool SeisTrc::isUdf( int icomp ) const
{
    if ( icomp >= nrComponents() )
	return true;

    Interval<int> comps( icomp, icomp );
    if ( icomp < 0 )
	comps.set( 0, nrComponents()-1 );

    const int sz = size();
    for ( icomp=comps.start; icomp<=comps.stop; icomp++ )
    {
	for ( int isamp=0; isamp<sz; isamp++ )
	{
	    const float val = get( isamp, icomp );
	    if ( !mIsUdf(val) )
		return false;
	}
    }
    return true;
}


bool SeisTrc::hasUndef( int icomp ) const
{
    if ( icomp >= nrComponents() )
	return true;

    Interval<int> comps( icomp, icomp );
    if ( icomp < 0 )
	comps.set( 0, nrComponents()-1 );

    const int sz = size();
    for ( icomp=comps.start; icomp<=comps.stop; icomp++ )
    {
	for ( int isamp=0; isamp<sz; isamp++ )
	{
	    const float val = get( isamp, icomp );
	    if ( mIsUdf(val) )
		return true;
	}
    }
    return false;
}


static bool findFirst( const SeisTrc& trc, int& sampnr, bool wantudf )
{
    const int nrsamps = trc.size();
    for ( int isamp=sampnr; isamp<nrsamps; isamp++ )
    {
	const float val = trc.get( isamp, 0 );
	const bool isudf = mIsUdf( val );
	if ( wantudf == isudf )
	    { sampnr = isamp; return true; }
    }
    return false;
}


inline static bool findFirstDefined( const SeisTrc& trc, int& sampnr )
{ return findFirst( trc, sampnr, false ); }
inline static bool findFirstUdf( const SeisTrc& trc, int& sampnr )
{ return findFirst( trc, sampnr, true ); }


void SeisTrc::ensureNoUndefs( float replval )
{
    const Interval<int> comps( 0, nrComponents()-1 );
    const int sz = size();
    const bool interpolate = mIsUdf(replval);

    for ( int icomp=comps.start; icomp<=comps.stop; icomp++ )
    {
	int cursamp = 0;
	if ( !findFirstUdf(*this,cursamp) )
	    continue; // no udfs, next component

	if ( cursamp == 0 ) // we start with one or more udf's
	{
	    if ( !findFirstDefined(*this,cursamp) )
	    {
		setAll( interpolate ? 0.f : replval, icomp );
		continue; // only udfs, next component
	    }

	    // extrapolate up to trace start
	    const float val2set = interpolate ? get(icomp,cursamp) : replval;
	    for ( int isamp=0; isamp<cursamp; isamp++ )
		set( isamp, val2set, icomp );

	    if ( !findFirstUdf(*this,cursamp) )
		continue; // no more udfs, next component
	}

	// interpolate
	bool componentfinished = false;
	while ( !componentfinished )
	{
	    Interval<int> udfsamps( cursamp, 0 );
	    if ( !findFirstDefined(*this,cursamp) )
		break; // only udfs until end
	    udfsamps.stop = cursamp - 1;

	    if ( !interpolate )
		for ( int isamp=udfsamps.start; isamp<=udfsamps.stop; isamp++ )
		    set( isamp, replval, icomp );
	    else
	    {
		const float v0 = get( udfsamps.start-1, icomp );
		const float v1 = get( udfsamps.stop+1, icomp );
		const int nrudfs = udfsamps.width() + 1;
		for ( int iudf=0; iudf<nrudfs; iudf++ )
		{
		    const float frac = (iudf+1.0f) / (nrudfs+1.0f);
		    const float newval = v0*(1.0f-frac) + v1*frac;
		    set( udfsamps.start+iudf, newval, icomp );
		}
	    }

	    if ( !findFirstUdf(*this,cursamp) )
		componentfinished = true; // no more udfs, next component
	}

	if ( !componentfinished )
	{
	    // extrapolate to trace end
	    const float val2set = interpolate ? get(icomp,cursamp-1) :replval;
	    for ( auto isamp=cursamp; isamp<sz; isamp++ )
		set( isamp, val2set, icomp );
	}
    }
}


float SeisTrc::getValue( float t, int icomp ) const
{
    const int sz = size();
    int sampidx = nearestSample( t );
    if ( sampidx < 0 || sampidx >= sz )
	return interpolator().udfval_;

    const float pos = info_.sampling.getfIndex( t );
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
    const float z0 = startPos() - snapdist * info_.sampling.step;
    const float z1 = endPos() + snapdist * info_.sampling.step;

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


void SeisTrc::convertToFPs( bool pres )
{
    data_.convertToFPs( pres );
}


bool SeisTrc::updateVelocities( const VelocityDesc& inpdesc,
		       const VelocityDesc& outdesc, const ZDomain::Info& zinfo,
		       double srd, const UnitOfMeasure* srduom,
		       int compnr, double t0 )
{
    if ( !inpdesc.isVelocity() || !outdesc.isVelocity() )
	return false;

    const int sz = size();
    const RegularZValues zvals( info().sampling, sz, zinfo );
    const Vel::Worker worker( inpdesc, srd, srduom );

    PtrMan<ValueSeries<double> > vels;
    for ( int icomp=0; icomp<nrComponents(); icomp++ )
    {
	if ( icomp == compnr || compnr < 0 )
	{
	    SeisTrcValueSeries trcvs( *this, icomp );
	    vels = ScaledValueSeries<double,float>::getFrom( trcvs );
	    if ( vels )
		worker.convertVelocities( *vels, zvals, outdesc, *vels, t0 );
	}
    }

    return true;
}


void SeisTrc::setAll( float val,int compnr )
{
// make MT
    const int sz = size();
    for ( int icomp=0; icomp<nrComponents(); icomp++ )
	if ( compnr == icomp || compnr < 0 )
	    for ( int isamp=0; isamp<sz; isamp++ )
		set( isamp, val, icomp );
}


void SeisTrc::setNrComponents( int newnrcomps,
			       DataCharacteristics::UserType fprep )
{
    const int oldnrcomps = nrComponents();
    const bool isautofprep = fprep == DataCharacteristics::Auto;
    if ( oldnrcomps == newnrcomps )
    {
	if ( isautofprep )
	    return;

	bool isok = true;
	for ( int icomp=0; icomp<oldnrcomps; icomp++ )
	    if ( data().getInterpreter(icomp)->dataChar().userType() != fprep )
		{ isok = false; break; }
	if ( isok )
	    return;
    }

    const int sz = size();
    if ( isautofprep )
	fprep = data().getInterpreter(0)->dataChar().userType();

    while ( nrComponents() > 0 )
	data().delComponent( 0 );

    for ( int icomp=0; icomp<newnrcomps; icomp++ )
	data().addComponent( sz, DataCharacteristics(fprep) );
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
	OD::memCopy( data().getComponent(icomp)->data(),
		trc.data().getComponent(icomp)->data(),
		sz * (int)dc.nrBytes() );
    }
}


void SeisTrc::reverse( int icomp )
{
     static const LinScaler revscale = LinScaler( 0., -1. );
     data_.scale( revscale, icomp );
}


SeisTrcValueSeries::SeisTrcValueSeries( const SeisTrc& t, int c )
    : trc_(const_cast<SeisTrc&>(t))
    , icomp_(c)
{}


SeisTrcValueSeries::~SeisTrcValueSeries()
{}


float SeisTrcValueSeries::value( od_int64 idx ) const
{ return trc_.get((int) idx,icomp_); }


void SeisTrcValueSeries::setValue( od_int64 idx,float v )
{ trc_.set((int) idx,v,icomp_); }


float* SeisTrcValueSeries::arr()
{
    float val;
    const DataCharacteristics dc( val );
    const TraceDataInterpreter* tdi = trc_.data().getInterpreter( icomp_ );
    if ( !tdi || tdi->dataChar() != dc )
	return nullptr;

    return (float*) trc_.data().getComponent( icomp_ )->data();
}


bool SeisTrcValueSeries::copytoArray( Array1D<float>& seistrcarr )
{
    const int trcsz = trc_.size();
    if ( seistrcarr.info().getSize(0) != trcsz )
	return false;

    if ( arr() )
    {
	void* srcptr = trc_.data().getComponent(icomp_)->data();
	OD::memCopy( seistrcarr.arr(), srcptr, trcsz*sizeof(float) );
	return true;
    }

    for ( int idx=0; idx<trcsz; idx++ )
	seistrcarr.set( idx, trc_.get(idx,icomp_) );

    return true;
}


const float* SeisTrcValueSeries::arr() const
{ return const_cast<SeisTrcValueSeries*>( this )->arr(); }


SeisTrcFunction::SeisTrcFunction(const SeisTrc& trc, int icomp)
    : trc_(trc)
    , icomp_(icomp)
{}


SeisTrcFunction::~SeisTrcFunction()
{}
