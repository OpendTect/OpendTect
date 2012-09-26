/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "expwavelet1d.h"
#include "attribprovider.h"
#include "seistrc.h"
#include "interpol.h"
#include "arrayndimpl.h"
#include "ptrman.h"
#include "simpnumer.h"


DefineEnumNames(Wavelet1DAttrib, WaveletLen,0,"WaveletLen")
{	  "2"
	, "4"
	, "8"
	, "16"
	, "32"
	, "64"
	, "128"
	, "256"
	, "512"
	, "1024"
	, "2048"
	, "4196"
	, 0
};


Wavelet1DAttrib::Wavelet1DAttrib( Parameters* param)
    : minwaveletlen( (Wavelet1DAttrib::WaveletLen) ((int) param->minwaveletlen))
    , maxwaveletlen( (Wavelet1DAttrib::WaveletLen) ((int) param->maxwaveletlen))
    , wavelet( (WaveletTransform::WaveletType) ((int) param->wavelet) )
    , AttribCalc( new Wavelet1DAttrib::Task( *this ) )
{ 
    param->fillDefStr( desc );
    delete param;

    WaveletLen tmp;
    if ( maxwaveletlen<minwaveletlen ) mSWAP(minwaveletlen, maxwaveletlen, tmp);
    scalelen = intpow( 2, maxwaveletlen );
    dsg = Interval<int>( -(scalelen-1), (scalelen-1) );

    AttribInputSpec* spec = new AttribInputSpec;
    spec->setDesc("Data which should be analysed");
    inputspec += spec;

    prepareInputs();
}


Wavelet1DAttrib::~Wavelet1DAttrib()
{  ; }


int Wavelet1DAttrib::Task::nextStep()
{
    Wavelet1DAttrib::Task::Input* inp = (Wavelet1DAttrib::Task::Input*)input;
    const SeisTrc* trc = inp->trc;
    const float inpstep = trc->info().sampling.step;
    const int component = inp->attrib;

    const int scalelen =  calculator.scalelen;

    int len = nrtimes + scalelen;
    while ( !isPower( len, 2 ) ) len++;

    Array1DImpl<float> signal( len );
    int off = (len-nrtimes)/2;


    for ( int idx=0; idx<len; idx++ )
    {
	signal.set(idx, trc->getValue(t1+(idx-off)*inpstep, component) );
    }

    Array1DImpl<float> transformed( len );
    DWT transform( calculator.wavelet );

    transform.setInputInfo( signal.info() );
    transform.init();
    transform.transform( signal, transformed );

    const int nrscales = isPower( len, 2 ) + 1;
    ArrPtrMan<float> spectrum =  new float[nrscales];
    spectrum[0] = fabs(transformed.get(0)); // scale 0 (dc)
    spectrum[1] = fabs(transformed.get(1)); // scale 1 

    for ( int idx=0; idx<nrtimes; idx++ )
    {
	for ( int scale=2; scale<nrscales; scale++ )
	{
	    int scalepos = intpow(2,scale-1) + ((idx+off) >> (nrscales-scale));
	    spectrum[scale] = fabs(transformed.get(scalepos));
	}

	int maxid = -1;
	float max = 0;

	float sum = 0;
	float wsum = 0;
	float sqsum = 0;
	float wsqsum = 0;

	float sumbeforemax = 0;
	float sumaftermax = 0;

	int nrstatscales = 0;

	int firstscale = nrscales-calculator.maxwaveletlen-1; // Reversed order
	int lastscale = nrscales-calculator.minwaveletlen-1; // Reversed order
	// The numbering of scales is not straighforward.
	// The transform and all literature counts the scales
	// from 0 (dc) to M (where N=2^(M+1)=nr samples). The user,
	// does not know how long the transform will be, and will thus
	// specify the scales as the length of the wavelet, i.e.
	// waveletlength = 2^(userscale+1). The conversion between the two
	// formats are done above.

	for ( int scale=firstscale; scale<=lastscale; scale++ )
	{
	    nrstatscales++;
	    float val = spectrum[scale];
	    sum += val;
	    sqsum += val*val;
	    wsum += scale*val;
	    wsqsum += scale*val*val;
	    if ( val>max ) { maxid=scale; max=val; }
	    else if ( maxid==-1 ) sumbeforemax += val;
	    else sumaftermax += val;
	}

	if ( outp[0] ) (outp[0])[idx] = wsum/sum;
	if ( outp[1] ) (outp[1])[idx] = wsqsum/sqsum;
	if ( outp[2] ) (outp[2])[idx] = maxid;
	if ( outp[3] )
	{
	    float scale = wsum/sum;

	    int intoff = (int) scale;
	    float pos = scale - intoff;

	    float res = polyInterpolate(
			intoff ? spectrum[intoff-1] : mUndefValue,
			intoff > lastscale ? mUndefValue :spectrum[intoff],
			intoff+1 > lastscale ? mUndefValue :spectrum[intoff+1],
			intoff+2 > lastscale ? mUndefValue :spectrum[intoff+2],
			pos );

	    if ( mIsUndefined( res ) ) res = 0;
	    (outp[3])[idx] = res/sum;
	}

	if ( outp[4] ) (outp[4])[idx] = spectrum[maxid];

	if ( outp[5] )
	{
	    float scale = wsqsum/sqsum;

	    int intoff = (int) scale;
	    float pos = scale - intoff;

	    float res = polyInterpolate(
			intoff ? spectrum[intoff-1] : mUndefValue,
			intoff > lastscale ? mUndefValue :spectrum[intoff],
			intoff+1 > lastscale ? mUndefValue :spectrum[intoff+1],
			intoff+2 > lastscale ? mUndefValue :spectrum[intoff+2],
			pos );

	    if ( mIsUndefined( res ) ) res = 0;
	    (outp[5])[idx] = res/sum;
	}

	if ( outp[6] ) (outp[6])[idx] = sumbeforemax/sum;
	if ( outp[7] ) (outp[7])[idx] = sumaftermax/sum;
	if ( outp[8] ) (outp[8])[idx] = Math::Sqrt(
		(sqsum-sum*sum/nrstatscales) / (nrstatscales-1));

    }

    return 0;
}


AttribCalc::Task*  Wavelet1DAttrib::Task::clone() const 
{ return new Wavelet1DAttrib::Task(calculator); }


bool Wavelet1DAttrib::Task::Input::set(const BinID& pos, 
	const ObjectSet<AttribProvider>& inp, const TypeSet<int>& inpattrib,
			     const TypeSet<float*>&)
{
    trc = inp[0]->getTrc( pos.inl, pos.crl ); 

    attrib = inp[0]->attrib2component( inpattrib[0] );
    return trc ? true : false;
} 


