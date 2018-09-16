/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2009
________________________________________________________________________

-*/


#include "welltiegeocalculator.h"

#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "fourier.h"
#include "fftfilter.h"
#include "hilberttransform.h"
#include "linear.h"
#include "genericnumer.h"
#include "spectrogram.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "valseries.h"
#include "velocitycalc.h"
#include "welllog.h"
#include "welllogset.h"
#include "welldata.h"
#include "welltrack.h"
#include "wellinfo.h"
#include "welltieunitfactors.h"
#include "welld2tmodel.h"
#include "odcomplex.h"


namespace WellTie
{

#define	mLocalEps	1e-2f
Well::D2TModel* GeoCalculator::getModelFromVelLog( const Well::Data& wd,
						   const char* sonlog ) const
{
    ConstRefMan<Well::Log> log = wd.logs().getLogByName( sonlog );
    if ( !log )
	return 0;

    RefMan<Well::Log> proclog = new Well::Log( *log );
    if ( log->propType() == PropertyRef::Son )
	son2TWT( *proclog, wd );
    else
	vel2TWT( *proclog, wd );

    TypeSet<float> dahs, vals;
    Well::LogIter iter( *proclog );
    while ( iter.next() )
    {
	const float dah = iter.dah();
	if ( mIsUdf(dah) )
	    continue;

	const float twt = iter.value();
	if ( mIsUdf(twt) || twt < -1. * mLocalEps )
	    continue;

	dahs += dah;
	vals += twt;
    }
    iter.retire();

    Well::D2TModel* d2tnew = new Well::D2TModel;
    for ( int idx=0; idx<dahs.size(); idx++ )
	d2tnew->setValueAt( dahs[idx], vals[idx] );

    d2tnew->setName( "Integrated Depth/Time Model" );
    return d2tnew;
}


void GeoCalculator::son2Vel( Well::Log& log ) const
{
    const UnitOfMeasure* loguom = log.unitOfMeasure();
    bool issonic = false;
    bool isimperial = false;
    float fact = 1.f;
    if ( loguom )
    {
	isimperial = loguom->isImperial();
	if ( loguom->propType()==PropertyRef::Son )
	{
	    issonic = true;
	    if ( loguom->name().contains("Milli") )
		fact = 1e3f;
	    else if ( loguom->name().contains("Micro") )
		fact = 1e6f;
	}
    }

    if ( !issonic )
	fact = 1e6f;

    BufferString outuomlbl;
    outuomlbl = issonic ? getDistUnitString( isimperial, false ) : "us/";
    outuomlbl += issonic ? "/s" : getDistUnitString( isimperial, false );
    const UnitOfMeasure* outuom = UnitOfMeasure::getGuessed( outuomlbl );

    TypeSet<float> dahs, vals;
    Well::LogIter iter( log );
    while ( iter.next() )
    {
	float val = iter.value();
	if ( !mIsUdf(val) )
	    val = fact/val;
	dahs += iter.dah();
	vals += val;
    }
    iter.retire();
    log.setData( dahs, vals );

    if ( outuom )
	log.setUnitMeasLabel( outuomlbl );
}


void GeoCalculator::son2TWT( Well::Log& log, const Well::Data& wd ) const
{
    if ( log.propType() == PropertyRef::Son )
    {
	son2Vel( log );
	vel2TWT( log, wd );
    }
    else
    {
	vel2TWT( log, wd );
	son2Vel( log );
    }
}


void GeoCalculator::vel2TWT( Well::Log& log, const Well::Data& wd ) const
{
    int sz = log.size();
    if ( !sz )
       return;

    const UnitOfMeasure* loguom = log.unitOfMeasure();
    bool logisvel = loguom && loguom->propType() == PropertyRef::Vel;
    BufferString outuomlbl = logisvel ? "s"
				      : getDistUnitString( SI().depthsInFeet(),
							   false );
    if ( !logisvel )
	outuomlbl += "/s";
    const UnitOfMeasure* outuom = UnitOfMeasure::getGuessed( outuomlbl );

    const Well::Track& track = wd.track();

    const float srddepth = -1.f*mCast(float,SI().seismicReferenceDatum());
    const float srddah = track.getDahForTVD( srddepth );
    const float replveldz = -1.f * srddepth - track.getKbElev();
    const float startdah = replveldz < 0 ? srddah : track.firstDah();
    const float replvel = wd.info().replacementVelocity();
    const float bulkshift = replveldz > 0 ? 2.f * replveldz / replvel : 0.f;

    TypeSet<float> dahs, vals;
    dahs += replveldz < 0 ? srddepth : track.firstValue( true );
    vals += logisvel ? replvel : bulkshift;
    Well::LogIter iter( log );
    while ( iter.next() )
    {
	const float dah = iter.dah();
	const float logval = iter.value();
	if ( !mIsUdf(dah) && !mIsUdf(logval) && dah > startdah+mLocalEps )
	{
	    dahs += (float)track.getPos(dah).z_;
	    vals += loguom ? loguom->getSIValue( logval ) : logval;
	}
    }
    iter.retire();

    sz = dahs.size();
    if ( !sz )
	return;

    TypeSet<double> sdahs;
    TypeSet<double> svals;
    mGetIdxArr( int, idxs, sz );
    if ( !idxs) return;
    sort_coupled( dahs.arr(), idxs, sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	const int sidx = idxs[idx];
	const double newdepth = dahs[idx];
	const double newval = vals[sidx];
	const int cursz = sdahs.size();
	if ( cursz>1 && (mIsEqual(newdepth,sdahs[cursz-1],mLocalEps) ||
			 mIsEqual(newval,svals[cursz-1],mLocalEps)) )
	    continue;
	sdahs += dahs[idx];
	svals += vals[sidx];
    }

    sz = sdahs.size();
    if ( !sz )
	return;

    TypeSet<float> outvals( sz, mUdf(float) );
    if ( logisvel )
    {
	const ArrayValueSeries<double,double> sdahsvs( sdahs.arr(), false );
	ArrayValueSeries<float,float> inpvelsvs( sz );
	mAllocVarLenArr( double, outtimes, sz );
	if ( !inpvelsvs.isOK() || !mIsVarLenArrOK(outtimes) ) return;
	for ( int idx=0; idx<sz; idx++ )
	    inpvelsvs.setValue( idx, mCast(float,svals[idx]) );
	if ( !TimeDepthConverter::calcTimes(inpvelsvs,sz,sdahsvs,outtimes) )
	    return;

	const float startime = mCast(float,outtimes[0]);
	for ( int idx=0; idx<sz; idx++ )
	    outvals[idx] = mCast(float,outtimes[idx]) + bulkshift - startime;
    }
    else
    {
	TimeDepthModel verticaldtmod;
	if ( !verticaldtmod.setModel(sdahs.arr(),svals.arr(),sz).isOK() )
	    return;

	outvals += replvel;
	for ( int idx=1; idx<sz; idx++ )
	    outvals += verticaldtmod.getVelocity( sdahs.arr(), svals.arr(),
						 sz, mCast(float,sdahs[idx]) );
    }

    log.setEmpty();
    for ( int idx=0; idx<sz; idx++ )
    {
	const float outdah = track.getDahForTVD( sdahs[idx] );
	const float outval = outuom ? outuom->getUserValueFromSI(outvals[idx])
				    : outvals[idx];

	log.setValueAt( outdah, outval );
    }

    if ( outuom )
	log.setUnitMeasLabel( outuomlbl );
}


void GeoCalculator::removeSpikes( float* inp, int sz, int gate, int fac ) const
{
    if ( sz< 2 || sz < 2*gate ) return;
    float prevval = inp[0];
    for ( int idx=gate/2; idx<sz-gate; idx+=gate  )
    {
	float avg = 0;
	for ( int winidx = idx-gate/2; winidx<idx+gate/2; winidx++ )
	    avg += inp[winidx]/gate;
	for ( int winidx = idx-gate/2; winidx<idx+gate/2; winidx++ )
	{
	    if ( inp[winidx] > fac*avg )
		inp[winidx] = idx ? prevval : avg;
	    prevval = inp[winidx];
	}
    }
}


class DeconvolveData
{
public:

DeconvolveData( int sz )
    : fft_(*Fourier::CC::createDefault())
    , sz_(getPower2Size(sz))
    , halfsz_(sz_/2)
    , ctwtvals_(sz_)
    , cfreqvals_(sz_)
{
    const float_complex cnullval = float_complex( 0, 0 );
    ctwtvals_.setAll( cnullval );
    cfreqvals_.setAll( cnullval );
}


~DeconvolveData()
{
    delete &fft_;
}


int getPower2Size( int inpsz )
{
    int outsz = 1;
    while ( outsz < inpsz )
	outsz <<= 1;
    return outsz;
}


bool doFFT( bool isfwd )
{
    fft_.setInputInfo( Array1DInfoImpl(sz_) );
    fft_.setDir( isfwd );
    fft_.setNormalization( !isfwd );
    fft_.setInput(  ( isfwd ? ctwtvals_ : cfreqvals_).getData() );
    fft_.setOutput( (!isfwd ? ctwtvals_ : cfreqvals_).getData() );
    return fft_.run( isfwd );
}

    Fourier::CC&		fft_;
    const int			sz_;
    const int			halfsz_;
    Array1DImpl<float_complex>	ctwtvals_;
    Array1DImpl<float_complex>	cfreqvals_;

};


#define mNoise 0.01f
void GeoCalculator::deconvolve( const float* inp, const float_complex* filter,
			        float* deconvals, int inpsz ) const
{
    if ( !inp || !filter )
	return;

    ArrayNDWindow window( Array1DInfoImpl(inpsz), false, "CosTaper", 0.90 );

    Array1DImpl<float> inputvals( inpsz );
    OD::memCopy( inputvals.getData(), inp, inpsz*sizeof(float) );
    window.apply( &inputvals );
    ArrayMath::removeBias<float,double,double>( inputvals, false, true );

    Array1DImpl<float_complex> cfiltervals( inpsz );
    OD::memCopy( cfiltervals.getData(), filter, inpsz*sizeof(float_complex) );
    window.apply( &cfiltervals );
    ArrayMath::removeBias<float_complex,float_complex,float>( cfiltervals,
							      false, true );

    DeconvolveData dcinp( inpsz ), dcfilter( inpsz );
    const int cidx = mCast(int, inpsz/2 );
    const int firstidx = dcinp.halfsz_ - cidx;
    for ( int idx=0; idx<inpsz; idx++ )
    {
	const float_complex valinp( inputvals.get( idx ), 0. );
	const float_complex valfilt( cfiltervals.get( idx ) );
	dcinp.ctwtvals_.set( idx + firstidx, valinp );
	dcfilter.ctwtvals_.set( idx + firstidx, valfilt );
    }
    if ( !dcinp.doFFT(true) || !dcfilter.doFFT(true) )
	return;

    float_complex wholespec = 0.;
    for ( int idx=0; idx<dcfilter.sz_; idx++ )
	wholespec += std::norm( dcfilter.cfreqvals_.get( idx ) );
    const float_complex cnoiseshift = mNoise * wholespec / (float) dcfilter.sz_;

    DeconvolveData dcout( dcinp.sz_ );
    float summod = 0;
    for ( int idx=0; idx<dcout.sz_; idx++ )
    {
	const float_complex inputval = dcinp.cfreqvals_.get( idx );
	const float_complex filterval = dcfilter.cfreqvals_.get( idx );
	const float_complex conjfilterval = std::conj( filterval );
	const float_complex num = inputval * conjfilterval;
	const float_complex denom = std::norm( filterval ) + cnoiseshift;
	const float_complex res = num / denom;
	summod += abs( res );
	dcout.cfreqvals_.set( idx, res );
    }

    // one-forth of average + normalization by number of samples
    const float minfreqamp = summod / mCast( float, 4 * inpsz );
    const float_complex cnullval = float_complex( 0., 0. );
    for ( int idx=0; idx<dcout.sz_; idx++ )
    {
	if ( abs( dcout.cfreqvals_.get( idx ) ) < minfreqamp )
	    dcout.cfreqvals_.set( idx, cnullval );
    }
    if ( !dcout.doFFT(false) )
	return;

    for ( int idx=0; idx<inpsz; idx++ )
    {
	const int readidx = idx < cidx ? dcout.sz_-cidx+idx : idx-cidx;
	deconvals[idx] = dcout.ctwtvals_.get( readidx ).real();
    }
}


double GeoCalculator::crossCorr( const float* seis, const float* synth,
				 float* outp, int sz ) const
{
    genericCrossCorrelation( sz, 0, seis, sz, 0, synth, sz, -sz/2, outp );
    LinStats2D ls2d; ls2d.use( seis, synth, sz );
    return ls2d.corrcoeff;
}


void GeoCalculator::d2TModel2Log( const Well::D2TModel& d2t,
					Well::Log& log ) const
{
    log.setEmpty();
    MonitorLock ml( d2t );
    Well::D2TModelIter iter( d2t );
    while ( iter.next() )
	log.setValueAt( iter.dah(), iter.value() );
    iter.retire();

    const PropertyRef::StdType tp = PropertyRef::Time;
    log.setUnitMeasLabel( UoMR().getInternalFor(tp)->symbol() );
}

} // namespace WellTie
