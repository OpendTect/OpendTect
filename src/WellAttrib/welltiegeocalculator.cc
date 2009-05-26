/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bruno
 Date:          Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiegeocalculator.cc,v 1.8 2009-05-26 07:06:53 cvsbruno Exp $";


#include "arraynd.h"
#include "arrayndutils.h"
#include "arrayndimpl.h"

#include "fft.h"
#include "wavelet.h"
#include "hilberttransform.h"
#include "genericnumer.h"
#include "welltiegeocalculator.h"
#include "welltieunitfactors.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "welltrack.h"
#include "welltiesetup.h"
#include "welld2tmodel.h"
#include "survinfo.h"

#include <complex>
#include <algorithm>

WellTieGeoCalculator::WellTieGeoCalculator( const WellTieParams* p,
					    const Well::Data* wd )
		: params_(*p)
		, wtsetup_(p->getSetup())	
		, wd_(*wd) 
		, denfactor_(params_.getUnits().denFactor())
		, velfactor_(params_.getUnits().velFactor())
{
}    

//each sample is converted to a time using the travel time log
//the correspondance between samples and depth values provides a first
//TWT approx.
Well::D2TModel* WellTieGeoCalculator::getModelFromVelLog( bool doclean )
{
    const Well::Log& log = *wd_.logs().getLog( params_.currvellognm_ );
    TypeSet<float> vals, d2t, dpt, time, depth;

    dpt += -wd_.track().value(0);
    vals += 0;
    for ( int validx=0; validx<log.size(); validx++ )
    {
	vals += log.valArr()[validx];
	dpt  += log.dah(validx);
    }

    if ( doclean )
    {
	interpolateLogData( dpt, log.dahStep(true), true );
	interpolateLogData( vals, log.dahStep(true), false );
    }
    if ( !wtsetup_.issonic_ )
    {
	for ( int validx=0; validx<vals.size(); validx++)
	    time +=  vals[validx]*velfactor_;
    }
    else
    {
	TWT2Vel( vals, dpt, d2t, false );

	int idx=0;
	while ( idx < d2t.size() )
	{
	    time += d2t[idx];
	    depth += dpt[idx];
	    idx++;
	}
    }

    Well::D2TModel* d2tnew = new Well::D2TModel;
    for ( int dahidx=0; dahidx<depth.size(); dahidx++ )
	d2tnew->add( depth[dahidx], time[dahidx] );

    return d2tnew;
}


void WellTieGeoCalculator::setVelLogDataFromModel(
				          const Array1DImpl<float>& depthvals,  					  const Array1DImpl<float>& velvals, 
					  Array1DImpl<float>& outp	) 
{
    const Well::D2TModel* d2t = wd_.d2TModel();
    TypeSet<float> time, depth, veldata;
    
    for ( int idx=0; idx<depthvals.info().getSize(0); idx++ )
    {
	depth += depthvals.get(idx);
 	time  += d2t->getTime(depthvals.get(idx));
    }
    TWT2Vel( time, depth, veldata, true );

    for ( int idx=0; idx<depthvals.info().getSize(0); idx++ )
	outp.setValue( idx, veldata[idx] );
}


Well::D2TModel* WellTieGeoCalculator::getModelFromVelLogData(
					    const Array1DImpl<float>& velvals, 
					    const Array1DImpl<float>& dptvals )
{/*
    TypeSet<float> vals, d2t, dpt, time, depth;

    dpt += -wd_.track().value(0);
    vals += 0;
    for ( int idx=0; idx<velvals.info().getSize(0); idx++ )
    {
	vals += velvals.get(idx);
	dpt  += dptvals.get(idx);
    }

    TWT2Vel( vals, dpt, d2t, false );
    int idx=0;
    while ( idx < d2t.size() )
    {
	time += d2t[idx];
	depth += dpt[idx];
	idx++;
    }
*/
    Well::D2TModel* d2tnew = new Well::D2TModel;
    for ( int dahidx=0; dahidx<dptvals.info().getSize(0); dahidx++ )
	d2tnew->add( dptvals.get(dahidx), velvals.get(dahidx) );

    return d2tnew;
}


//Small TWT/ Interval Velocity converter
#define mFactor 1000000000
void WellTieGeoCalculator::TWT2Vel( const TypeSet<float>& timevel,
				     const TypeSet<float>& dpt,	
				     TypeSet<float>& outp, bool t2vel  )
{
    if ( t2vel )
    {
	outp += 0;
	for ( int idx=1; idx<timevel.size(); idx++ )
	    outp +=  mFactor*( timevel[idx]-timevel[idx-1] )
		    /( (dpt[idx]-dpt[idx-1])/velfactor_*2 );
	outp[0] = outp[1];
    }
    else 
    {
	outp += 0;
	for ( int idx=1; idx<timevel.size(); idx++ )
	    outp +=  2*( dpt[idx]-dpt[idx-1] )*timevel[idx]/velfactor_/mFactor;

	for ( int idx=1;  idx<timevel.size(); idx++ )
	    outp[idx] += outp[idx-1];
    }
}


void WellTieGeoCalculator::stretchArr( const Array1DImpl<float>& inp,
				       Array1DImpl<float>& outp, int idxstart,
				       int idxstop, int idxpick, int idxlast )
{
    const int datasz = inp.info().getSize(0);
    const float stretchfac = ( idxpick-idxstart ) / (float) (idxstop-idxstart); 
    const float squeezefac = ( idxlast-idxpick ) / (float) (idxlast - idxstop);
   
    float val;
    for ( int idx=idxstart; idx<idxstop; idx++ )
    {
	float curval = ( idx - idxstart )*stretchfac + idxstart;
	int curidx = (int)curval;
	if ( curidx >= datasz-1 ) return;
	interpolAtIdx( inp.get( curidx ), inp.get( curidx+1), curval, val );
	outp.setValue( idx, val );
    }
    for ( int idx=idxstop; idx<datasz-1; idx++ )
    {
	float curval = ( idx - datasz )*squeezefac + datasz;
	int curidx = (int)curval;
	if ( curidx >= datasz-1 ) return;
	interpolAtIdx( inp.get( curidx ), inp.get( curidx+1), curval, val );
	outp.setValue( idx , val );
    }
}


void WellTieGeoCalculator::interpolAtIdx( float prevval, float postval,
				 	float curval, float& outval )
{
    int curidx = (int)curval;
    outval = ( curval - curidx ) * ( postval - prevval ) + prevval;
}


//Small Data Interpolator, specially designed for log (dah) data
//assumes no possible negative values in dens or vel log
//will replace them by interpolated values if so. 
void WellTieGeoCalculator::interpolateLogData( TypeSet<float>& data,
					       float dahstep, bool isdah )
{
    int startidx = getFirstDefIdx( data );
    int lastidx = getLastDefIdx( data );

    for ( int idx=startidx; idx<lastidx; idx++)
    {
	//no negative values in dens or vel log assumed
	if ( !isdah && ( mIsUdf(data[idx]) || data[idx]<0 ) )
	    data[idx] = data[idx-1];
	if ( isdah && (mIsUdf(data[idx]) || data[idx]<data[idx-1]
	     || data[idx] >= data[lastidx])  )
	    data[idx] = data[idx-1] + dahstep;
    }
    for ( int idx=0; idx<startidx; idx++ )
	data[idx] = data[startidx];
    for ( int idx=lastidx; idx<data.size(); idx++ )
	data[idx] = data[lastidx];
}


int WellTieGeoCalculator::getFirstDefIdx( const TypeSet<float>& logdata )
{
    int idx = 0;
    while ( mIsUdf(logdata[idx]) )
	idx++;
    return idx;
}


int WellTieGeoCalculator::getLastDefIdx( const TypeSet<float>& logdata )
{
    int idx = logdata.size()-1;
    while ( mIsUdf( logdata[idx] ) )
	idx--;
    return idx;
}


bool WellTieGeoCalculator::isValidLogData( const TypeSet<float>& logdata )
{
    if ( logdata.size() == 0 || getFirstDefIdx(logdata) > logdata.size() )
	return false;
    return true;
}



//low pass filter, almost similar as this of the freqfilter attribute
//TODO put in algo
#define mDoTransform(tf,isstraight,inp,outp,sz) \
{   \
    tf.setInputInfo(Array1DInfoImpl(sz));\
    tf.setDir(isstraight);\
    tf.init();\
    tf.transform(inp,outp);\
}
void WellTieGeoCalculator::lowPassFilter( Array1DImpl<float>& vals,
					  float cutfreq )
{
    int filtersz = vals.info().getSize(0);
    if ( !filtersz )
       	return;

    float df = 1/( filtersz * SI().zStep() );
    TypeSet<float> freq, fwin, backwin; 

    for ( int idx=0; idx<filtersz; idx++ )
    {
	backwin += vals.get(idx);
	freq += df*idx; 
	fwin += 0;
    }
    for ( int idx=0; idx<(int)filtersz/2; idx++ )
    {
	if ( freq[idx] <= cutfreq )
	{
	    fwin[idx] += 1;
	    fwin[filtersz-idx] += 1;
	}
	else if ( freq[idx] > cutfreq )
	{
	    fwin[idx] += 0;
	    fwin[filtersz-idx] += 0;
	}
    }

    Array1DImpl<float> timevals ( filtersz );
    Array1DImpl<float_complex> ctimevals ( filtersz ), cfreqvals ( filtersz ),
    			       filtervals( filtersz ), coutvals  ( filtersz );
    //init data
    memcpy( timevals.getData(), vals.getData(), filtersz*sizeof(float) ); 
    ArrayNDWindow window( Array1DInfoImpl(filtersz),
			 false, "CosTaper", 0.95 );
    //window.apply( &timevals );
    float avg = computeAvg( &timevals );
    if ( mIsZero(avg,1e-4) )
	removeBias( &timevals );

    //compute complex numbers
    HilbertTransform hil;
    hil.setCalcRange(0,filtersz,0);
    mDoTransform( hil, true, timevals, ctimevals, filtersz );
    
    //apply hight-cut windowing in the frequency domain
    FFT fft(false);
    mDoTransform( fft, true, ctimevals, cfreqvals, filtersz );
    for ( int idx=0; idx<filtersz; idx++ )
	cfreqvals.setValue( idx, fwin[idx]*cfreqvals.get(idx) );

    //back to time domain 
    mDoTransform( fft, false, cfreqvals, coutvals, filtersz );
    
    for ( int idx=0; idx<filtersz; idx++ )
    {
	if ( idx<filtersz/400 || idx>filtersz-filtersz/400 )
	    vals.set( idx, backwin[idx] );
	else
	    vals.set( idx, coutvals.get( idx ).real() + avg );
    }
}


//resample data. If higher step, linear interpolation
//else nearest sample
void WellTieGeoCalculator::resampleData( const Array1DImpl<float>& invals,
				 Array1DImpl<float>& outvals, float step )
{
    int datasz = invals.info().getSize(0);
    if ( step >1  )
    {
	for ( int idx=0; idx<datasz-1; idx++ )
	{
	    float stepval = ( invals.get(idx+1) - invals.get(idx) ) / step;
	    for ( int stepidx=0; stepidx<step; stepidx++ )
		outvals.setValue( idx, invals.get(idx) + stepidx*stepval ); 
	}
	outvals.setValue( datasz-1, invals.get(datasz-1) );
    }
    else 
    {
	for ( int idx=0; idx<datasz*step; idx+=(int)(1/step) ) 
	    outvals.setValue( idx, invals.get(idx) );
    }
}


void WellTieGeoCalculator::computeAI( const Array1DImpl<float>& velvals,
				   const Array1DImpl<float>& denvals,
				   Array1DImpl<float>& aivals )
{
    for ( int idx = 0; idx < velvals.info().getSize(0); idx++ )
    {
	float velval = velvals.get(idx);
	float denval = denvals.get(idx);
	aivals.setValue( idx, denval/velval *mFactor*denfactor_*velfactor_ );
    }
}


//Compute reflectivity values at a the display sample step (Survey step)
void WellTieGeoCalculator::computeReflectivity(const Array1DImpl<float>& aivals,
					       Array1DImpl<float>& reflvals,
					       int shiftstep )
{
    float ai1, ai2, rval;
    float prevval = 0;
    int sz = reflvals.info().getSize(0);

    for ( int idx=0; idx<sz-1; idx++ )
    {
	ai2 = aivals.get( shiftstep*(idx+1));
	ai1 = aivals.get( shiftstep*(idx)  );	   

	if ( (ai1 + ai2 ) == 0 )
	    rval = prevval;
	else
	    rval =  ( ai2 - ai1 ) / ( ai2 + ai1 );     
	
	reflvals.setValue( idx, rval ); 
	prevval = rval;
    }
	reflvals.setValue( 0, reflvals.get(1) ); 
	reflvals.setValue( sz-1, reflvals.get(sz-2) ); 
}


void WellTieGeoCalculator::convolveWavelet( const Array1DImpl<float>& wvltvals,
					const Array1DImpl<float>& reflvals,
					Array1DImpl<float>& synvals, int widx )
{
    int reflsz = reflvals.info().getSize(0);
    int wvltsz = wvltvals.info().getSize(0);
    float* outp = new float[reflsz];

    GenericConvolve( wvltsz, -widx, wvltvals.getData(),
		     reflsz, 0  	 , reflvals.getData(),
		     reflsz, 0  	 , outp );

    memcpy( synvals.getData(), outp, reflsz*sizeof(float));
    delete outp;
}


#define mNoise 0.005
void WellTieGeoCalculator::deconvolve( const Array1DImpl<float>& inputvals,
				       const Array1DImpl<float>& filtervals,
				       Array1DImpl<float>& deconvals, 
				       int wvltsz )
{
    const int sz = inputvals.info().getSize(0);
    const int filtersz = filtervals.info().getSize(0);
    int border = wvltsz/2;

    Array1DImpl<float> timeinputvals( sz ), timefiltervals(filtersz );

    Array1DImpl<float_complex> ctimeinputvals( sz ), cfreqinputvals( sz ),
		ctimefiltervals( filtersz ), cfreqfiltervals( filtersz ),
		ctimedeconvvals( filtersz ), cfreqdeconvvals( filtersz ),
		ctimenoisevals ( filtersz ), cfreqnoisevals ( filtersz ),
	       	cspecfiltervals( filtersz );

    memcpy(timeinputvals.getData(), inputvals.arr(), filtersz*sizeof(float)); 
    memcpy(timefiltervals.getData(), filtervals.arr(), filtersz*sizeof(float)); 

    ArrayNDWindow window( Array1DInfoImpl(filtersz),
			 false, "CosTaper", 0.95 );
    window.apply( &timefiltervals );
    window.apply( &timeinputvals );
    //removeBias( &timefiltervals );
    //removeBias( &timeinputvals );

    HilbertTransform hil;
    hil.setCalcRange(0, filtersz, 0);
    mDoTransform( hil, true, timeinputvals, ctimeinputvals, filtersz );
    hil.setCalcRange(0, filtersz, 0);
    mDoTransform( hil, true, timefiltervals, ctimefiltervals, filtersz );
    
    FFT fft(false);
    mDoTransform( fft, true, ctimeinputvals, cfreqinputvals, filtersz );
    mDoTransform( fft, true, ctimefiltervals, cfreqfiltervals, filtersz );

    Spectrogram spec;
    mDoTransform( spec, true, ctimefiltervals, cspecfiltervals, filtersz );

    float_complex wholespec = 0;
    float_complex noise = mNoise/filtersz;

    for ( int idx=0; idx<filtersz; idx++ )
	wholespec += cspecfiltervals.get(idx);  
    float_complex cnoiseshift = noise*wholespec;
    
    for ( int idx=border; idx<filtersz-border; idx++ )
    {
	float_complex inputval = cfreqinputvals.get(idx);
	float_complex filterval = cfreqfiltervals.get(idx);

	double rfilterval = filterval.real();
	double ifilterval = filterval.imag();
	float_complex conjfilterval = float_complex( rfilterval ,-ifilterval ); 

	float_complex numer = inputval * conjfilterval;
	float_complex denom = filterval* conjfilterval;// + cnoiseshift;
	float_complex res = numer / denom;

	cfreqdeconvvals.setValue( idx, res );
    }
    for ( int idx=0; idx<border; idx++ )
    {
	cfreqdeconvvals.setValue( idx, 0 );
	cfreqdeconvvals.setValue( idx+filtersz-border, 0 );
    }
    
    float avg = 0;
    for ( int idx=0; idx<filtersz; idx++ )
	avg += abs( cfreqdeconvvals.get(idx) )/filtersz;

    for ( int idx=0; idx<filtersz; idx++ )
    {
	if ( abs (cfreqdeconvvals.get(idx) ) < avg/4 )
	    cfreqdeconvvals.set( idx, 0 );
    }
   
    mDoTransform( fft, false, cfreqdeconvvals, ctimedeconvvals, filtersz );

    int mid = (int)(filtersz)/2;
    for ( int idx=0; idx<=mid; idx++ )
	deconvals.set( idx, ctimedeconvvals.get( mid-idx ).real() );
    for ( int idx=mid+1; idx<filtersz; idx++ )
	deconvals.set( idx, ctimedeconvvals.get( filtersz-idx+mid ).real() );
}


void WellTieGeoCalculator::reverseWavelet( Wavelet& wvlt )
{
    const int wvltsz = wvlt.size();
    Array1DImpl<float> wvltvals (wvltsz);
    memcpy( wvltvals.getData(), wvlt.samples(), wvltsz*sizeof(float) );

    for ( int idx=0; idx<wvltsz/2; idx++ )
    {
	wvlt.samples()[idx] = wvltvals.get(wvltsz-idx-1);
	wvlt.samples()[wvltsz-idx-1] = wvltvals.get( idx );
    }
}


void WellTieGeoCalculator::autocorr( const Array1DImpl<float>& seisvals, 
				     const Array1DImpl<float>& synthvals,
       				     Array1DImpl<float>& outpvals	)
{
    const int datasz = seisvals.info().getSize(0);
    float* outp = new float[datasz];
    genericCrossCorrelation( datasz, 0, seisvals,
			     datasz, 0, synthvals,
			     datasz, 0, outp);
    memcpy( outpvals.getData(), outp, datasz*sizeof(float));
    delete outp;
}
