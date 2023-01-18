/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "welltiegeocalculator.h"

#include "arrayndalgo.h"
#include "fourier.h"
#include "fftfilter.h"
#include "hilberttransform.h"
#include "linear.h"
#include "genericnumer.h"
#include "spectrogram.h"
#include "survinfo.h"
#include "timedepthmodel.h"
#include "unitofmeasure.h"
#include "valseries.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "welllogset.h"
#include "welltrack.h"


#define	mLocalEps	1e-2f

Well::D2TModel* WellTie::GeoCalculator::getModelFromVelLog(
				const Well::Data& wd, const char* sonlog )
{
    const Well::Log* log = wd.logs().getLog( sonlog );
    if ( !log ) return nullptr;

    Well::Log proclog( *log );
    if ( log->propType() == Mnemonic::Son )
	son2TWT( proclog, wd );
    else
	vel2TWT( proclog, wd );

    TypeSet<float> dpt, vals;
    for ( int idx=0; idx<proclog.size(); idx++ )
    {
	const float dah = proclog.dah( idx );
	if ( mIsUdf(dah) )
	    continue;

	const float twt = proclog.value( idx );
	if ( mIsUdf(twt) || twt < -1. * mLocalEps )
	    continue;

	dpt += dah;
	vals += twt;
    }

    auto* d2tnew = new Well::D2TModel;
    for ( int idx=0; idx<dpt.size(); idx++ )
	d2tnew->add( dpt[idx], vals[idx] );

    d2tnew->setName( "Integrated Depth/Time Model");
    return d2tnew;
}


void WellTie::GeoCalculator::son2Vel( Well::Log& log )
{
    const UnitOfMeasure* loguom = log.unitOfMeasure();
    bool issonic = false;
    bool isimperial = false;
    float fact = 1.f;
    if ( loguom )
    {
	isimperial = loguom->isImperial();
	if ( loguom->propType()==Mnemonic::Son )
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

    for ( int idx=0; idx<log.size(); idx++ )
    {
	float& val = log.valArr()[idx];
	if ( !mIsUdf(val) )
	    val = fact/val;
    }

    if ( outuom )
    {
	log.setUnitMeasLabel( outuomlbl );
	log.setMnemonicLabel( nullptr, true );
    }
}


void WellTie::GeoCalculator::son2TWT( Well::Log& log, const Well::Data& wd )
{
    if ( log.propType() == Mnemonic::Son )
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


void WellTie::GeoCalculator::vel2TWT( Well::Log& log, const Well::Data& wd )
{
    int sz = log.size();
    if ( !sz )
       return;

    const UnitOfMeasure* loguom = log.unitOfMeasure();
    bool logisvel = loguom && loguom->propType() == Mnemonic::Vel;
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
    const float startdah = replveldz < 0 ? srddah : track.dah(0);
    const float replvel = wd.info().replvel_;
    const float bulkshift = replveldz > 0 ? 2.f * replveldz / replvel : 0.f;

    TypeSet<float> dpts, vals;
    dpts += replveldz < 0 ? srddepth : track.value(0);
    vals += logisvel ? replvel : bulkshift;
    for ( int idx=0; idx<sz; idx++ )
    {
	const float dah = log.dah( idx );
	const float logval = log.value( idx );
	if ( !mIsUdf(dah) && !mIsUdf(logval) && dah > startdah+mLocalEps )
	{
	    dpts += (float)track.getPos(dah).z;
	    vals += loguom ? loguom->getSIValue( logval ) : logval;
	}
    }

    sz = dpts.size();
    if ( !sz )
	return;

    TypeSet<float> sdpts, svals;
    mGetIdxArr( int, idxs, sz );
    if ( !idxs) return;
    sort_coupled( dpts.arr(), idxs, sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	const int sidx = idxs[idx];
	const float newdepth = dpts[idx];
	const float newval = vals[sidx];
	const int cursz = sdpts.size();
	if ( cursz>1 && (mIsEqual(newdepth,sdpts[cursz-1],mLocalEps) ||
			 mIsEqual(newval,svals[cursz-1],mLocalEps)) )
	    continue;
	sdpts += dpts[idx];
	svals += vals[sidx];
    }

    sz = sdpts.size();
    if ( !sz )
	return;

    TypeSet<float> outvals( sz, mUdf(float) );
    if ( logisvel )
    {
	ArrayValueSeries<float,float> svalsvs(svals.arr(),false);
	ArrayValueSeries<float,float> sdptsvs(sdpts.arr(),false);
	if ( !TimeDepthConverter::calcTimes(svalsvs,sz,sdptsvs,outvals.arr()) )
	    return;

	const float startime = outvals[0];
	for ( int idx=0; idx<sz; idx++ )
	    outvals[idx] += bulkshift - startime;
    }
    else
    {
	TimeDepthModel verticaldtmod;
	if ( !verticaldtmod.setModel(sdpts.arr(),svals.arr(),sz) )
	    return;

	outvals += replvel;
	for ( int idx=1; idx<sz; idx++ )
	    outvals += verticaldtmod.getVelocity( sdpts.arr(), svals.arr(),
						  sz, sdpts[idx] );
    }

    log.setEmpty();
    for ( int idx=0; idx<sz; idx++ )
    {
	const float outdah = track.getDahForTVD( sdpts[idx] );
	const float outval = outuom ? outuom->getUserValueFromSI(outvals[idx])
				    : outvals[idx];

	log.addValue( outdah, outval );
    }

    if ( outuom )
    {
	log.setUnitMeasLabel( outuomlbl );
	log.setMnemonicLabel( nullptr, true );
    }
}


void WellTie::GeoCalculator::removeSpikes( float* inp, int sz, int gate,int fac)
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
void WellTie::GeoCalculator::deconvolve( const float* inp,
					 const float_complex* filter,
					 float* deconvals, int inpsz )
{
    if ( !inp || !filter )
	return;

    ArrayNDWindow window( Array1DInfoImpl(inpsz), false, "CosTaper", 0.90 );

    Array1DImpl<float> inputvals( inpsz );
    OD::memCopy( inputvals.getData(), inp, inpsz*sizeof(float) );
    window.apply( &inputvals );
    removeBias<float,float>( inputvals );

    Array1DImpl<float_complex> cfiltervals( inpsz );
    OD::memCopy( cfiltervals.getData(), filter, inpsz*sizeof(float_complex) );
    window.apply( &cfiltervals );
    removeBias<float_complex,float>( cfiltervals );

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


double WellTie::GeoCalculator::crossCorr( const float* seis, const float* synth,
					  float* outp, int sz )
{
    genericCrossCorrelation( sz, 0, seis, sz, 0, synth, sz, -sz/2, outp );
    LinStats2D ls2d; ls2d.use( seis, synth, sz );
    return ls2d.corrcoeff;
}


void WellTie::GeoCalculator::d2TModel2Log( const Well::D2TModel& d2t,
					   Well::Log& log )
{
    log.setEmpty();
    for ( int idx=0; idx<d2t.size(); idx++ )
	log.addValue( d2t.dah( idx ), d2t.value( idx ) );

    const Mnemonic::StdType tp = Mnemonic::Time;
    log.setUnitMeasLabel( UoMR().getInternalFor(tp)->symbol() );
    log.setMnemonicLabel( nullptr, true );
}
