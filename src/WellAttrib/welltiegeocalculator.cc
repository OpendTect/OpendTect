/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bruno
 Date:          Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiegeocalculator.cc,v 1.2 2009-04-22 09:22:06 cvsbruno Exp $";


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


void WellTieGeoCalculator::TWT2Vel( const TypeSet<float>& timevel,
				     const TypeSet<float>& dpt,	
				     TypeSet<float>& outp, bool t2vel  )
{
    float velfactor = wtsetup_.factors_.velFactor();
    if ( t2vel )
    {
	outp += timevel[0] / (velfactor*dpt[0]);
	for ( int idx=1; idx<timevel.size(); idx++ )
	    outp +=  1000000000*( timevel[idx]-timevel[idx-1] )
		    /( (dpt[idx]-dpt[idx-1])/velfactor*2 );
    
//To Remove, test
outp[0] += 150;
outp[1] += 50;
outp[2] -= 50;
outp[3] += 50;
outp[4] -= 50;

    }
    else 
    {
	outp +=  2*dpt[0]*timevel[0]/velfactor_;
	for ( int idx=1; idx<timevel.size(); idx++ )
	    outp +=  2*( dpt[idx]-dpt[idx-1] )*timevel[idx]/velfactor_;

	for ( int idx=1;  idx<timevel.size(); idx++ )
	    outp[idx] += outp[idx-1];
    }
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
    }

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
	if ( freq[idx] < cutfreq - 25*df )
	{
	    fwin[idx] += 1;
	    fwin[filtersz-idx] += 1;
	}
	else if ( freq[idx] > cutfreq + 25*df )
	{
	    fwin[idx] += 0;
	    fwin[filtersz-idx] += 0;
	}
	else
	{
	    fwin[idx] = filterborders[idborder];
	    fwin[filtersz-idx] += filterborders[idborder];
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
    window.apply( &timevals );
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
    for ( int idx = 0; idx < velvals.info().getSize(0); idx++ )
	aivals.setValue( idx,  denvals.get(idx)*1000000000*denfactor_*velfactor_
			     / velvals.get(idx) );
}


#define mStep = 10 
void WellTieGeoCalculator::computeReflectivity(const Array1DImpl<float>& aivals,
					       Array1DImpl<float>& reflvals )
{
    int size = aivals.info().getSize(0);
    size = int (size/20)+1;
    for ( int idx=0; idx<size-2; idx++ )
    {
	if ( (aivals.get(idx+30) + aivals.get((idx+10)) ) != 0 )
	{
	    float rval = ( aivals.get(20*idx+30) - aivals.get((20*idx+10)) )
			/( aivals.get(20*idx+30) + aivals.get((20*idx+10)) );
	     reflvals.setValue( idx, rval ); 
	}
	else
	    reflvals.setValue(idx,idx>0? reflvals.get(idx-1):reflvals.get(idx));
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

void WellTieGeoCalculator::stackWavelets( const TypeSet<float>& seisvals,
       					const TypeSet<float>& reflvals,
					TypeSet<float>& wvltvals)
{
    TypeSet<float> tmpreflvals, tmpseisvals, tmpwvltvals;
    const int wvltsize = wvltvals.size();
    const int wvlttotalnr = int (tmpseisvals.size()/wvltsize); 

    for ( int wvltnr=0; wvltnr<wvltnr; wvltnr+=wvlttotalnr )
    {
	for ( int idx=wvltnr; idx<wvltnr+wvltsize; idx++ )
	{
	    tmpreflvals += reflvals[idx];
	    tmpseisvals += seisvals[idx];
	}
	//deconvolve( tmpseisvals, tmpreflvals, tmpwvltvals );

	for ( int idwvlt=0; idwvlt<wvltsize; idwvlt++ )
	    wvltvals[idwvlt] += tmpwvltvals[idwvlt]/wvlttotalnr;
    }
}


void WellTieGeoCalculator::deconvolve( const Array1DImpl<float>& inputvals,
				    const Array1DImpl<float>& filtervals,
				    Array1DImpl<float>& deconvals )
{
    const int sz = inputvals.info().getSize(0);
    const int filtersz = filtervals.info().getSize(0);

    Array1DImpl<float> timeinputvals( sz );
    Array1DImpl<float> timefiltervals(filtersz );

    Array1DImpl<float_complex> ctimeinputvals( sz );
    Array1DImpl<float_complex> ctimefiltervals( filtersz );
    Array1DImpl<float_complex> cfreqinputvals( sz );
    Array1DImpl<float_complex> cfreqfiltervals( filtersz );
    Array1DImpl<float_complex> cfreqdeconvvals( filtersz );
    Array1DImpl<float_complex> ctimedeconvvals( filtersz );

    memcpy(timeinputvals.getData(), inputvals.arr(), filtersz*sizeof(float)); 
    memcpy(timefiltervals.getData(), filtervals.arr(), filtersz*sizeof(float)); 


    ArrayNDWindow window( Array1DInfoImpl(filtersz),
			 false, "CosTaper", 0.95 );
    window.apply( &timefiltervals );
    window.apply( &timeinputvals );
    removeBias( &timeinputvals );
    removeBias( &timefiltervals );

    HilbertTransform hil;
    hil.setCalcRange(0,filtersz,0);
    mDoTransform( hil, true, timeinputvals, ctimeinputvals, filtersz );
    hil.setCalcRange(0,filtersz,0);
    mDoTransform( hil, true, timefiltervals, ctimefiltervals, filtersz );

    FFT fft(false);
    mDoTransform( fft, true, ctimeinputvals, cfreqinputvals, filtersz );
    mDoTransform( fft, true, ctimefiltervals, cfreqfiltervals, filtersz );

    for ( int idx=1; idx<filtersz; idx++ )
    {
	float_complex inputval = cfreqinputvals.get(idx);
	float_complex filterval = cfreqfiltervals.get(idx);
	float_complex res = inputval/filterval;

	cfreqdeconvvals.setValue( idx, res );
    }
    
    mDoTransform( fft, false, cfreqdeconvvals, ctimedeconvvals, filtersz );

    float max = 0;
    for ( int idx=0; idx<filtersz; idx++ )
    {
	deconvals.setValue( idx, ctimedeconvvals.value(idx).real() );
	if ( deconvals[idx] > max)
	max = deconvals[idx];
    }
}


