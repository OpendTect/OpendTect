/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "welltiegeocalculator.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "fourier.h"
#include "fftfilter.h"
#include "hilberttransform.h"
#include "linear.h"
#include "genericnumer.h"
#include "spectrogram.h"
#include "stdio.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "valseries.h"
#include "velocitycalc.h"
#include "welllog.h"
#include "welllogset.h"
#include "welldata.h"
#include "welltrack.h"
#include "welltieunitfactors.h"
#include "welld2tmodel.h"

#include <complex>


namespace WellTie
{


#define	mLocalEps	1e-2f
Well::D2TModel* GeoCalculator::getModelFromVelLog( const Well::Data& wd,
						   const char* sonlog ) const
{
    const Well::Log* log = wd.logs().getLog( sonlog );
    if ( !log ) return 0;

    Well::Log proclog = Well::Log( *log );
    const UnitOfMeasure* loguom = log->unitOfMeasure();
    if ( loguom && loguom->propType()==PropertyRef::Son )
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

    Well::D2TModel* d2tnew = new Well::D2TModel;
    for ( int idx=0; idx<dpt.size(); idx++ )
	d2tnew->add( dpt[idx], vals[idx] );

    d2tnew->setName( "Integrated Depth/Time Model");
    return d2tnew;
}


void GeoCalculator::ensureValidD2TModel( Well::D2TModel& d2t, 
					const Well::Data& wd ) const
{
    const int sz = d2t.size();
    TypeSet<float> dahs, times;
    mAllocVarLenArr( int, zidxs, sz );

    const float replvel = wd.info().replvel;
    const float replveldz = wd.info().srdelev - wd.track().getKbElev();
    const float replvelshift = 2.f * replveldz / replvel;
    const float srddah = -1.f * replveldz; //dz; should use its dah instead
    const bool srdbelowkb = replveldz < 0;

    float initialt = mUdf(float);
    for ( int idx=0; idx<sz; idx++ )
    {
	dahs += d2t.dah( idx ); 
	times += d2t.value( idx ); 
	zidxs[idx] = idx;
	if ( mIsZero(d2t.dah(idx),mLocalEps) )
	    initialt = d2t.value( idx );
	else if ( mIsZero(d2t.dah(idx)-srddah,mLocalEps) )
	    initialt = 2.f * srddah / replvel;
	else initialt = replvelshift;
    }
    sort_coupled( times.arr(), mVarLenArr(zidxs), sz );
    d2t.setEmpty();

    d2t.add( 0, replvelshift ); //set KB
    if ( srdbelowkb )
	d2t.add( srddah, 0); //set SRD

    int idah = -1;
    const float bulkshift = mIsZero( initialt-replvelshift, mLocalEps ) ? 0 :
				     initialt-replvelshift;
    const float lastdah = dahs[zidxs[sz-1]];
    if ( lastdah < srddah-mLocalEps )
	return;

    do { idah++; }
    while ( dahs[zidxs[idah]] <= srddah || dahs[zidxs[idah]]  < mLocalEps ||
	    times[zidxs[idah]] < mLocalEps );

    if ( dahs[zidxs[idah]] > srddah && dahs[zidxs[idah]] > mLocalEps &&
	 times[zidxs[idah]] > mLocalEps )
	d2t.add( dahs[zidxs[idah]], times[zidxs[idah]] + bulkshift );

    for ( int idx=idah+1; idx<sz; idx++ )
    {
	int idx0 = zidxs[idx-1];
	int idx1 = zidxs[idx];
	const float dh = dahs[idx1] - dahs[idx0];
	const float dt = times[idx1] - times[idx0];
	if ( dh > mLocalEps && dahs[idx1] > dahs[0] && dahs[idx1] > srddah
	     && dt > 1e-6f && times[idx1] > times[0] && times[idx1] >mLocalEps )
	    d2t.add( dahs[idx1], times[idx1] + bulkshift );
    }
    if ( d2t.size() < 2 )
	d2t.setEmpty();
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
	    if ( strstr(loguom->name(),"Milli") )
		fact = 1e3f;

	    if ( strstr(loguom->name(),"Micro") )
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
	log.setUnitMeasLabel( outuomlbl );
}


void GeoCalculator::son2TWT( Well::Log& log, const Well::Data& wd ) const
{
    const UnitOfMeasure* loguom = log.unitOfMeasure();
    const bool logissonic = loguom && loguom->propType() == PropertyRef::Son;

    if ( logissonic )
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

    const float srddepth = -1.f * wd.info().srdelev;
    const float srddah = track.getDahForTVD( srddepth );
    const float replveldz = -1.f * srddepth - track.getKbElev();
    const float startdah = replveldz < 0 ? srddah : track.dah(0);
    const float replvel = wd.info().replvel;
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
    mGetIdxArr(int,idxs,sz);
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



#define mDoTransform(tf,isstraight,inp,outp,sz) \
{\
    tf.setInputInfo(Array1DInfoImpl(sz));\
    tf.setDir(isstraight);\
    tf.init();\
    tf.transform(inp,outp);\
}

#define mDoFourierTransform(tf,isstraight,inp,outp,sz) \
{   \
    tf->setInputInfo(Array1DInfoImpl(sz));\
    tf->setDir(isstraight);\
    tf->setNormalization(!isstraight); \
    tf->setInput(inp.getData());\
    tf->setOutput(outp.getData());\
    tf->run(true); \
}

#define mNoise 0.05f
void GeoCalculator::deconvolve( const float* inp, const float_complex* filter,
			        float* deconvals, int inpsz ) const
{
    ArrayNDWindow window( Array1DInfoImpl(inpsz), false, "CosTaper", 0.90 );

    Array1DImpl<float> inputvals( inpsz );
    memcpy( inputvals.getData(), inp, inpsz*sizeof(float) );
    window.apply( &inputvals );
    removeBias<float,float>( &inputvals );
    Array1DImpl<float_complex> cinputvals( inpsz );
    for ( int idx=0; idx<inpsz; idx++ )
	cinputvals.set( idx, inputvals.get( idx ) );

    Array1DImpl<float_complex> cfiltervals( inpsz );
    memcpy( cfiltervals.getData(), filter, inpsz*sizeof(float) );
    window.apply( &cfiltervals );
    removeBias<float_complex,float>( &cfiltervals );
   
    Spectrogram spec;
    Array1DImpl<float_complex> cspecfiltervals( inpsz );
    mDoTransform( spec, true, cfiltervals, cspecfiltervals, inpsz );

    float_complex wholespec = 0;
    float_complex noise = mNoise/inpsz;
    for ( int idx=0; idx<inpsz; idx++ )
	wholespec += cspecfiltervals.get( idx );  
    float_complex cnoiseshift = noise*wholespec;

    PtrMan<Fourier::CC> fft = Fourier::CC::createDefault();
    mDoFourierTransform( fft, true, cinputvals, cinputvals, inpsz );
    mDoFourierTransform( fft, true, cfiltervals, cfiltervals, inpsz );

    Array1DImpl<float_complex> cdeconvvals( inpsz ); 
    for ( int idx=0; idx<inpsz; idx++ )
    {
	float_complex inputval = cinputvals.get(idx);
	float_complex filterval = cfiltervals.get(idx);

	double rfilterval = filterval.real();
	double ifilterval = filterval.imag();
	float_complex conjfilterval = float_complex( (float) rfilterval,
						     (float) -ifilterval );
	float_complex num = inputval * conjfilterval;
	float_complex denom = filterval * conjfilterval + cnoiseshift;
	float_complex res = num / denom;

	cdeconvvals.setValue( idx, res );
    }

    float avg = 0;
    for ( int idx=0; idx<inpsz; idx++ )
	avg += abs( cdeconvvals.get( idx ) )/inpsz;
    for ( int idx=0; idx<inpsz; idx++ )
    {
	if ( abs( cdeconvvals.get( idx ) ) < avg/4 )
	    cdeconvvals.set( idx, 0 );
    }

    mDoFourierTransform( fft, false, cdeconvvals, cdeconvvals, inpsz );

    int mid = (int)(inpsz)/2;
    for ( int idx=0; idx<=mid; idx++ )
	deconvals[idx] = cdeconvvals.get( mid-idx ).real();;
    for ( int idx=mid+1; idx<inpsz; idx++ )
	deconvals[idx] = cdeconvvals.get( inpsz-idx+mid ).real();
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
    for ( int idx=0; idx<d2t.size(); idx++ )
	log.addValue( d2t.dah( idx ), d2t.value( idx ) );

    const PropertyRef::StdType tp = PropertyRef::Time;
    log.setUnitMeasLabel( UoMR().getInternalFor(tp)->symbol() );
}

}; //namespace WellTie
