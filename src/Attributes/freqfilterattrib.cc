/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          February 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: freqfilterattrib.cc,v 1.32 2009-05-27 09:40:50 cvshelene Exp $";


#include "freqfilterattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "timeser.h"
#include "ptrman.h"
#include "arrayndinfo.h"
#include "survinfo.h"

#include <math.h>


#define mFilterLowPass          0
#define mFilterHighPass         1
#define mFilterBandPass         2


#define mMINNRSAMPLES 		100

inline void smoothingArray( TypeSet<float>& shutdownarray )
{
    int idt = 0;
    for ( float idx= 0.95; idx<=1; idx+=0.001 )
    {
        float rx = idx;
        rx -= 0.95;
        rx *= 20;
        shutdownarray[idt]= (1 + cos( M_PI * rx )) * .5;
	idt++;
    }
}


inline void static FFTLowPass( float df, float maxfreq, 
			       const Array1DImpl<float_complex>& input,
			       Array1DImpl<float_complex>& output)
{
    const int arraysize = input.info().getTotalSz();
    TypeSet<float> filterborders(51,0);
    smoothingArray(filterborders);
    const int posmaxfreq = (int)(maxfreq/df);
    const int infthreshold = posmaxfreq - 25;
    const int supthreshold = posmaxfreq + 25;
    int idarray = 0;
    for ( int idx=0 ; idx<arraysize/2 ; idx++ )
    {
	float_complex outpval;
	if ( idx < infthreshold )
	    outpval = input.get(idx);
	else if ( idx > supthreshold )
	    outpval = 0;
	else
	{
	    outpval = input.get(idx)*filterborders[idarray]; 
	    idarray++;
	}
	output.set( idx,outpval );
    }
    idarray = 50;
    for ( int idx=arraysize/2; idx<arraysize; idx++ )
    {
	float_complex outpval;
	if ( idx > arraysize - infthreshold )
	    outpval = input.get(idx);
	else if ( idx < arraysize - supthreshold )
	    outpval = 0;
	else
	{
	    outpval = input.get(idx)*filterborders[idarray];
	    idarray--;
	}
	output.set( idx,outpval );
    }
}


inline void static FFTHighPass( float df, float minfreq,
			        const Array1DImpl<float_complex>& input,
			        Array1DImpl<float_complex>& output)
{
    const int arraysize = input.info().getTotalSz();
    TypeSet<float> filterborders(51,0);
    smoothingArray(filterborders);
    const int posminfreq = (int)(minfreq/df);
    const int infthreshold = posminfreq - 25;
    const int supthreshold = posminfreq + 25;
    int idarray = 50;
    for ( int idx=0 ; idx<arraysize/2 ; idx++ )
    {
	float_complex outpval;
	if ( idx < infthreshold )
	    outpval = 0;
	else if ( idx > supthreshold )
	    outpval = input.get(idx);
	else
	{
	    outpval = input.get(idx)*filterborders[idarray]; 
	    idarray--;
	}
	output.set( idx,outpval );
    }
    idarray = 0;
    for ( int idx=arraysize/2; idx<arraysize; idx++ )
    {
	float_complex outpval;
	if ( idx > arraysize - infthreshold )
	    outpval = 0;
	else if ( idx < arraysize - supthreshold )
	    outpval = input.get(idx);
	else
	{
	    outpval = input.get(idx)*filterborders[idarray];
	    idarray++;
	}
	output.set( idx,outpval );
    }
}


inline void static FFTBandPass( float df, float minfreq, float maxfreq,
				const Array1DImpl<float_complex>& input,
				Array1DImpl<float_complex>& output )
{
    const int arraysize = input.info().getTotalSz();
    TypeSet<float> filterborders(51,0);
    smoothingArray(filterborders);
    const int posmaxfreq = (int)(maxfreq/df);
    const int infmaxthreshold = posmaxfreq - 25;
    const int supmaxthreshold = posmaxfreq + 25;
    const int posminfreq = (int)(minfreq/df);
    const int infminthreshold = posminfreq - 25;
    const int supminthreshold = posminfreq + 25;
    int idarray = 0;
    int idarrayback = 50;
    for ( int idx=0 ; idx<arraysize/2 ; idx++ )
    {
	float_complex outpval;
	if ( idx < infmaxthreshold && idx > supminthreshold )
	    outpval = input.get(idx);
	else if ( idx > supmaxthreshold || idx < infminthreshold )
	    outpval = 0;
	else if ( idx >= infminthreshold && idx <= supminthreshold )
	{
	    outpval = input.get(idx)*filterborders[idarrayback]; 
	    idarrayback--;
	}
	else
	{
	    outpval = input.get(idx)*filterborders[idarray];
	    idarray++;
	}
	output.set( idx,outpval );
    }
    idarray = 0;
    idarrayback = 50;
    for ( int idx=arraysize/2; idx<arraysize; idx++ )
    {
	float_complex outpval;
	if ( idx <  arraysize - supminthreshold 
	     && idx > arraysize -infmaxthreshold )
	    outpval = input.get(idx);
	else if ( idx > arraysize - infminthreshold
		  || idx < arraysize - supmaxthreshold )
	    outpval = 0;
	else if ( idx >= arraysize - supmaxthreshold 
		  && idx <= arraysize -infmaxthreshold )
	{
	    outpval = input.get(idx)*filterborders[idarrayback];
	    idarrayback--;
	}
	else
	{
	    outpval = input.get(idx)*filterborders[idarray];
	    idarray++;
	}
	output.set( idx,outpval );
    }
}


namespace Attrib
{

mAttrDefCreateInstance(FreqFilter) 
   
void FreqFilter::initClass()
{
    mAttrStartInitClassWithUpdate

    //Note: Ordering must be the same as numbering!
    EnumParam* filtertype = new EnumParam( filtertypeStr() );
    filtertype->addEnum( filterTypeNamesStr(mFilterLowPass) );
    filtertype->addEnum( filterTypeNamesStr(mFilterHighPass) );
    filtertype->addEnum( filterTypeNamesStr(mFilterBandPass) );
    filtertype->setDefaultValue( mFilterLowPass );
    desc->addParam( filtertype );

    FloatParam* minfreq = new FloatParam( minfreqStr() );
    minfreq->setLimits( Interval<float>(0,mUdf(float)) );
    minfreq->setDefaultValue(15);
    desc->addParam( minfreq );

    FloatParam* maxfreq = new FloatParam( maxfreqStr() );
    maxfreq->setLimits( Interval<float>(0,mUdf(float)) );
    maxfreq->setDefaultValue(50);
    desc->addParam( maxfreq );

    IntParam* nrpoles = new IntParam( nrpolesStr() );
    nrpoles->setLimits( Interval<int>(2,20) );
    nrpoles->setDefaultValue( 4 );
    desc->addParam( nrpoles );

    BoolParam* isfftfilter = new BoolParam( isfftfilterStr() );
    isfftfilter->setDefaultValue( false );
    desc->addParam( isfftfilter );

    StringParam* window = new StringParam( windowStr() );
    window->setDefaultValue( "CosTaper" );
    desc->addParam( window );

    FloatParam* variable = new FloatParam( paramvalStr() );
    const float defval = 0.95;
    variable->setDefaultValue( defval );
    variable->setRequired( false );
    desc->addParam(variable);


    desc->addOutputDataType( Seis::UnknowData );

    desc->addInput( InputSpec("Real data",true) );
    desc->addInput( InputSpec("Imag data",true) );

    mAttrEndInitClass
}


void FreqFilter::updateDesc( Desc& desc )
{
    Attrib::ValParam* valpar = desc.getValParam( FreqFilter::paramvalStr() );
    Attrib::ValParam* winpar = desc.getValParam( FreqFilter::windowStr() );
    if ( !valpar || !winpar ) return;

    BufferString winstr = winpar->getStringValue();
    if ( winstr == "CosTaper5" )
    { winpar->setValue( "CosTaper" ); valpar->setValue( (float)0.95 ); }
    else if ( winstr == "CosTaper10" )
    { winpar->setValue( "CosTaper" ); valpar->setValue( (float)0.9 ); }
    else if ( winstr == "CosTaper20" )
    { winpar->setValue( "CosTaper" ); valpar->setValue( (float)0.8 ); }

    const ValParam* ftype = desc.getValParam( filtertypeStr() );
    const BufferString type = ftype->getStringValue();
    desc.setParamEnabled( minfreqStr(),
	    		  type != filterTypeNamesStr(mFilterLowPass) );
    desc.setParamEnabled( maxfreqStr(),
	    		  type != filterTypeNamesStr(mFilterHighPass) );

    const bool isfft = desc.getValParam(isfftfilterStr())->getBoolValue();
    desc.setParamEnabled( nrpolesStr(), !isfft );
    desc.inputSpec(1).enabled = isfft;
}


const char* FreqFilter::filterTypeNamesStr( int type )
{
    if ( type==mFilterLowPass ) return "LowPass";
    if ( type==mFilterHighPass ) return "HighPass";
    return "BandPass";
}


FreqFilter::FreqFilter( Desc& ds )
    : Provider( ds )
    , fftsz(-1)
    , fft(false)
    , fftinv(false)
    , signal(0)
    , timedomain(0)
    , freqdomain(0)
    , tmpfreqdomain(0)
    , timecplxoutp(0)
    , window_(0)
    , windowtype_(0)
    , variable_(0)
{
    if ( !isOK() ) return;

    mGetEnum( filtertype, filtertypeStr() );
    
    if ( filtertype==mFilterLowPass )
    {
	mGetFloat( maxfreq, maxfreqStr() );
    }
    else if ( filtertype==mFilterHighPass )
    {
	mGetFloat( minfreq, minfreqStr() );
    }
    else
    {
	mGetFloat( maxfreq, maxfreqStr() );
	mGetFloat( minfreq, minfreqStr() );
    }

    mGetInt( nrpoles, nrpolesStr() );
    mGetBool( isfftfilter, isfftfilterStr() );

    int wtype;
    mGetString( windowtype_, windowStr() );
    mGetFloat( variable_, paramvalStr() );

    if ( isfftfilter && strcmp(windowtype_,"None") )
	window_ = new ArrayNDWindow( Array1DInfoImpl(100),
				    false, windowtype_, variable_ );
	
    zmargin = Interval<int>( -mNINT(mMINNRSAMPLES/2), mNINT(mMINNRSAMPLES/2) );
}


FreqFilter::~FreqFilter()
{
    delete window_;
}


bool FreqFilter::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool FreqFilter::getInputData( const BinID& relpos, int idx )
{
    redata = inputs[0]->getData(relpos, idx);
    if ( !redata ) return false;

    realidx_ = getDataIndex( 0 );
    
    imdata = isfftfilter ? inputs[1]->getData(relpos, idx) : 0;
    imagidx_ = isfftfilter ? getDataIndex( 1 ) : -1;
    if ( isfftfilter && !imdata ) return false;

    return true;
}


bool FreqFilter::computeData( const DataHolder& output, const BinID& relpos,
				int z0, int nrsamples, int threadid ) const
{
    if ( isfftfilter )
	const_cast<FreqFilter*>(this)->fftFilter( output, z0, nrsamples );
    else
	const_cast<FreqFilter*>(this)->butterWorthFilter(output,z0,nrsamples);

    return true;
}


void FreqFilter::butterWorthFilter( const DataHolder& output,
				    int z0, int nrsamples )
{
    int nrsamp = nrsamples;
    int csamp = z0;
    if ( nrsamples < mMINNRSAMPLES )
    {
	nrsamp = mMINNRSAMPLES;
	csamp = z0 - mNINT(nrsamp/2) + mNINT(nrsamples/2);
    }

    ArrPtrMan<float> data = new float [nrsamp];
    ArrPtrMan<float> outp = new float [nrsamp];

    for ( int idx=0; idx<nrsamp; idx++ )
    {
	int reidx = idx + csamp - z0;
	int checkidx = idx + csamp - redata->z0_;
	int maxidx = redata->nrsamples_-1;
	data[idx] = checkidx<0 ? getInputValue( *redata, realidx_, 0, z0 )
			    : checkidx>maxidx 
			    	? getInputValue( *redata, realidx_, maxidx, z0 )
			        : getInputValue( *redata, realidx_, reidx, z0 );
    }

    if ( filtertype == mFilterLowPass )
    {
	float cutoff = refstep * maxfreq;
	BFlowpass( nrpoles, cutoff, nrsamp, data, outp );
    }
    else if ( filtertype == mFilterHighPass )
    {
	float cutoff = refstep * minfreq;
	BFhighpass( nrpoles, cutoff, nrsamp, data, outp );
    }
    else
    {
	float cutoff = refstep * maxfreq;
	ArrPtrMan<float> tmp = new float [nrsamp];
	BFlowpass( nrpoles, cutoff, nrsamp, data, tmp );
	cutoff = refstep * minfreq;
	BFhighpass( nrpoles, cutoff, nrsamp, tmp, outp );
    }
    
    if ( nrsamples < mMINNRSAMPLES )
    {
	int offset = mNINT(nrsamp/2) - mNINT(nrsamples/2);
	for ( int idx=0; idx<nrsamples; idx++ )
	    setOutputValue( output, 0, idx, z0, outp[offset - 1 + idx] );
    }
    else
    {
	float* out = output.series(0)->arr();
	memcpy(out,outp,nrsamp*sizeof(float));
    }
}


void FreqFilter::fftFilter( const DataHolder& output,
			    int z0, int nrsamples )
{
    int nrsamp = nrsamples;
    int z0safe = z0;
    if ( nrsamples < mMINNRSAMPLES )
    {
	nrsamp = mMINNRSAMPLES;
	z0safe = z0 - mNINT(nrsamp/2) + mNINT(nrsamples/2);
    }
    
    if ( !fft.isInit() || !fftinv.isInit() || nrsamp>signal.info().getSize(0))
    {
	fftsz = FFT::nearestBiggerFastSize(nrsamp*3);
	fft.setInputInfo( Array1DInfoImpl(fftsz) );
	fft.setDir(true);
	if ( fft.isInit() ) fft.setMeasure( false );
	fft.init();

	fftinv.setInputInfo( Array1DInfoImpl(fftsz) );
	fftinv.setDir(false);
	if ( fftinv.isInit() ) fftinv.setMeasure( false );
	fftinv.init();
	
	setSz(nrsamp);

	delete window_;
	if ( strcmp(windowtype_,"None") )
	    window_ = new ArrayNDWindow( Array1DInfoImpl(nrsamp),
					 false, windowtype_, variable_ );
    }

    for ( int idx=0; idx<fftsz; idx++ )
        timedomain.set( idx, 0 );

    const float df = FFT::getDf( refstep, fftsz);
    const int sz = fftsz/2 - nrsamp/2;
    for ( int idx=0; idx<nrsamp; idx++ )
    {
	int csamp = idx + z0safe;
	int cidx = csamp - z0;
	int remaxidx = redata->nrsamples_-1;
	int checkridx = csamp - redata->z0_;
	float real = checkridx<0 
			 ? getInputValue( *redata, realidx_, 0, z0 )
			 : checkridx>remaxidx 
			    ? getInputValue( *redata, realidx_, remaxidx, z0 )
			    : getInputValue( *redata, realidx_, cidx, z0 );
	if ( mIsUdf(real) )
	    real = idx>0 ? signal.get(idx-1).real() : 0;
	int immaxidx = imdata->nrsamples_-1;
	int checkiidx = csamp - imdata->z0_;
	const float imag = checkiidx<0 
	    		? -getInputValue( *imdata, imagidx_, 0, z0 )
			: checkiidx>immaxidx
			    ? -getInputValue( *imdata, imagidx_, immaxidx, z0 )
			    : -getInputValue( *imdata, imagidx_, cidx, z0 );
	signal.set( idx, float_complex(real,imag) );
    }

    if ( window_ ) window_->apply( &signal );
    float avg = computeAvg( &signal ).real();
    removeBias( &signal );
    for ( int idy=0; idy<nrsamp; idy++ )
	timedomain.set( sz+idy, signal.get(idy) );

    fft.transform( timedomain, freqdomain );
    if ( filtertype == mFilterLowPass )
	FFTLowPass( df, maxfreq, freqdomain, tmpfreqdomain );
    else if ( filtertype == mFilterHighPass)
	FFTHighPass( df, minfreq, freqdomain, tmpfreqdomain );
    else
	FFTBandPass( df, minfreq, maxfreq, freqdomain, tmpfreqdomain );

    fftinv.transform( tmpfreqdomain, timecplxoutp );

    const int firstidx = nrsamples < mMINNRSAMPLES ? fftsz/2 - nrsamples/2: sz;
    bool needrestorebias = filtertype==mFilterLowPass
			   || ( filtertype==mFilterBandPass && minfreq==0 );
    float correctbias = needrestorebias ? avg : 0;
    for ( int idx=0; idx<nrsamples; idx++ )
	setOutputValue( output, 0, idx, z0,
			timecplxoutp.get(firstidx+idx).real()+correctbias );
}


void FreqFilter::setSz( int sz )
{
    signal.setInfo( Array1DInfoImpl( sz ) );
    timedomain.setInfo( Array1DInfoImpl( fftsz ) );
    freqdomain.setInfo( Array1DInfoImpl( fftsz ) );
    tmpfreqdomain.setInfo( Array1DInfoImpl( fftsz ) );
    timecplxoutp.setInfo( Array1DInfoImpl( fftsz ) );
}


const Interval<int>* FreqFilter::desZSampMargin(int input, int output) const
{
    return &zmargin;
}

};//namespace
