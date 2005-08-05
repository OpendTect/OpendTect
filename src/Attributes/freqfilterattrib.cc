/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          February 2003
 RCS:           $Id: freqfilterattrib.cc,v 1.5 2005-08-05 13:05:01 cvsnanne Exp $
________________________________________________________________________

-*/


#include "freqfilterattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "datainpspec.h"
#include "timeser.h"
#include "ptrman.h"
#include "arrayndinfo.h"

#include <math.h>


#define mFilterLowPass          0
#define mFilterHighPass         1
#define mFilterBandPass         2


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
    
void FreqFilter::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    EnumParam* filtertype = new EnumParam(filtertypeStr());
    //Note: Ordering must be the same as numbering!
    
    filtertype->addEnum(filterTypeNamesStr(mFilterLowPass));
    filtertype->addEnum(filterTypeNamesStr(mFilterHighPass));
    filtertype->addEnum(filterTypeNamesStr(mFilterBandPass));
    filtertype->setDefaultValue("0");
    desc->addParam(filtertype);

    FloatParam* minfreq = new FloatParam( minfreqStr() );
    minfreq->setLimits( Interval<float>(0,mUndefValue) );
    desc->addParam( minfreq );

    FloatParam* maxfreq = new FloatParam( maxfreqStr() );
    maxfreq->setLimits( Interval<float>(0,mUndefValue) );
    desc->addParam( maxfreq );

    IntParam* nrpoles = new IntParam( nrpolesStr() );
    nrpoles->setLimits( Interval<int>(2,20) );
    nrpoles->setDefaultValue("4");
    desc->addParam( nrpoles );

    BoolParam* isfftfilter = new BoolParam( isfftfilterStr() );
    isfftfilter->setDefaultValue(false);
    desc->addParam( isfftfilter );

    EnumParam* window = new EnumParam(windowStr());
    window->addEnums(ArrayNDWindow::WindowTypeNames);
    window->setDefaultValue("CosTaper5");
    desc->addParam(window);

    desc->addOutputDataType( Seis::UnknowData );

    desc->addInput( InputSpec("Real data",true) );
    desc->addInput( InputSpec("Imag data",true) );

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* FreqFilter::createInstance( Desc& ds )
{
    FreqFilter* res = new FreqFilter( ds );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void FreqFilter::updateDesc( Desc& desc )
{
    const ValParam* ftype = (ValParam*)desc.getParam(filtertypeStr());
    if ( !strcmp(ftype->getStringValue(0),filterTypeNamesStr(mFilterLowPass)))
    {
	desc.setParamEnabled(minfreqStr(),false);
	desc.setParamEnabled(maxfreqStr(),true);
    }
    else if ( !strcmp( ftype->getStringValue(0),
			filterTypeNamesStr(mFilterHighPass) ) )
    {
	desc.setParamEnabled(minfreqStr(),true);
	desc.setParamEnabled(maxfreqStr(),false);
    }
    else
    {
	desc.setParamEnabled(minfreqStr(),true);
	desc.setParamEnabled(maxfreqStr(),true);
    }

    bool isfft = ((ValParam*)desc.getParam(isfftfilterStr()))->getBoolValue();
    desc.inputSpec(1).enabled = isfft;
}

const char* FreqFilter::filterTypeNamesStr(int type)
{
    if ( type==mFilterLowPass ) return "LowPass";
    if ( type==mFilterHighPass ) return "HighPass";
    return "BandPass";
}


FreqFilter::FreqFilter( Desc& desc_ )
    : Provider( desc_ )
    , fftsz(-1)
    , fft(false)
    , fftinv(false)
    , signal(0)
    , timedomain(0)
    , freqdomain(0)
    , tmpfreqdomain(0)
    , timecplxoutp(0)
    , window(0)
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
    mGetEnum( wtype, windowStr() );
    windowtype = (ArrayNDWindow::WindowType)wtype;

    if ( isfftfilter )
	window = new ArrayNDWindow( Array1DInfoImpl(100),
		 false, (ArrayNDWindow::WindowType) windowtype );
}


FreqFilter::~FreqFilter()
{
    delete window;
}


bool FreqFilter::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool FreqFilter::getInputData(const BinID& relpos, int idx)
{
    redata = inputs[0]->getData(relpos, idx);
    if ( !redata ) return false;
    
    imdata = isfftfilter ? inputs[1]->getData(relpos, idx) : 0;
    if ( isfftfilter && !imdata ) return false;

    return true;
}


bool FreqFilter::computeData( const DataHolder& output, const BinID& relpos,
				int t0, int nrsamples ) const
{
    if ( isfftfilter )
	const_cast<FreqFilter*>(this)->fftFilter( output, t0, nrsamples );
    else
	const_cast<FreqFilter*>(this)->butterWorthFilter(output,t0,nrsamples);

    return true;
}


void FreqFilter::butterWorthFilter( const DataHolder& output,
				    int t0, int nrsamples )
{
    const int minnrsamples = 100;
	
    int nrsamp = nrsamples;
    float curt = t0;
    if ( nrsamples == 1 )
    {
	nrsamp = minnrsamples;
	curt = t0 - refstep*nrsamp/2;
    }

    ArrPtrMan<float> data = new float [nrsamp];
    ArrPtrMan<float> outp = new float [nrsamp];

    for ( int idx=0; idx<nrsamp; idx++ )
    {
	data[idx] = redata->item(0)->value( idx - redata->t0_ );
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

    if ( nrsamples == 1 )
	output.item(0)->setValue( 0, outp[nrsamp/2 - 1] );
    else
    {
	float* out = output.item(0)->arr();
	memcpy(out,outp,nrsamp*sizeof(float));
    }
}


void FreqFilter::fftFilter( const DataHolder& output,
			    int t0, int nrsamples )
{
    if ( !fft.isinit() || !fftinv.isinit() )
    {
	fftsz = FFT::_getNearBigFastSz(nrsamples*3);
	fft.setInputInfo( Array1DInfoImpl(fftsz) );
	fft.setDir(true);
	fft.init();

	fftinv.setInputInfo( Array1DInfoImpl(fftsz) );
	fftinv.setDir(false);
	fftinv.init();
	
	setSz(nrsamples);

	delete window;
	window = new ArrayNDWindow( Array1DInfoImpl(nrsamples),
	                 false, (ArrayNDWindow::WindowType) windowtype );
    }

    for ( int idx=0; idx<fftsz; idx++ )
        timedomain.set( idx, 0 );

    const float df = FFT::getDf( refstep, fftsz);
    const int sz = fftsz/2 - nrsamples/2;
    for ( int idx=0; idx<nrsamples; idx++ )
    {
        const float real = redata->item(0)->value( idx - redata->t0_ );
        const float imag = -imdata->item(0)->value( idx - imdata->t0_ );
        signal.set( idx, float_complex( real, imag ));
    }

    window->apply( &signal );
    removeBias( &signal );
    for ( int idy=0; idy<nrsamples; idy++ )
    {
	timedomain.set( sz+idy, signal.get(idy) );
    }
      
    fft.transform( timedomain, freqdomain );
    if ( filtertype == mFilterLowPass )
	FFTLowPass( df, maxfreq, freqdomain, tmpfreqdomain );
    else if ( filtertype == mFilterHighPass)
	FFTHighPass( df, minfreq, freqdomain, tmpfreqdomain );
    else
	FFTBandPass( df, minfreq, maxfreq, freqdomain, tmpfreqdomain );

    fftinv.transform( tmpfreqdomain, timecplxoutp );

    for ( int idx = 0; idx<nrsamples; idx++ )
	output.item(0)->setValue( idx, timecplxoutp.get(sz+idx).real() );
}


void FreqFilter::setSz( int sz )
{
    signal.setInfo( Array1DInfoImpl( sz ) );
    int fftsz = FFT::_getNearBigFastSz( sz*3 );
    timedomain.setInfo( Array1DInfoImpl( fftsz ) );
    freqdomain.setInfo( Array1DInfoImpl( fftsz ) );
    tmpfreqdomain.setInfo( Array1DInfoImpl( fftsz ) );
    timecplxoutp.setInfo( Array1DInfoImpl( fftsz ) );
}

};//namespace
