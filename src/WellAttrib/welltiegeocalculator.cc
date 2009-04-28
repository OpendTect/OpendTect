/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bruno
 Date:          Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiegeocalculator.cc,v 1.5 2009-04-28 14:30:26 cvsbruno Exp $";


#include "arraynd.h"
#include "arrayndutils.h"
#include "arrayndimpl.h"

#include "fft.h"
#include "hilberttransform.h"
#include "genericnumer.h"
#include "welltiegeocalculator.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "welltiesetup.h"
#include "welld2tmodel.h"
#include "survinfo.h"

#include <complex>
#include <algorithm>

WellTieGeoCalculator::WellTieGeoCalculator( const WellTieSetup& wts,
					    const Well::Data& wd )
		: wtsetup_(wts)
		, wd_(wd) 
		, denfactor_(wtsetup_.factors_.denFactor())
		, velfactor_(wtsetup_.factors_.velFactor())
{
}    

//each sample is converted to a time using the travel time log
//the correspondance between samples and depth values provides a first
//TWT approx.
#define mMsFact 0.001
Well::D2TModel* WellTieGeoCalculator::getModelFromVelLog( bool doclean )
{
    const BufferString lognm( wd_.checkShotModel() && wtsetup_.iscscorr_
			    ? wtsetup_.corrvellognm_ : wtsetup_.vellognm_ );

    const Well::Log& log = *wd_.logs().getLog( lognm );
    TypeSet<float> vals, d2t, dpt, time, depth;

    for ( int validx=0; validx<log.size(); validx++ )
    {
	vals += log.valArr()[validx];
	dpt  += log.dah(validx);
    }

    if ( doclean )
    {
	interpolateData( dpt, true, log.dahStep(true) );
	interpolateData( vals, false, log.dahStep(true) );
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
	d2tnew->add( depth[dahidx], time[dahidx]*mMsFact );

    return d2tnew;
}

//Small TWT/ Interval Velocity converter
#define mFactor 1000000000
void WellTieGeoCalculator::TWT2Vel( const TypeSet<float>& timevel,
				     const TypeSet<float>& dpt,	
				     TypeSet<float>& outp, bool t2vel  )
{
    float velfactor = wtsetup_.factors_.velFactor();
    if ( t2vel )
    {
	outp += timevel[0] / (velfactor*dpt[0]);
	for ( int idx=1; idx<timevel.size(); idx++ )
	    outp +=  mFactor*( timevel[idx]-timevel[idx-1] )
		    /( (dpt[idx]-dpt[idx-1])/velfactor*2 );
	outp[0] = outp[1];
	outp[0] += 150;
	outp[1] -= 50;
	outp[2] += 50;
    }
    else 
    {
	outp +=  2*dpt[0]*timevel[0]/velfactor_/mFactor;
	for ( int idx=1; idx<timevel.size(); idx++ )
	    outp +=  2*( dpt[idx]-dpt[idx-1] )*timevel[idx]/velfactor_/mFactor;

	for ( int idx=1;  idx<timevel.size(); idx++ )
	    outp[idx] += outp[idx-1];
    }
}


void WellTieGeoCalculator::stretchArr( const Array1DImpl<float>& inp,
					Array1DImpl<float>& outp, 
					int idxstart, int idxstop,
					int idxpick )
{
    const int datasz = inp.info().getSize(0);
    const float stretchfac = ( idxpick-idxstart ) / (float) (idxstop-idxstart); 
    const float squeezefac = ( datasz-idxpick ) / (float) (datasz - idxstop);
   
    float val;
    for ( int idx=idxstart; idx<idxstop; idx++ )
    {
	interpolAtIdx( inp, ( idx - idxstart )*stretchfac + idxstart, val );
	outp.setValue( idx, val );
    }
    for ( int idx=idxstop; idx<datasz-1; idx++ )
    {
	interpolAtIdx( inp, ( idx - datasz )*squeezefac + datasz, val );
	outp.setValue( idx , val );
    }
}


void WellTieGeoCalculator::interpolAtIdx( const Array1DImpl<float>& inp,
				 	float curval, float& outval )
{
    int curidx = (int)curval;
    float prevval = inp.get( curidx );
    float postval = inp.get( curidx+1 );
    outval = ( curval - curidx ) * ( postval - prevval ) + prevval;
}

    /*
bool WellTieD2TModelManager::shiftModel( const float shift)
{
    TypeSet<float> dah, time;

    const Well::D2TModel& d2t = d2T();
    //copy old d2t
    for (int idx = 0; idx<d2t_.size(); idx++)
    {
    time += d2t.value( idx );
    dah  += d2t.dah( idx );
    }

    //replace by shifted one
    d2t.erase();
    for ( int dahidx=0; dahidx<dah.size(); dahidx++ )
    d2t.add( dah[dahidx], time[dahidx] + shift );

    if ( d2t.size() > 1 )
    wd_.d2tchanged.trigger();
    else
    return false;

return true;
}
*/


//Small Data Interpolator, specially designed for log (dah) data
//assumes no possible negative values in dens or vel log
//will replace them by interpolated values if so. 
void WellTieGeoCalculator::interpolateData( TypeSet<float>& data,
					      float dahstep,
					      bool isdah )
{
    const int sz = data.size();
    for ( int idx=1; idx<sz; idx++ )
    {
	if ( isdah )
	{
	    if ( (mIsUdf(data[idx]) || data[idx] < data[idx-1]
				    || data[idx] >= data[sz-1]) )
		data[idx] = data[idx-1] + dahstep;
	}
	else
	{
	    if  ( ( mIsUdf(data[idx]) || data[idx] < 0 ) )
		data[idx] = data[idx-1];
	}
    }
}


//low pass filter similar as this of the freqfilter attribute
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
{/*
    //construct filter window
    TypeSet<float> filterborders(51,0);
    int idt = 0;
    for ( float idx= 0.95; idx<=1; idx+=0.001 )
    {
	float rx = idx;
	rx -= 0.95;
	rx *= 20;
	filterborders[idt]= (1 + cos( M_PI * rx )) * .5;
	idt++;
    }*/

    int filtersz = vals.info().getSize(0);
    float df = 1/( filtersz * SI().zStep() );
    TypeSet<float> freq, fwin;

    for ( int idx=0; idx< filtersz; idx++ )
    {
	freq += df*idx; 
	fwin += 0;
    }
    int idborder = 0;
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
	else
	{
	    //fwin[idx] = filterborders[idborder];
	    //fwin[filtersz-idx] += filterborders[idborder];
	    idborder++;
	}
    }

    Array1DImpl<float> timevals ( filtersz );
    Array1DImpl<float_complex> ctimevals ( filtersz );
    Array1DImpl<float_complex> cfreqvals ( filtersz );
    Array1DImpl<float_complex> filtervals( filtersz );
    Array1DImpl<float_complex> coutvals  ( filtersz );
    
    //init data
    memcpy( timevals.getData(), vals.getData(), filtersz*sizeof(float) ); 
    ArrayNDWindow window( Array1DInfoImpl(filtersz),
			 false, "CosTaper", 0.95 );
    //window.apply( &timevals );
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
	vals.set( idx, coutvals.get( idx ).real() );
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
				   Array1DImpl<float>& aivals, bool dofilter )
{
    const int datasz = velvals.info().getSize(0);
    for ( int idx = 0; idx < datasz; idx++ )
    {
	aivals.setValue( idx,  denvals.get(idx)*mFactor*denfactor_*velfactor_
			     / velvals.get(idx) );
	if ( mIsUdf(aivals.get(idx)) && idx>0 ) 
	    aivals.setValue( idx, aivals.get(idx-1) );
    }
}


//Compute reflectivity values at a the display sample step (Survey step)
#define mStep 20 
void WellTieGeoCalculator::computeReflectivity(const Array1DImpl<float>& aivals,
					       Array1DImpl<float>& reflvals )
{
    float ai1, ai2, rval;
    float prevval = 0;

    for ( int idx=0; idx<reflvals.info().getSize(0); idx++ )
    {
	ai2 = aivals.get( mStep*(idx+1) + mStep/2 );
	ai1 = aivals.get( mStep*(idx+1) - mStep/2 );	   

	if ( (ai1 + ai2 ) == 0 )
	    reflvals.setValue( idx, prevval );
	else
	{
	    rval =  ( ai2 - ai1 ) / ( ai2 + ai1 );     
	    reflvals.setValue( idx, rval ); 
	}
	prevval = reflvals.get(idx);
    }
}


void WellTieGeoCalculator::convolveWavelet( const Array1DImpl<float>& wvltvals,
					const Array1DImpl<float>& reflvals,
					Array1DImpl<float>& synvals, int widx )
{
    int reflsz = reflvals.info().getSize(0);
    int wvltsz = wvltvals.info().getSize(0);
    float* outp = new float[reflsz];

    GenericConvolve( wvltsz, -widx, wvltvals.getData(),
		     reflsz, 0  	 , reflvals,
		     reflsz, 0  	 , outp );

    for ( int idx=0; idx<reflsz; idx++ )
	synvals.setValue( idx, outp[idx] );

    delete outp;
}


#define mNoise 0.05
void WellTieGeoCalculator::deconvolve( const Array1DImpl<float>& inputvals,
				    const Array1DImpl<float>& filtervals,
				    Array1DImpl<float>& deconvals )
{
    const int sz = inputvals.info().getSize(0);
    const int filtersz = filtervals.info().getSize(0);

    Array1DImpl<float> timeinputvals( sz ), timefiltervals(filtersz );

    Array1DImpl<float_complex> ctimeinputvals( sz ), cfreqinputvals( sz ),
		ctimefiltervals( filtersz ), cfreqfiltervals( filtersz ),
		ctimedeconvvals( filtersz ), cfreqdeconvvals( filtersz ),
		ctimenoisevals ( filtersz ), cfreqnoisevals ( filtersz ),
	       	cspecfiltervals( filtersz ), cfreqdeconswappedvals( filtersz );

    memcpy(timeinputvals.getData(), inputvals.arr(), filtersz*sizeof(float)); 
    memcpy(timefiltervals.getData(), filtervals.arr(), filtersz*sizeof(float)); 

    removeBias( &timeinputvals );
    removeBias( &timefiltervals );

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
    
    for ( int idx=0; idx<filtersz; idx++ )
    {
	float_complex inputval = cfreqinputvals.get(idx);
	float_complex filterval = cfreqfiltervals.get(idx);

	double rfilterval = filterval.real();
	double ifilterval = filterval.imag();
	float_complex conjfilterval = float_complex( rfilterval ,-ifilterval ); 

	float_complex numer = inputval * conjfilterval;
	float_complex denom = filterval* conjfilterval + cnoiseshift;
	float_complex res = numer / denom;

	cfreqdeconvvals.setValue( idx, res );
    }
    
    float avg = 0;
    for ( int idx=0; idx<filtersz; idx++ )
	avg += abs( cfreqdeconvvals.get(idx) )/filtersz;

    for ( int idx=0; idx<sz; idx++ )
    {
	if ( abs (cfreqdeconvvals.get(idx) ) < avg/4 )
	    cfreqdeconvvals.set( idx, 0 );
    }

    mDoTransform( fft, false, cfreqdeconvvals, ctimedeconvvals, filtersz );

    int mid = (int)filtersz/2;
    for ( int idx=0; idx<mid; idx++ )
    {
	deconvals.set( idx, ctimedeconvvals.get( mid-idx-1 ).real() );
	deconvals.set( filtersz-idx-1, ctimedeconvvals.get( idx+mid ).real() );
    }
}
