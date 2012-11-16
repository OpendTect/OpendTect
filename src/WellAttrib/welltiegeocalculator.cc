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
#include "survinfo.h"
#include "welllog.h"
#include "welllogset.h"
#include "welldata.h"
#include "welltrack.h"
#include "welltieunitfactors.h"
#include "welld2tmodel.h"

#include <complex>

namespace WellTie
{


Well::D2TModel* GeoCalculator::getModelFromVelLog( const Well::Data& wd, 
					const char* sonlog, bool issonic ) const
{
    const Well::Log* log = wd.logs().getLog( sonlog );
    if ( !log ) return 0; 

    Well::Log proclog = Well::Log( *log );

    const float replveldz = wd.info().srdelev - wd.track().getKbElev();

    const float startdah = replveldz < 0 ? -1.f * replveldz : 0;
    const float defvelrepl = mCast( float, !SI().zInFeet() ? 2000 : 8000 );
    const float replvel = mIsUdf(wd.info().replvel) ? defvelrepl :
       				 wd.info().replvel;
    const float bulkshift = replveldz > 0 ? 
			   2.f * replveldz / replvel : 0;
    
    if ( issonic )
	son2TWT( proclog, wd.track(), true, startdah );
    else
	vel2TWT( proclog, wd.track(), true, startdah );

    TypeSet<float> dpt, vals;
    for ( int idx=0; idx<proclog.size(); idx++ )
    {
	float dah = proclog.dah( idx );
	if ( mIsUdf( dah ) ) continue;
	dpt += dah;
	vals += proclog.getValue( dah, true ) + bulkshift;
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
    const float defvelrepl = mCast( float, !SI().zInFeet() ? 2000 : 8000 );

    float replvel = mIsUdf(wd.info().replvel) ? defvelrepl : wd.info().replvel;
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
	if ( mIsZero( d2t.dah( idx ), 1e-3 ) )
	    initialt = d2t.value( idx );
	else if ( mIsZero( d2t.dah( idx ) - srddah, 1e-3 ) )
	    initialt = 2.f * srddah / replvel;
	else initialt = replvelshift;
    }
    sort_coupled( times.arr(), mVarLenArr(zidxs), sz );
    d2t.erase();

    d2t.add( 0, replvelshift ); //set KB
    if ( srdbelowkb )
	d2t.add( srddah, 0); //set SRD

    int idah = -1;
    const float bulkshift = mIsZero( initialt-replvelshift, 1e-3 ) ? 0 :
				     initialt-replvelshift;
    do { idah++; }
    while ( dahs[zidxs[idah]] <= srddah || dahs[zidxs[idah]]  < 1e-1 ||
	    times[zidxs[idah]] < 0 );
    if ( idah >= d2t.size() )
	idah = 0;
    if ( dahs[zidxs[idah]] > srddah && dahs[zidxs[idah]] > 1e-1 &&
	 times[zidxs[idah]] > 0 )
	d2t.add( dahs[zidxs[idah]], times[zidxs[idah]] + bulkshift );

    for ( int idx=idah+1; idx<sz; idx++ )
    {
	int idx0 = zidxs[idx-1];
	int idx1 = zidxs[idx];
	const float dh = dahs[idx1] - dahs[idx0];
	const float dt = times[idx1] - times[idx0];
	if ( dh > 1e-1 && dahs[idx1] > dahs[0] && dahs[idx1] > srddah
	    && dt > 1e-6 && times[idx1] > times[0] && times[idx1] > 0)
	    d2t.add( dahs[idx1], times[idx1] + bulkshift );
    }
}


#define mMicroSFactor 10e5f
void GeoCalculator::son2Vel( Well::Log& log, bool straight ) const
{
    UnitFactors uf; double velfac = uf.getVelFactor( log, straight );
    if ( !velfac || mIsUdf(velfac) ) 
	velfac = 1;

    for ( int idx=0; idx<log.size(); idx++ )
    {
	float& val = log.valArr()[idx];
	val = val ? (float)velfac/val : val;
	val *= straight ? 1 : mMicroSFactor;
    }
    log.setUnitMeasLabel( straight ? UnitFactors::getStdVelLabel() 
				   : UnitFactors::getStdSonLabel() );
}


void GeoCalculator::son2TWT(Well::Log& log, const Well::Track& track,
			    bool straight, float startdah) const
{
    if ( straight )
    {
	son2Vel( log, straight );
	vel2TWT( log, track, straight, startdah );
    }
    else
    {
	vel2TWT( log, track, straight, startdah );
	son2Vel( log, straight );
    }
}


void GeoCalculator::vel2TWT(Well::Log& log, const Well::Track& track,
			    bool straight, float startdah) const
{
    const int sz = log.size(); 
    if ( sz < 2 ) return;

    UnitFactors uf; double velfac = uf.getVelFactor( log, false );
    if ( !velfac || mIsUdf(velfac) ) 
	velfac = 1;

    TypeSet<float> dpts, vals;
    dpts += startdah; vals += straight ? log.value( 0 ) : 0; 
    for ( int idx=0; idx<sz; idx++ )
    {
	const float dah = log.dah( idx );
	if ( dah >= startdah )
	    { dpts += dah; vals += log.value( idx ); }
    }

    log.erase();
    float prevval, newval; newval = prevval = 0;
    for ( int idx=1; idx<dpts.size(); idx++ )
    {
	if ( straight )
	{
	    float v = vals[idx];
	    if ( !v ) continue; 
	    newval = (float) ( 2.f * ( track.getPos(dpts[idx]).z -
		       		       track.getPos(dpts[idx-1]).z )
		    		       / (v*velfac) );
	    newval += prevval;
	    prevval = newval;
	}
	else 
	{
	    if ( vals[idx] != vals[idx-1] )
		newval = mCast( float, 2.f * ( track.getPos(dpts[idx]).z -
				 track.getPos(dpts[idx-1]).z )
		    		/ fabs(vals[idx]-vals[idx-1]) );
	}
	log.addValue( dpts[idx], newval );
    }
    log.setUnitMeasLabel( straight ? UnitFactors::getStdTimeLabel() 
	    			   : UnitFactors::getStdVelLabel() ); 
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
void GeoCalculator::deconvolve( const float* inp, const float* filter,
			        float* deconvals, int inpsz ) const
{
    ArrayNDWindow window( Array1DInfoImpl(inpsz), false, "CosTaper", 0.1 );
    Array1DImpl<float> inputvals( inpsz );
    Array1DImpl<float> filtervals( inpsz );
    memcpy(inputvals.getData(),inp,inpsz*sizeof(float));
    memcpy(filtervals.getData(),filter,inpsz*sizeof(float));
    window.apply( &inputvals );		removeBias<float,float>( &inputvals );
    window.apply( &filtervals );	removeBias<float,float>( &filtervals );

    Array1DImpl<float_complex> cinputvals( inpsz );
    Array1DImpl<float_complex> cfiltervals( inpsz );
    for ( int idx=0; idx<inpsz; idx++ )
    {
	cinputvals.set( idx, inputvals.get( idx ) );
	cfiltervals.set( idx, filtervals.get( idx ) );
    }
   
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
	float_complex conjfilterval = float_complex( (float) rfilterval ,
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
    log.erase();
    for ( int idx=0; idx<d2t.size(); idx++ )
	log.addValue( d2t.dah( idx ), d2t.value( idx ) );

    log.setUnitMeasLabel( UnitFactors::getStdTimeLabel() ); 
}

}; //namespace WellTie
