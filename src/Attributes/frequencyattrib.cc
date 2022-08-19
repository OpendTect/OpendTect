/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "frequencyattrib.h"
#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "filepath.h"
#include "fftfilter.h"
#include "genericnumer.h"
#include "oddirs.h"
#include "samplfunc.h"
#include "simpnumer.h"
#include "statrand.h"
#include "survinfo.h"
#include "windowfunction.h"
#include "odcomplex.h"
#include "od_ostream.h"

#include <math.h>

namespace Attrib
{
mAttrDefCreateInstance(Frequency)

void Frequency::initClass()
{
    mAttrStartInitClassWithDescAndDefaultsUpdate

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setDefaultValue( Interval<float>(-28, 28) );
    gate->setLimits( -mLargestZGate, mLargestZGate);
    desc->addParam( gate );

    desc->addParam( new BoolParam(normalizeStr(),false) );
    desc->addParam( new StringParam(windowStr(),"CosTaper") );

    FloatParam* paramval = new FloatParam( paramvalStr(), 0.95, false );
    paramval->setLimits( 0.0, 1.0 );
    desc->addParam( paramval );

    desc->addParam( new BoolParam(dumptofileStr(),false,false) );
    desc->addParam( new BoolParam(smoothspectrumStr(),false,false) );

    desc->addInput( InputSpec("Real data",true) );
    desc->addInput( InputSpec("Imag data",true) );
    desc->setNrOutputs( Seis::UnknowData, 8 );

    desc->setLocality( Desc::SingleTrace );
    mAttrEndInitClass
}


void Frequency::updateDesc( Desc& desc )
{
//Backward compatibility
    Attrib::ValParam* valpar = desc.getValParam( Frequency::paramvalStr() );
    Attrib::ValParam* winpar = desc.getValParam( Frequency::windowStr() );
    if ( !valpar || !winpar ) return;

    BufferString winstr = winpar->getStringValue();
    if ( winstr == "CosTaper5" )
    { winpar->setValue( "CosTaper" ); valpar->setValue( (float)0.95 ); }
    else if ( winstr == "CosTaper10" )
    { winpar->setValue( "CosTaper" ); valpar->setValue( (float)0.9 ); }
    else if ( winstr == "CosTaper20" )
    { winpar->setValue( "CosTaper" ); valpar->setValue( (float)0.8 ); }

    WindowFunction* winfunc = WINFUNCS().create( winstr );
    const bool hasvar = winfunc && winfunc->hasVariable();
    desc.setParamEnabled( paramvalStr(), hasvar );
    delete winfunc;
}


void Frequency::updateDefaults( Desc& desc )
{
    ValParam* paramgate = desc.getValParam(gateStr());
    mDynamicCastGet( ZGateParam*, zgate, paramgate )
    float roundedzstep = SI().zStep()*SI().zDomain().userFactor();
    if ( roundedzstep > 0 )
	roundedzstep = Math::Floor( roundedzstep );
    zgate->setDefaultValue( Interval<float>(-roundedzstep*7, roundedzstep*7) );
}


Frequency::Frequency( Desc& ds )
    : Provider(ds)
    , fft_(Fourier::CC::createDefault())
    , fftisinit_(false)
    , fftsz_(-1)
    , window_(0)
    , variable_(0)
    , signal_(0)
    , timedomain_(0)
    , freqdomain_(0)
{
    if ( !isOK() ) return;

    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1.f/zFactor() );

    mGetBool( normalize_, normalizeStr() );
    mGetString( windowtype_, windowStr() );
    mGetFloat( variable_, paramvalStr() );
    mGetBool( dumptofile_, dumptofileStr() );
    mGetBool( smoothspectrum_, smoothspectrumStr() );
    samplegate_ = Interval<int>(mNINT32(gate_.start/SI().zStep()),
			       mNINT32(gate_.stop/SI().zStep()));

    if ( !smoothspectrum_ )
    {
	if ( windowtype_ != "None" )
	    window_ = new ArrayNDWindow( Array1DInfoImpl(samplegate_.width()+1),
					 false, windowtype_, variable_ );

	fftsz_ = Fourier::FFTCC1D::getFastSize( (samplegate_.width()+1)*3 );
    }
}


Frequency::~Frequency()
{
    if ( dumptofile_ )
    {
	Stats::RandGen gen;
	for ( int idx=0; idx<dumpset_.size(); idx++ )
	{
	    const char* data = (dumpset_.get(idx)).buf();
	    if ( data && *data )
	    {
		FilePath fp( GetDataDir() );
		BufferString filename( "frequency." );
		filename += gen.getIndex(mUdf(int));
		filename = fp.add( filename ).fullPath();
		od_ostream strm( filename );
		if ( strm.isOK() )
		{
		    BufferString bfstr;
		    desc_.getDefStr(bfstr);
		    strm << bfstr << '\n' << data;
		}
	    }
	}
    }

    delete window_;
    delete signal_;
    delete timedomain_;
    delete freqdomain_;
    delete fft_;
}


void Frequency::prepPriorToBoundsCalc()
{
    if ( !mIsEqual( refstep_, SI().zStep(), 1e-6 ) )
    {
	samplegate_ = Interval<int>(mNINT32(gate_.start/refstep_),
				   mNINT32(gate_.stop/refstep_));

	if ( !smoothspectrum_ )
	{
	    fftsz_ = Fourier::FFTCC1D::getFastSize( (samplegate_.width()+1)*3 );
	    if ( window_ )
	    {
		delete window_;
		window_ = new ArrayNDWindow(
				Array1DInfoImpl(samplegate_.width()+1),
						false, windowtype_, variable_ );
	    }
	}
    }

    Provider::prepPriorToBoundsCalc();
}


const Interval<float>* Frequency::reqZMargin( int inp, int ) const
{ return &gate_; }


bool Frequency::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Frequency::getInputData( const BinID& relpos, int zintv )
{
    redata_ = inputs_[0]->getData( relpos, zintv );
    if ( !redata_ ) return false;

    imdata_ = inputs_[1]->getData( relpos, zintv );
    if ( !imdata_ ) return false;

    realidx_ = getDataIndex( 0 );
    imagidx_ = getDataIndex( 1 );

    return true;
}

#define mErrDelReturn()\
{\
    delete fftfilter;\
    return false;\
}

bool Frequency::computeData( const DataHolder& output, const BinID& relpos,
			     int z0, int nrsamples, int threadid ) const
{
    const int sgatesz = samplegate_.width()+1;
    Frequency* myself = const_cast<Frequency*>(this);
    FFTFilter* fftfilter = 0;
    if ( smoothspectrum_ )
    {
	fftfilter = new FFTFilter( sgatesz, refstep_);
	fftfilter->setLowPass( 80 );
	if ( windowtype_ != "None" )
	    if ( !fftfilter->setTimeTaperWindow( sgatesz, windowtype_,
						 variable_ ) )
		mErrDelReturn()
    }
    else
    {
	if ( !fftisinit_ )
	{
	    myself->fftsz_ = myself->fft_->getFastSize( sgatesz*3 );
	    myself->fft_->setInputInfo(Array1DInfoImpl(fftsz_));
	    myself->fft_->setDir(true);
	    myself->timedomain_ = new Array1DImpl<float_complex>( fftsz_ );
	    myself->freqdomain_ = new Array1DImpl<float_complex>( fftsz_ );
	}
    }

    if ( !fftisinit_ )
    {
	myself->signal_ = new Array1DImpl<float_complex>( sgatesz );
	myself->fftisinit_ = true;
    }

    if ( !myself->signal_ ) mErrDelReturn()

    if ( !smoothspectrum_ )
    {
	for ( int idx=0; idx<fftsz_; idx++ )
	    myself->timedomain_->set( idx, 0 );
    }

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = z0 + idx;
	int tempsamp = idx + samplegate_.start;
	for ( int idy=0; idy<sgatesz; idy ++ )
	{
	    float real = getInputValue( *redata_, realidx_, tempsamp, z0 );

	    if ( mIsUdf(real) )
		real = idy>0 ? myself->signal_->get(idy-1).real() : 0;

	    float imag = getInputValue( *imdata_, imagidx_, tempsamp, z0 );
	    if ( mIsUdf(imag) )
		imag = idy>0 ? myself->signal_->get(idy-1).imag() : 0;

	    myself->signal_->set( idy, float_complex(real,imag) );
	    tempsamp++;
	}

	Array1DImpl<float_complex>* freqdomain = 0;
	if ( smoothspectrum_ )
	{
	    fftfilter->requestStayInFreqDomain();
	    if ( !fftfilter->apply( *(myself->signal_), true ) )
		mErrDelReturn()

	    freqdomain = fftfilter->getFreqDomainArr();
	}
	else
	{
	    if ( window_ ) window_->apply( myself->signal_ );
	    removeBias<float_complex,float>( *myself->signal_ );
	    for ( int idy=0; idy<sgatesz; idy++ )
		myself->timedomain_->set( sgatesz+idy, signal_->get(idy) );

	    fft_->setInput( timedomain_->getData() );
	    fft_->setOutput( myself->freqdomain_->getData() );
	    if ( !fft_->run( true ) )
		return false;

	    freqdomain = myself->freqdomain_;
	}

	if ( !freqdomain ) mErrDelReturn()

	const int fftsz = freqdomain->info().getSize(0);
	const float df = 1.f / (refstep_ * fftsz);

	TypeSet<float> freqdomainpower( fftsz, 0 );
	int maxnr = -1;
	float maxval = 0;
	float sum = 0;

	for ( int idy=0; idy<=fftsz/2; idy++ )
	{
	    const float val = abs( freqdomain->get(idy) );
	    const float val2 = val * val;
	    freqdomainpower[idy] = val2;

	    sum += val2;

	    if ( dumptofile_ )
	    {
		BufferString dump;
		BinID pos = currentbid_;
		dump += pos.inl(); dump += " "; dump += pos.crl(); dump += " ";
		dump += cursample*refstep_; dump += " ";
		dump += df*idy; dump += " "; dump += val2; dump += "\n";
		myself->dumpset_.add( dump );
	    }

	    if ( val2<maxval ) continue;

	    maxval = val2;
	    maxnr = idy;
	}

	ArrayValueSeries<float,float> arr( freqdomainpower.arr(), false );
	FreqFunc func( arr, fftsz );
	float exactpos = findExtreme( func, false, mCast(float,maxnr-1),
						      mCast(float,maxnr+1) );
	if ( !mIsUdf(exactpos) )
	    maxval = func.getValue( exactpos );
	else
	    exactpos = mCast( float, maxnr );

	if ( mIsZero(sum,mDefEps) || exactpos<0.1 )
	{
	    setOutputValue( output, 0, idx, z0, 0 );
	    setOutputValue( output, 1, idx, z0, 0 );
	    setOutputValue( output, 2, idx, z0, 0 );
	    setOutputValue( output, 3, idx, z0, 0 );
	    setOutputValue( output, 4, idx, z0, 0 );
	    setOutputValue( output, 5, idx, z0, 0 );
	    setOutputValue( output, 6, idx, z0, 0 );
	    setOutputValue( output, 7, idx, z0, 0 );
	    continue;
	}

	if ( normalize_ )
	{
	    for ( int idy=0; idy<=fftsz/2; idy++ )
		freqdomainpower[idy] /= sum;

	    maxval /= sum;
	    sum = 1;
	}

	const float halfsum = sum/2;
	float sa = 0;
	float mfsum = 0;
	bool mfisset = false;
	float aqf = 0;

	float wf = 0;
	float wf2 = 0;


	for ( int idy=0; idy<=fftsz/2; idy++ )
	{
	    float height = freqdomainpower[idy];
	    float freq = idy * df;
	    float hf = height*freq;

	    if ( idy>maxnr )
	    {
		sa += height;
		aqf += hf;
	    }

	    if ( !mfisset && (mfsum+=height)>halfsum )
	    {
		mfisset = true;
		setOutputValue( output, 2, idx, z0, idy*df );
	    }

	    wf += hf;
	    wf2 += hf*freq;
	}

	setOutputValue( output, 0, idx, z0, exactpos*df );
	setOutputValue( output, 1, idx, z0, wf/sum );
	setOutputValue( output, 3, idx, z0, wf2/sum );
	setOutputValue( output, 4, idx, z0, maxval );
	setOutputValue( output, 5, idx, z0, sa );
	setOutputValue( output, 6, idx, z0, 1+(maxval-sum)/(maxval+sum) );
	setOutputValue( output, 7, idx, z0, aqf );
    }

    return true;
}


bool Frequency::checkInpAndParsAtStart()
{
    if ( windowtype_ == "None" || smoothspectrum_ )
	return Provider::checkInpAndParsAtStart();
    else
	return window_ && window_->isOK() && Provider::checkInpAndParsAtStart();
}

} // namespace Attrib
