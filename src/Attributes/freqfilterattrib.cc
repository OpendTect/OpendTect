/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          February 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "freqfilterattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "fftfilter.h"
#include "fourier.h"
#include "genericnumer.h"
#include "timeser.h"
#include "transform.h"
#include "ptrman.h"
#include "arrayndinfo.h"
#include "survinfo.h"

#include <math.h>


#define mMINNRSAMPLES 		100


namespace Attrib
{

mAttrDefCreateInstance(FreqFilter) 
   
void FreqFilter::initClass()
{
    mAttrStartInitClassWithUpdate

    //Note: Ordering must be the same as numbering!
    EnumParam* filtertype_ = new EnumParam( filtertypeStr() );
    static const char** filtertypes = FFTFilter::TypeNames();
    for ( int idx=0; filtertypes[idx]; idx++ )
	filtertype_->addEnum( filtertypes[idx] );
    filtertype_->setDefaultValue( FFTFilter::LowPass );
    desc->addParam( filtertype_ );

    FloatParam* minfreq_ = new FloatParam( minfreqStr() );
    minfreq_->setLimits( Interval<float>(0,mUdf(float)) );
    minfreq_->setDefaultValue(15);
    desc->addParam( minfreq_ );

    FloatParam* maxfreq_ = new FloatParam( maxfreqStr() );
    maxfreq_->setLimits( Interval<float>(0,mUdf(float)) );
    maxfreq_->setDefaultValue(50);
    desc->addParam( maxfreq_ );

    IntParam* nrpoles_ = new IntParam( nrpolesStr() );
    nrpoles_->setLimits( Interval<int>(2,20) );
    nrpoles_->setDefaultValue( 4 );
    desc->addParam( nrpoles_ );

    BoolParam* isfftfilter_ = new BoolParam( isfftfilterStr() );
    isfftfilter_->setDefaultValue( true );
    desc->addParam( isfftfilter_ );

    StringParam* window_ = new StringParam( windowStr() );
    window_->setDefaultValue( "CosTaper" );
    desc->addParam( window_ );

    FloatParam* variable_ = new FloatParam( paramvalStr() );
    const float defval = 0.95f;
    variable_->setDefaultValue( defval );
    variable_->setRequired( false );
    desc->addParam(variable_);
    
    BoolParam* isfreqtaper = new BoolParam( isfreqtaperStr() );
    isfreqtaper->setDefaultValue( true );
    desc->addParam( isfreqtaper );

    StringParam* fwindow_ = new StringParam( fwindowStr() );
    fwindow_->setDefaultValue( "CosTaper" );
    fwindow_->setRequired( false );
    desc->addParam( fwindow_ );
    
    FloatParam* highfreqvariable_ = new FloatParam( highfreqparamvalStr() );
    highfreqvariable_->setLimits( Interval<float>(0,mUdf(float)) );
    highfreqvariable_->setDefaultValue( 10 );
    highfreqvariable_->setRequired( false );
    desc->addParam( highfreqvariable_ );
    
    FloatParam* lowfreqvariable_ = new FloatParam( lowfreqparamvalStr() );
    lowfreqvariable_->setLimits( Interval<float>(0,mUdf(float)) );
    lowfreqvariable_->setDefaultValue( 55 );
    lowfreqvariable_->setRequired( false );
    desc->addParam( lowfreqvariable_ );

    desc->addOutputDataType( Seis::UnknowData );

    desc->addInput( InputSpec("Real data",true) );
    desc->addInput( InputSpec("Imag data",true) );

    desc->setLocality( Desc::SingleTrace );
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
		      type != FFTFilter::getTypeString( FFTFilter::LowPass ) );
    desc.setParamEnabled( maxfreqStr(),
		      type != FFTFilter::getTypeString( FFTFilter::HighPass ) );

    const bool isfft = desc.getValParam(isfftfilterStr())->getBoolValue();
    desc.setParamEnabled( nrpolesStr(), !isfft );
    desc.inputSpec(1).enabled_ = isfft;
}



FreqFilter::FreqFilter( Desc& ds )
    : Provider( ds )
    , fftsz_(-1)
    , minfreq_(mUdf(float))
    , maxfreq_(mUdf(float))
    , nrpoles_(mUdf(int))
    , signal_(0)
    , timedomain_(0)
    , timecplxoutp_(0)
    , window_(0)
    , windowtype_(0)
    , variable_(mUdf(float))
    , highfreqvariable_(mUdf(float))
    , lowfreqvariable_(mUdf(float))
{
    if ( !isOK() ) return;

    mGetEnum( filtertype_, filtertypeStr() );
    if ( filtertype_ != FFTFilter::LowPass )
	mGetFloat( minfreq_, minfreqStr() );

    if ( filtertype_ != FFTFilter::HighPass )
	mGetFloat( maxfreq_, maxfreqStr() );

    mGetBool( isfftfilter_, isfftfilterStr() );
    if ( isfftfilter_ )
    {
	if ( filtertype_ != FFTFilter::LowPass )
	    mGetFloat( highfreqvariable_, highfreqparamvalStr() );

	if ( filtertype_ != FFTFilter::HighPass )
	    mGetFloat( lowfreqvariable_, lowfreqparamvalStr() );

	mGetString( windowtype_, windowStr() );
	mGetFloat( variable_, paramvalStr() );
    }
    else
	mGetInt( nrpoles_, nrpolesStr() );

    zmargin_ = Interval<int>( -mNINT32((float) mMINNRSAMPLES/2),
	    		       mNINT32((float) mMINNRSAMPLES/2) );
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
    redata_ = inputs_[0]->getData(relpos, idx);
    if ( !redata_ ) return false;

    realidx_ = getDataIndex( 0 );
    
    imdata_ = isfftfilter_ ? inputs_[1]->getData(relpos, idx) : 0;
    imagidx_ = isfftfilter_ ? getDataIndex( 1 ) : -1;
    if ( isfftfilter_ && !imdata_ ) return false;

    return true;
}


bool FreqFilter::computeData( const DataHolder& output, const BinID& relpos,
				int z0, int nrsamples, int threadid ) const
{
    if ( isfftfilter_ )
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
	csamp = z0 - mNINT32((float) nrsamp/2) + mNINT32((float) nrsamples/2);
    }

    ArrPtrMan<float> data = new float [nrsamp];
    ArrPtrMan<float> outp = new float [nrsamp];

    for ( int idx=0; idx<nrsamp; idx++ )
    {
	int reidx = idx + csamp - z0;
	int checkidx = idx + csamp - redata_->z0_;
	int maxidx = redata_->nrsamples_-1;
	data[idx] = checkidx<0 ? getInputValue( *redata_, realidx_, 0, z0 )
	    : checkidx>maxidx ? getInputValue( *redata_, realidx_, maxidx, z0 )
			      : getInputValue( *redata_, realidx_, reidx, z0 );
    }

    if ( filtertype_ == FFTFilter::LowPass )
    {
	float cutoff = refstep_ * maxfreq_;
	BFlowpass( nrpoles_, cutoff, nrsamp, data, outp );
	reverseArray( outp.ptr(), nrsamp );
	BFlowpass( nrpoles_, cutoff, nrsamp, outp, outp );
	reverseArray( outp.ptr(), nrsamp );
    }
    else if ( filtertype_ == FFTFilter::HighPass )
    {
	float cutoff = refstep_ * minfreq_;
	BFhighpass( nrpoles_, cutoff, nrsamp, data, outp );
	reverseArray( outp.ptr(), nrsamp );
	BFhighpass( nrpoles_, cutoff, nrsamp, outp, outp );
	reverseArray( outp.ptr(), nrsamp );
    }
    else
    {
	float cutoff = refstep_ * maxfreq_;
	ArrPtrMan<float> tmp = new float [nrsamp];
	BFlowpass( nrpoles_, cutoff, nrsamp, data, tmp );
	cutoff = refstep_ * minfreq_;
	BFhighpass( nrpoles_, cutoff, nrsamp, tmp, outp );
	reverseArray( outp.ptr(), nrsamp );
	cutoff = refstep_ * maxfreq_;
	BFlowpass( nrpoles_, cutoff, nrsamp, outp, outp );
	cutoff = refstep_ * minfreq_;
	BFhighpass( nrpoles_, cutoff, nrsamp, outp, outp );
	reverseArray( outp.ptr(), nrsamp );
    }
    
    if ( nrsamples < mMINNRSAMPLES )
    {
	int offset = mNINT32((float) nrsamp/2) - mNINT32((float) nrsamples/2);
	for ( int idx=0; idx<nrsamples; idx++ )
	    setOutputValue( output, 0, idx, z0, outp[offset - 1 + idx] );
    }
    else
    {
	float* out = output.series(0)->arr();
	memcpy(out,outp,nrsamp*sizeof(float));
    }
}


void FreqFilter::fftFilter( const DataHolder& output, int z0, int nrsamples )
{
    signal_.setInfo( Array1DInfoImpl( nrsamples ) );
    bool isimagudf = false;
    Array1DImpl<float> realsignal( nrsamples );
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float real = getInputValue( *redata_, realidx_, idx, z0 );
	const float imag = getInputValue( *imdata_, imagidx_, idx, z0 );
	signal_.set( idx, float_complex(real,-imag) );
	if ( imag > 1e20 )
	    isimagudf = true;

	realsignal.set( idx, real );
    }

    FFTFilter filter( nrsamples, refstep_);
    if ( strcmp(windowtype_,"None") )
	if ( !filter.setTimeTaperWindow(nrsamples,windowtype_,variable_) )
	    return;

    if ( filtertype_ == FFTFilter::HighPass )
	filter.setHighPass( highfreqvariable_, minfreq_ );
    else if ( filtertype_ == FFTFilter::LowPass )
	filter.setLowPass( maxfreq_, lowfreqvariable_ );
    else if ( filtertype_ == FFTFilter::BandPass )
	filter.setBandPass( highfreqvariable_, minfreq_,
	       		    maxfreq_, lowfreqvariable_ );

    if ( isimagudf )
    {
	if ( !filter.apply(realsignal) )
	    return;
    }
    else
	if ( !filter.apply(signal_) )
	    return;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	if ( isimagudf )
	    setOutputValue( output, 0, idx, z0, realsignal.get( idx ) );
	else
	    setOutputValue( output, 0, idx, z0, signal_.get( idx ).real() );
    }
}


void FreqFilter::setSz( int sz )
{
    signal_.setInfo( Array1DInfoImpl( sz ) );
    timedomain_.setInfo( Array1DInfoImpl( fftsz_ ) );
    timecplxoutp_.setInfo( Array1DInfoImpl( fftsz_ ) );
}


const Interval<int>* FreqFilter::desZSampMargin(int input, int output) const
{
    return &zmargin_;
}


};//namespace
