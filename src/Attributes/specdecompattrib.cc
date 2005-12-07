/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          January 2004
 RCS:           $Id: specdecompattrib.cc,v 1.11 2005-12-07 11:26:05 cvshelene Exp $
________________________________________________________________________

-*/

#include "specdecompattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "datainpspec.h"
#include "genericnumer.h"
#include "survinfo.h"
#include "envvars.h"

#include <math.h>
#include <complex>


#define mTransformTypeFourier		0
#define mTransformTypeDiscrete  	1
#define mTransformTypeContinuous	2

namespace Attrib
{

void SpecDecomp::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    EnumParam* ttype = new EnumParam(transformTypeStr());
    //Note: Ordering must be the same as numbering!
    ttype->addEnum(transTypeNamesStr(mTransformTypeFourier));
    ttype->addEnum(transTypeNamesStr(mTransformTypeDiscrete));
    ttype->addEnum(transTypeNamesStr(mTransformTypeContinuous));
    ttype->setDefaultValue("0");
    desc->addParam(ttype);

    EnumParam* window = new EnumParam(windowStr());
    window->addEnums(ArrayNDWindow::WindowTypeNames);
    window->setRequired(false);
    window->setDefaultValue("CosTaper5");
    window->setValue( 0 );
    desc->addParam(window);

    ZGateParam* gate = new ZGateParam(gateStr());
    gate->setLimits( Interval<float>(-mLargestZGate,mLargestZGate) );
    desc->addParam( gate );

    FloatParam* deltafreq = new FloatParam( deltafreqStr() );
    deltafreq->setRequired(false);
    deltafreq->setDefaultValue("5");
    desc->addParam( deltafreq );

    EnumParam* dwtwavelet = new EnumParam(dwtwaveletStr());
    dwtwavelet->addEnums(WaveletTransform::WaveletTypeNames);
    dwtwavelet->setDefaultValue("Haar");
    desc->addParam(dwtwavelet);

    EnumParam* cwtwavelet = new EnumParam(cwtwaveletStr());
    cwtwavelet->addEnums(CWT::WaveletTypeNames);
    cwtwavelet->setDefaultValue("Morlet");
    desc->addParam(cwtwavelet);

    desc->addOutputDataType( Seis::UnknowData );

    desc->addInput( InputSpec("Real data",true) );
    desc->addInput( InputSpec("Imag data",true) );

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* SpecDecomp::createInstance( Desc& ds )
{
    SpecDecomp* res = new SpecDecomp( ds );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void SpecDecomp::updateDesc( Desc& desc )
{
    BufferString type = desc.getValParam(transformTypeStr())->getStringValue();
    const bool isfft = type == transTypeNamesStr( mTransformTypeFourier );
    desc.setParamEnabled( gateStr(), isfft );
    desc.setParamEnabled( windowStr(), isfft );

    const bool isdwt = type == transTypeNamesStr( mTransformTypeDiscrete );
    desc.setParamEnabled( dwtwaveletStr(), isdwt );

    const bool iscwt = type == transTypeNamesStr( mTransformTypeContinuous );
    desc.setParamEnabled( cwtwaveletStr(), iscwt );

    //HERE see what to do when SI().zStep() != refstep !!!
    float dfreq;
    mGetFloat( dfreq, deltafreqStr() );
    const float nyqfreq = 0.5 / SI().zStep();
    const int nrattribs = mNINT( nyqfreq / dfreq );
    desc.setNrOutputs( Seis::UnknowData, nrattribs );
}


const char* SpecDecomp::transTypeNamesStr(int type)
{
    if ( type==mTransformTypeFourier ) return "DFT";
    if ( type==mTransformTypeDiscrete ) return "DWT";
    return "CWT";
}


SpecDecomp::SpecDecomp( Desc& desc_ )
    : Provider( desc_ )
    , window(0)
    , fftisinit(false)
    , timedomain(0)
    , freqdomain(0)
    , signal(0)
    , scalelen(0)
{ 
    if ( !isOK() ) return;

    mGetEnum( transformtype, transformTypeStr() );
    mGetFloat( deltafreq, deltafreqStr() );

    if ( transformtype == mTransformTypeFourier )
    {
	int wtype;
	mGetEnum( wtype, windowStr() );
	windowtype = (ArrayNDWindow::WindowType)wtype;
	
	mGetFloatInterval( gate, gateStr() );
	gate.start = gate.start / zFactor(); gate.stop = gate.stop / zFactor();
    }
    else if ( transformtype == mTransformTypeDiscrete )
    {
	int dwave;
	mGetEnum( dwave, dwtwaveletStr() );
	dwtwavelet = (WaveletTransform::WaveletType) dwave;
    }
    else 
    {
	int cwave;
	mGetEnum( cwave, cwtwaveletStr() );
	cwtwavelet = (CWT::WaveletType) cwave;
    }
}


SpecDecomp::~SpecDecomp()
{
    delete window;
}


bool SpecDecomp::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool SpecDecomp::getInputData( const BinID& relpos, int idx )
{
    redata = inputs[0]->getData( relpos, idx );
    if ( !redata ) return false;

    imdata = inputs[1]->getData( relpos, idx );
    if ( !imdata ) return false;

    realidx_ = getDataIndex( 0 );
    imagidx_ = getDataIndex( 1 );
    
    return true;
}


bool SpecDecomp::computeData( const DataHolder& output, const BinID& relpos,
	                          int z0, int nrsamples ) const
{
    if ( !fftisinit )
    {
	if ( transformtype == mTransformTypeFourier )
	{
	    const_cast<SpecDecomp*>(this)->samplegate = 
	     Interval<int>(mNINT(gate.start/refstep), mNINT(gate.stop/refstep));
	    const_cast<SpecDecomp*>(this)->sz = samplegate.width()+1;

	    const float fnyq = 0.5 / refstep;
	    const int minsz = mNINT( 2*fnyq/deltafreq );
	    const_cast<SpecDecomp*>(this)->fftsz = sz > minsz ? sz : minsz;
	    const_cast<SpecDecomp*>(this)->
			fft_.setInputInfo(Array1DInfoImpl(fftsz));
	    const_cast<SpecDecomp*>(this)->fft_.setDir(true);
	    const_cast<SpecDecomp*>(this)->fft_.init();
	    const_cast<SpecDecomp*>(this)->df = FFT::getDf( refstep, fftsz );

	    const_cast<SpecDecomp*>(this)->window = 
		new ArrayNDWindow( Array1DInfoImpl(sz), false, 
			(ArrayNDWindow::WindowType)windowtype );
	}
	else
	    const_cast<SpecDecomp*>(this)->scalelen = 1024;

	const_cast<SpecDecomp*>(this)->fftisinit = true;
    }
    
    if ( transformtype == mTransformTypeFourier )
    {
	const_cast<SpecDecomp*>(this)->
	    		signal = new Array1DImpl<float_complex>( sz );

	const_cast<SpecDecomp*>(this)->
	    timedomain = new Array1DImpl<float_complex>( fftsz );
	const int tsz = timedomain->info().getTotalSz();
	memset(timedomain->getData(),0, tsz*sizeof(float_complex));

	const_cast<SpecDecomp*>(this)->
			freqdomain = new Array1DImpl<float_complex>( fftsz );
    }
    
    bool res;
    if ( transformtype == mTransformTypeFourier )
	res = calcDFT(output, z0, nrsamples);
    else if ( transformtype == mTransformTypeDiscrete )
	res = calcDWT(output, z0, nrsamples);
    else if ( transformtype == mTransformTypeContinuous )
	res = calcCWT(output, z0, nrsamples);

    
    delete signal;
    delete timedomain;
    delete freqdomain;
    return res;
}


bool SpecDecomp::calcDFT(const DataHolder& output, int z0, int nrsamples ) const
{
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	int cursample = z0 + idx;
	int samp = cursample + samplegate.start;
	for ( int ids=0; ids<sz; ids++ )
	{
	    float real = redata->series(realidx_)? 
			redata->series(realidx_)->value(samp-redata->z0_) : 0;
	    float imag = imdata->series(imagidx_)? 
			-imdata->series(imagidx_)->value(samp-imdata->z0_) : 0;

	    signal->set( ids, float_complex(real,imag) );
	    samp++;
	}

	removeBias( signal );
	window->apply( signal );

	const int diff = (int)(fftsz - sz)/2;
	for ( int idy=0; idy<sz; idy++ )
	    timedomain->set( diff+idy, signal->get(idy) );

	fft_.transform( *timedomain, *freqdomain );

	for ( int idf=0; idf<outputinterest.size(); idf++ )
	{
	    if ( !outputinterest[idf] ) continue;

	    float_complex val = freqdomain->get( idf );
	    float real = val.real();
	    float imag = val.imag();
	    output.series(idf)->setValue( idx, sqrt(real*real+imag*imag) );
	}
    }

    return true;
}


bool SpecDecomp::calcDWT(const DataHolder& output, int z0, int nrsamples ) const
{
    int len = nrsamples + scalelen;
    while ( !isPower( len, 2 ) ) len++;

    Array1DImpl<float> inputdata( len );
    if ( !redata->series(realidx_) ) return false;
    
    int off = (len-nrsamples)/2;
    for ( int idx=0; idx<len; idx++ )
    {
	int cursample = z0 - redata->z0_ + idx-off;
        inputdata.set( idx, redata->series(realidx_)->value(cursample) );
    }

    Array1DImpl<float> transformed( len );
    ::DWT dwt(dwtwavelet );// what does that really mean?

    dwt.setInputInfo( inputdata.info() );
    dwt.init();
    dwt.transform( inputdata, transformed );

    const int nrscales = isPower( len, 2 ) + 1;
    ArrPtrMan<float> spectrum =  new float[nrscales];
    spectrum[0] = fabs(transformed.get(0)); // scale 0 (dc)
    spectrum[1] = fabs(transformed.get(1)); // scale 1

    for ( int idx=0; idx<nrsamples; idx++ )
    {
        for ( int scale=2; scale<nrscales; scale++ )
        {
            int scalepos = intpow(2,scale-1) + ((idx+off) >> (nrscales-scale));
            spectrum[scale] = fabs(transformed.get(scalepos));

	    if ( !outputinterest[scale] ) continue;
	    output.series(scale)->setValue( idx, spectrum[scale] );
        }
    }

    return true;
}


#define mGetNextPow2(inp) \
    int nr = 2; \
    while ( nr < inp ) \
        nr *= 2; \
    inp = nr;


bool SpecDecomp::calcCWT(const DataHolder& output, int z0, int nrsamples ) const
{
    int nrsamp = nrsamples;
    if ( nrsamples == 1 ) nrsamp = 256;
    mGetNextPow2( nrsamp );
    if ( !redata->series(realidx_) || !imdata->series(imagidx_) ) return false;

    const int off = (nrsamp-nrsamples)/2;
    Array1DImpl<float_complex> inputdata( nrsamp );
    for ( int idx=0; idx<nrsamp; idx++ )
    {
	const int cursample = z0 + idx - off;
	const int reidx = cursample-redata->z0_;
	const float real = reidx < 0 || reidx >= redata->nrsamples_
		? 0 : redata->series(realidx_)->value( cursample-redata->z0_ );

	const int imidx = cursample-imdata->z0_;
	const float imag = imidx < 0 || imidx >= imdata->nrsamples_
		? 0 : -imdata->series(imagidx_)->value( cursample-imdata->z0_ );
        inputdata.set( idx, float_complex(real,imag) );
    }

    CWT& cwt = const_cast<CWT&>(cwt_);
    cwt.setInputInfo( Array1DInfoImpl(nrsamp) );
    cwt.setDir( true );
    cwt.setWavelet( cwtwavelet );
    cwt.setDeltaT( refstep );

    const float nyqfreq = 0.5 / SI().zStep();
    const int nrattribs = mNINT( nyqfreq / deltafreq );
    const float freqstop = deltafreq*nrattribs;
    cwt.setTransformRange( StepInterval<float>(deltafreq,freqstop,deltafreq) );
    cwt.init();

    Array2DImpl<float> outputdata(0,0);
    cwt.transform( inputdata, outputdata );

    const int nrscales = outputdata.info().getSize(1);
    if ( GetEnvVarYN("DTECT_PRINT_SPECDECOMP") )
    {
	for ( int idx=0; idx<nrsamp; idx++ )
	{
	    for ( int idy=0; idy<nrscales; idy++ )
		std::cerr << idx << '\t' << idy << '\t'
		    	  << outputdata.get(idx,idy)<<'\n';
	}
    }
    
    for ( int idx=0; idx<nrscales; idx++ )
    {
	if ( !outputinterest[idx] ) continue;
	for ( int ids=0; ids<nrsamples; ids++ )
	    output.series(idx)->setValue( ids, outputdata.get(ids+off,idx) );
    }

    return true;
}


const Interval<float>* SpecDecomp::reqZMargin( int inp, int ) const
{ return transformtype != mTransformTypeFourier ? 0 : &gate; }

}//namespace
