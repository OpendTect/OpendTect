/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiegeocalculator.cc,v 1.59 2011-05-02 14:25:45 cvsbruno Exp $";


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
#include "wavelet.h"
#include "welllog.h"
#include "welllogset.h"
#include "welltrack.h"
#include "welltieunitfactors.h"
#include "welld2tmodel.h"

#include <complex>

namespace WellTie
{

Well::D2TModel* GeoCalculator::getModelFromVelLog( const Well::Log& log, 
					    const Well::Track* track, 
					    float surfelev, bool issonic ) const
{
    float rdelev = 0;
    if ( track && !track->isEmpty() )
    {
	rdelev = track->dah( 0 ) - track->value( 0 );
	if ( mIsUdf( rdelev ) ) rdelev = 0;
    }

    Well::Log proclog = Well::Log( log );
    if ( track )
    {
	proclog.insertAtDah( -track->value(0), 0 );
    }
    removeSpikes( proclog.valArr(), proclog.size(), 10, 3 );
    velLogConv( proclog, issonic ? Son2TWT : Vel2TWT );

    TypeSet<float> dpt, vals;

    for ( int idx=0; idx<proclog.size(); idx++ )
    {
	float dah = proclog.dah( idx );
	if ( mIsUdf( dah ) ) continue;
	dpt += dah;
	vals += proclog.getValue( dah, true );
    }
    Well::D2TModel* d2tnew = new Well::D2TModel;
    d2tnew->add( rdelev - surfelev, 0 ); //set KB Depth
    for ( int idx=0; idx<dpt.size(); idx++ )
	d2tnew->add( dpt[idx]-surfelev, vals[idx] );

    return d2tnew;
}


void GeoCalculator::ensureValidD2TModel( Well::D2TModel& d2t ) const
{
    const int sz = d2t.size();
    TypeSet<float> dahs, times;
    mAllocVarLenArr( int, zidxs, sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	dahs += d2t.dah( idx ); 
	times += d2t.value( idx ); 
	zidxs[idx] = idx;
    }
    sort_coupled( times.arr(), mVarLenArr(zidxs), sz );
    d2t.erase();
    d2t.add( dahs[zidxs[0]], times[zidxs[0]] );
    for ( int idx=1; idx<sz; idx++ )
    {
	int idx0 = zidxs[idx-1]; 
	int idx1 = zidxs[idx];
	if ( dahs[idx1] >= dahs[idx0] && dahs[idx1] > dahs[0] && times[idx1]>0 )
	    d2t.add( dahs[idx1], times[idx1] );
    }
}


void GeoCalculator::velLogConv( Well::Log& log, Conv conv ) const
{
    const int sz = log.size(); 
    if ( sz < 2 ) return;
    const bool issonic = conv == Son2Vel || conv == Son2TWT;
    UnitFactors uf; double velfac = uf.getVelFactor( log, issonic );
    if ( !velfac ) velfac = 1;
    TypeSet<float> dpts, vals;
    for ( int idx=0; idx<log.size(); idx++ )
	{ dpts += log.dah( idx ); vals += log.value( idx ); }
    log.erase();
    float prevval, newval; newval = prevval = 0;
    for ( int idx=1; idx<sz; idx++ )
    {
	if ( conv == Vel2TWT || conv == Son2TWT )
	{
	    float v = vals[idx]; 
	    if ( conv == Son2TWT ) v = 1/v;
	    newval = 2*( dpts[idx] - dpts[idx-1] )/(v*velfac);
	    newval += prevval;
	    prevval = newval;
	}
	else if ( conv == TWT2Vel )
	{
	    newval = 2*( dpts[idx] - dpts[idx-1] )/( vals[idx]-vals[idx-1] );
	}
	else if ( conv == Son2Vel )
	{
	    newval = velfac/vals[idx];
	}
	else if ( conv == Vel2Son )
	{
	    newval = 1/vals[idx]*velfac;
	}
	log.addValue( dpts[idx], newval );
    }
}


void GeoCalculator::stretch( WellTie::GeoCalculator::StretchData& sd ) const
{
    sd.stretchfac_ = (sd.pick2_-sd.start_)/(float)(sd.pick1_-sd.start_);
    sd.isstretch_ = true;
    stretch( sd, sd.stretchfac_ ); 

    sd.isstretch_ = false;
    sd.squeezefac_ = (sd.stop_-sd.pick2_ )/(float)(sd.stop_-sd.pick1_);
    stretch( sd, sd.squeezefac_ );
}


void GeoCalculator::stretch( const WellTie::GeoCalculator::StretchData& sd, 
			    float factor ) const
{
    int start = sd.isstretch_ ? sd.start_ : sd.pick2_; 
    int stop = sd.isstretch_ ? sd.pick2_ : sd.stop_; 
    const int datasz = sd.inp_->info().getSize(0);
    for ( int idx=start; idx<stop; idx++ )
    {
	float v = sd.isstretch_ ? sd.start_ : sd.stop_;
	const float curval = Interpolate::linearReg1D( v, (float)idx, factor );
	const int curidx = (int) curval;
	if ( curidx >= datasz-1 || curidx < 0 ) continue;
	const float newval = Interpolate::linearReg1D( sd.inp_->get(curidx),
	sd.inp_->get(curidx+1),
	curval-curidx);
	sd.outp_->setValue( idx , newval );
    }
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



#define mDoFourierTransform(tf,isstraight,inp,outp,sz) \
{   \
    tf->setInputInfo(Array1DInfoImpl(sz));\
    tf->setDir(isstraight);\
    tf->setNormalization(!isstraight); \
    tf->setInput(inp->getData());\
    tf->setOutput(outp->getData());\
    tf->run(true); \
}

#define mNoise 0.05
void GeoCalculator::deconvolve( const float* inp, const float* filter,
			        float* deconvals, int inpsz ) const
{
    ArrayNDWindow window( Array1DInfoImpl(inpsz), false, "CosTaper", 0.1 );
    Array1DImpl<float>* inputvals = new Array1DImpl<float>( inpsz );
    Array1DImpl<float>* filtervals = new Array1DImpl<float>( inpsz );
    memcpy(inputvals->getData(),inp,inpsz*sizeof(float));
    memcpy(filtervals->getData(),filter,inpsz*sizeof(float));
    window.apply( inputvals );		removeBias( inputvals );
    window.apply( filtervals );		removeBias( filtervals );

    HilbertTransform hil;
    hil.setCalcRange(0, inpsz, 0);
    Array1DImpl<float_complex>* cinputvals = 
				new Array1DImpl<float_complex>( inpsz );
    for ( int idx=0; idx<inpsz; idx++ )
	cinputvals->set( idx, inputvals->get( idx ) );
    delete inputvals;
    Array1DImpl<float_complex>* cfiltervals = 
				new Array1DImpl<float_complex>( inpsz );
    for ( int idx=0; idx<inpsz; idx++ )
	cfiltervals->set( idx, filtervals->get( idx ) );
    delete filtervals;
   
    PtrMan<Fourier::CC> fft = Fourier::CC::createDefault();
    Array1DImpl<float_complex>* cfreqinputvals = 
				new Array1DImpl<float_complex>( inpsz );
    mDoFourierTransform( fft, true, cinputvals, cfreqinputvals, inpsz );
    delete cinputvals;
    Array1DImpl<float_complex>* cfreqfiltervals = 
				new Array1DImpl<float_complex>( inpsz );
    mDoFourierTransform( fft, true, cfiltervals, cfreqfiltervals, inpsz );

    Spectrogram spec;
    Array1DImpl<float_complex>* cspecfiltervals = 
				new Array1DImpl<float_complex>( inpsz );
    for ( int idx=0; idx<inpsz; idx++ )
	cspecfiltervals->set( idx, cfiltervals->get( idx ) );
    delete cfiltervals;

    float_complex wholespec = 0;
    float_complex noise = mNoise/inpsz;
    for ( int idx=0; idx<inpsz; idx++ )
	wholespec += cspecfiltervals->get( idx );  
    float_complex cnoiseshift = noise*wholespec;
   
    Array1DImpl<float_complex>* cdeconvvals = 
				new  Array1DImpl<float_complex>( inpsz ); 
    for ( int idx=0; idx<inpsz; idx++ )
    {
	float_complex inputval = cfreqinputvals->get(idx);
	float_complex filterval = cfreqfiltervals->get(idx);

	double rfilterval = filterval.real();
	double ifilterval = filterval.imag();
	float_complex conjfilterval = float_complex( rfilterval ,-ifilterval ); 

	float_complex num = inputval * conjfilterval;
	float_complex denom = filterval * conjfilterval + cnoiseshift;
	float_complex res = num / denom;

	cdeconvvals->setValue( idx, res );
    }
    delete cfreqinputvals; delete  cfreqfiltervals;

    float avg = 0;
    for ( int idx=0; idx<inpsz; idx++ )
	avg += abs( cdeconvvals->get( idx ) )/inpsz;
    for ( int idx=0; idx<inpsz; idx++ )
    {
	if ( abs( cdeconvvals->get( idx ) ) < avg/4 )
	    cdeconvvals->set( idx, 0 );
    }

    Array1DImpl<float_complex>* ctimedeconvvals = 
				new Array1DImpl<float_complex>( inpsz );
    mDoFourierTransform( fft, false, cdeconvvals, ctimedeconvvals, inpsz );
    delete cdeconvvals;

    int mid = (int)(inpsz)/2;
    for ( int idx=0; idx<=mid; idx++ )
	deconvals[idx] = ctimedeconvvals->get( mid-idx ).real();;
    for ( int idx=mid+1; idx<inpsz; idx++ )
	deconvals[idx] = ctimedeconvvals->get( inpsz-idx+mid ).real();
    delete ctimedeconvvals;
}


double GeoCalculator::crossCorr( const float* seis, const float* synth, 
				float* outp, int sz ) const
{
    genericCrossCorrelation( sz, 0, seis, sz, 0, synth, sz, -sz/2, outp );
    LinStats2D ls2d; ls2d.use( seis, synth, sz );
    return ls2d.corrcoeff;
}


int GeoCalculator::getIdx( const Array1DImpl<float>& inp, float pos) const 
{
    int idx = 0;
    while ( inp.get(idx)<pos )
    {
	if( idx == inp.info().getSize(0)-1 )
	    break;
	idx++;
    }
    return idx;
}


void GeoCalculator::d2TModel2Log( const Well::D2TModel& d2t, 
					Well::Log& log ) const
{
    log.erase();
    for ( int idx=0; idx<d2t.size(); idx++ )
	log.addValue( d2t.dah( idx ), d2t.value( idx ) );
}

}; //namespace WellTie
