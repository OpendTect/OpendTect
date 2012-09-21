/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2004
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "specdecompattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "genericnumer.h"
#include "survinfo.h"
#include "strmprov.h"
#include "transform.h"
#include "envvars.h"
#include "math2.h"

#include <math.h>
#include <complex>

#include <iostream>

#define mTransformTypeFourier		0
#define mTransformTypeDiscrete  	1
#define mTransformTypeContinuous	2

namespace Attrib
{

mAttrDefCreateInstance(SpecDecomp)
    
void SpecDecomp::initClass()
{
    mAttrStartInitClassWithDescAndDefaultsUpdate

    EnumParam* ttype = new EnumParam( transformTypeStr() );
    //Note: Ordering must be the same as numbering!
    ttype->addEnum( transTypeNamesStr(mTransformTypeFourier) );
    ttype->addEnum( transTypeNamesStr(mTransformTypeDiscrete) );
    ttype->addEnum( transTypeNamesStr(mTransformTypeContinuous) );
    ttype->setDefaultValue( mTransformTypeFourier );
    desc->addParam( ttype );

    EnumParam* window = new EnumParam( windowStr() );
    window->addEnums( ArrayNDWindow::WindowTypeNames() );
    window->setRequired( false );
    window->setDefaultValue( ArrayNDWindow::CosTaper5 );
    window->setValue( 0 );
    desc->addParam( window );

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-mLargestZGate,mLargestZGate) );
    gate->setDefaultValue( Interval<float>(-28, 28) );
    desc->addParam( gate );

    FloatParam* deltafreq = new FloatParam( deltafreqStr() );
    deltafreq->setRequired( false );
    deltafreq->setDefaultValue( 5 );
    desc->addParam( deltafreq );

    EnumParam* dwtwavelet = new EnumParam( dwtwaveletStr() );
    dwtwavelet->addEnums( WaveletTransform::WaveletTypeNames() );
    dwtwavelet->setDefaultValue( WaveletTransform::Haar );
    desc->addParam( dwtwavelet );

    EnumParam* cwtwavelet = new EnumParam( cwtwaveletStr() );
    cwtwavelet->addEnums( CWT::WaveletTypeNames() );
    cwtwavelet->setDefaultValue( CWT::Morlet );
    desc->addParam( cwtwavelet );

    desc->addInput( InputSpec("Real data",true) );
    desc->addInput( InputSpec("Imag data",true) );
    desc->addOutputDataType( Seis::UnknowData );

    desc->setLocality( Desc::SingleTrace );
    mAttrEndInitClass
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

    //HERE see what to do when SI().zStep() != refstep_ !!!
    float dfreq;
    mGetFloatFromDesc( desc, dfreq, deltafreqStr() );
    const float nyqfreq = 0.5f / SI().zStep();
    const int nrattribs = (int)( nyqfreq / dfreq );
    desc.setNrOutputs( Seis::UnknowData, nrattribs );
}


void SpecDecomp::updateDefaults( Desc& desc )
{
    ValParam* paramgate = desc.getValParam(gateStr());
    mDynamicCastGet( ZGateParam*, zgate, paramgate )
    float roundedzstep = SI().zStep()*SI().zDomain().userFactor();
    if ( roundedzstep > 0 )
	roundedzstep = (int)( roundedzstep );
    zgate->setDefaultValue( Interval<float>(-roundedzstep*7, roundedzstep*7) );
}


const char* SpecDecomp::transTypeNamesStr(int type)
{
    if ( type==mTransformTypeFourier ) return "DFT";
    if ( type==mTransformTypeDiscrete ) return "DWT";
    return "CWT";
}


SpecDecomp::SpecDecomp( Desc& desc )
    : Provider( desc )
    , window_(0)
    , fftisinit_(false)
    , scalelen_(0)
    , fft_(Fourier::CC::createDefault())
{ 
    if ( !isOK() ) return;

    mGetEnum( transformtype_, transformTypeStr() );
    mGetFloat( deltafreq_, deltafreqStr() );

    if ( transformtype_ == mTransformTypeFourier )
    {
	int wtype;
	mGetEnum( wtype, windowStr() );
	windowtype_ = (ArrayNDWindow::WindowType)wtype;
	
	mGetFloatInterval( gate_, gateStr() );
	gate_.start = gate_.start / zFactor();
	gate_.stop = gate_.stop / zFactor();
    }
    else if ( transformtype_ == mTransformTypeDiscrete )
    {
	int dwave;
	mGetEnum( dwave, dwtwaveletStr() );
	dwtwavelet_ = (WaveletTransform::WaveletType) dwave;
    }
    else 
    {
	int cwave;
	mGetEnum( cwave, cwtwaveletStr() );
	cwtwavelet_ = (CWT::WaveletType) cwave;
    }
    
    desgate_ = Interval<int>( -(1024-1), 1024-1 );
}


SpecDecomp::~SpecDecomp()
{
    delete fft_;
    delete window_;
}


bool SpecDecomp::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool SpecDecomp::getInputData( const BinID& relpos, int idx )
{
    redata_ = inputs_[0]->getData( relpos, idx );
    if ( !redata_ ) return false;

    imdata_ = inputs_[1]->getData( relpos, idx );
    if ( !imdata_ ) return false;

    realidx_ = getDataIndex( 0 );
    imagidx_ = getDataIndex( 1 );
    
    return true;
}


bool SpecDecomp::computeData( const DataHolder& output, const BinID& relpos,
			      int z0, int nrsamples, int threadid ) const 
{
    if ( !fftisinit_ )
    {
	if ( transformtype_ == mTransformTypeFourier )
	{
	    const_cast<SpecDecomp*>(this)->samplegate_ = 
		     Interval<int>(mNINT32(gate_.start/refstep_),
				   mNINT32(gate_.stop/refstep_));
	    const_cast<SpecDecomp*>(this)->sz_ = samplegate_.width()+1;

	    const float fnyq = 0.5f / refstep_;
	    const int minsz = mNINT32( 2*fnyq/deltafreq_ );
	    const_cast<SpecDecomp*>(this)->fftsz_ = sz_ > minsz ? sz_ : minsz;
	    const_cast<SpecDecomp*>(this)->
			fft_->setInputInfo(Array1DInfoImpl(fftsz_));
	    const_cast<SpecDecomp*>(this)->fft_->setDir(true);
	    const_cast<SpecDecomp*>(this)->df_ = 
		Fourier::CC::getDf(refstep_,fftsz_);

	    const_cast<SpecDecomp*>(this)->window_ = 
		new ArrayNDWindow( Array1DInfoImpl(sz_), false, 
			(ArrayNDWindow::WindowType)windowtype_ );
	}
	else
	    const_cast<SpecDecomp*>(this)->scalelen_ = 1024;

	const_cast<SpecDecomp*>(this)->fftisinit_ = true;
    }
    
    if ( transformtype_ == mTransformTypeFourier )
	return calcDFT(output, z0, nrsamples);
    else if ( transformtype_ == mTransformTypeDiscrete )
	return calcDWT(output, z0, nrsamples);
    else if ( transformtype_ == mTransformTypeContinuous )
	return calcCWT(output, z0, nrsamples);

    return true;
}


bool SpecDecomp::calcDFT(const DataHolder& output, int z0, int nrsamples ) const
{
    Array1DImpl<float_complex> signal( sz_ );
    Array1DImpl<float_complex> timedomain( fftsz_ );
    Array1DImpl<float_complex> freqdomain( fftsz_ );
    memset( timedomain.getData(), 0, fftsz_*sizeof(float_complex) );

    const int outputsz = outputinterest_.size();
    const int maxidx = fftsz_ - 1;
    
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	int samp = idx + samplegate_.start;
	for ( int ids=0; ids<sz_; ids++ )
	{
	    float real = redata_->series(realidx_) ?
			    getInputValue( *redata_, realidx_, samp, z0 ) : 0;	
	    float imag = imdata_->series(imagidx_) ? 
			    -getInputValue( *imdata_, imagidx_, samp, z0 ) : 0;	

	    if ( mIsUdf(real) ) real = 0;
	    if ( mIsUdf(imag) ) imag = 0;
	    signal.set( ids, float_complex(real,imag) );
	    samp++;
	}

	removeBias( &signal );
	window_->apply( &signal );

	const int diff = (int)(fftsz_ - sz_)/2;
	for ( int idy=0; idy<sz_; idy++ )
	    timedomain.set( diff+idy, signal.get(idy) );

	fft_->setInput( timedomain.getData() );
	fft_->setOutput( freqdomain.getData() );
	if ( !fft_->run( true ) )
	    return false;

	for ( int idf=0; idf<outputsz; idf++ )
	{
	    if ( !outputinterest_[idf] ) continue;

	    float val = mUdf(float);
	    if ( idf<maxidx )
	    {
    		const float_complex cval = freqdomain.get( idf+1 );
    		const float real = cval.real();
    		const float imag = cval.imag();
		val = Math::Sqrt(real*real+imag*imag);
	    }

	    setOutputValue( output, idf, idx, z0, val );
	}
    }

    return true;
}


bool SpecDecomp::calcDWT(const DataHolder& output, int z0, int nrsamples ) const
{
    int len = nrsamples + scalelen_;
    while ( !isPower( len, 2 ) ) len++;

    Array1DImpl<float> inputdata( len );
    if ( !redata_->series(realidx_) ) return false;
    
    int off = (len-nrsamples)/2;
    for ( int idx=0; idx<len; idx++ )
        inputdata.set( idx, getInputValue( *redata_, realidx_, idx-off, z0 ) );

    Array1DImpl<float> transformed( len );
    ::DWT dwt(dwtwavelet_ );

    dwt.setInputInfo( inputdata.info() );
    dwt.setInput( inputdata.getData() );
    dwt.setOutput( transformed.getData() );
    if ( !dwt.run(false) )
	return false;
    
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

	    if ( !outputinterest_[scale] ) continue;
	    setOutputValue( output, scale, idx, z0, spectrum[scale] );
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
    if ( nrsamples < 256 ) nrsamp = 256;
    mGetNextPow2( nrsamp );
    if ( !redata_->series(realidx_) || !imdata_->series(imagidx_) )
	return false;

    const int off = (nrsamp-nrsamples)/2;
    Array1DImpl<float_complex> inputdata( nrsamp );
    for ( int idx=0; idx<nrsamp; idx++ )
    {
	const int cursample = z0 + idx - off;   
	const int reidx = cursample-redata_->z0_;
	float real = reidx < 0 || reidx >= redata_->nrsamples_
		    ? 0 : getInputValue( *redata_, realidx_, idx-off, z0 );
	if ( mIsUdf(real) ) real = 0;

	const int imidx = cursample-imdata_->z0_;
	float imag = imidx < 0 || imidx >= imdata_->nrsamples_
		    ? 0 : -getInputValue( *imdata_, imagidx_, idx-off, z0 );
	if ( mIsUdf(imag) ) imag = 0;
        inputdata.set( idx, float_complex(real, imag) );
    }
    
    CWT& cwt = const_cast<CWT&>(cwt_);
    cwt.setInputInfo( Array1DInfoImpl(nrsamp) );
    cwt.setDir( true );
    cwt.setWavelet( cwtwavelet_ );
    cwt.setDeltaT( refstep_ );

    const float nyqfreq = 0.5f / SI().zStep();
    const int nrattribs = (int)( nyqfreq / deltafreq_ );
    const float freqstop = deltafreq_*nrattribs;
    TypeSet<int> freqidxs;
    for ( int idx=0; idx<nrOutputs(); idx++ )
	if ( outputinterest_[idx]>0 ) freqidxs += idx;

    cwt.setFreqIdxs( freqidxs );
    cwt.setTransformRange( StepInterval<float>(deltafreq_,freqstop,deltafreq_));
    cwt.init();
    Array2DImpl<float> outputdata(0,0);
    cwt.transform( inputdata, outputdata );
    const int nrscales = outputdata.info().getSize(1);

    const char* fname = GetEnvVar( "OD_PRINT_SPECDECOMP_FILE" );
    if ( fname && *fname )
    {
	StreamData sd = StreamProvider(fname).makeOStream();
	if ( sd.usable() )
	{
	    for ( int idx=0; idx<nrsamp; idx++ )
	    {
		for ( int idy=0; idy<nrscales; idy++ )
		    *sd.ostrm << idx << '\t' << idy << '\t'
			      << outputdata.get(idx,idy)<<'\n';
	    }
	    sd.close();
	}
    }

    for ( int idx=0; idx<nrscales; idx++ )
    {
	if ( !outputinterest_[idx] ) continue;
	for ( int ids=0; ids<nrsamples; ids++ )
	    setOutputValue( output, idx, ids, z0, outputdata.get(ids+off,idx) );
    }

    return true;
}


const Interval<float>* SpecDecomp::reqZMargin( int inp, int ) const
{ return transformtype_ != mTransformTypeFourier ? 0 : &gate_; }


const Interval<int>* SpecDecomp::desZSampMargin( int inp, int ) const
{
    return transformtype_ == mTransformTypeFourier ? 0 : &desgate_;
}


void SpecDecomp::getCompNames( BufferStringSet& nms ) const
{
    nms.erase();
    const float fnyq = 0.5f / refstep_;
    const char* basestr = "frequency = ";
    BufferString suffixstr = zIsTime() ? " Hz" : " cycles/mm";
    for ( float freq=deltafreq_; freq<fnyq; freq+=deltafreq_ )
    {
	BufferString tmpstr = basestr; tmpstr += freq; tmpstr += suffixstr;
	nms.add( tmpstr.buf() );
    }
}


bool SpecDecomp::prepPriorToOutputSetup()
{
    return areAllOutputsEnabled();
}


bool SpecDecomp::areAllOutputsEnabled() const
{
    for ( int idx=0; idx<nrOutputs(); idx++ )
	if ( !outputinterest_[idx] )
	    return false;

    return true;
}


}//namespace
