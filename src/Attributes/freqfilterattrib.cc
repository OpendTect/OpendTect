/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          February 2003
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: freqfilterattrib.cc,v 1.66 2012-07-24 08:51:39 cvsnageswara Exp $";


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
    isfftfilter_->setDefaultValue( false );
    desc->addParam( isfftfilter_ );

    StringParam* window_ = new StringParam( windowStr() );
    window_->setDefaultValue( "CosTaper" );
    desc->addParam( window_ );

    StringParam* fwindow_ = new StringParam( fwindowStr() );
    fwindow_->setDefaultValue( "CosTaper" );
    fwindow_->setRequired( false );
    desc->addParam( fwindow_ );

    FloatParam* variable_ = new FloatParam( paramvalStr() );
    const float defval = 0.95;
    variable_->setDefaultValue( defval );
    variable_->setRequired( false );
    desc->addParam(variable_);
    
    BoolParam* isfreqtaper = new BoolParam( isfreqtaperStr() );
    isfreqtaper->setDefaultValue( false );
    desc->addParam( isfreqtaper );
    
    FloatParam* highfreqval = new FloatParam( highfreqparamvalStr() );
    const float hdefval = 10;
    highfreqval->setDefaultValue( hdefval );
    highfreqval->setRequired( false );
    desc->addParam(highfreqval);
    
    FloatParam* lowfreqval = new FloatParam( lowfreqparamvalStr() );
    const float lfreqval = 55;
    lowfreqval->setDefaultValue( lfreqval );
    lowfreqval->setRequired( false );
    desc->addParam(lowfreqval);

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
    , minfreq_(0)		   
    , maxfreq_(0)		   
    , signal_(0)
    , timedomain_(0)
    , timecplxoutp_(0)
    , window_(0)
    , windowtype_(0)
    , variable_(0)
    , highfreqvariable_(0)
    , lowfreqvariable_(0)
{
    if ( !isOK() ) return;

    mGetEnum( filtertype_, filtertypeStr() );
    
    if ( filtertype_ == FFTFilter::LowPass )
    {
	mGetFloat( maxfreq_, maxfreqStr() );
    }
    else if ( filtertype_ == FFTFilter::HighPass )
    {
	mGetFloat( minfreq_, minfreqStr() );
    }
    else
    {
	mGetFloat( maxfreq_, maxfreqStr() );
	mGetFloat( minfreq_, minfreqStr() );
    }

    mGetInt( nrpoles_, nrpolesStr() );
    mGetBool( isfftfilter_, isfftfilterStr() );

    mGetString( windowtype_, windowStr() );
    mGetFloat( variable_, paramvalStr() );
    mGetFloat( highfreqvariable_, highfreqparamvalStr() );
    mGetFloat( lowfreqvariable_, lowfreqparamvalStr() );

    if ( isfftfilter_ && strcmp(windowtype_,"None") )
    {
	delete window_;
	window_ = new ArrayNDWindow( Array1DInfoImpl(100),
				    false, windowtype_, variable_ );
    }
	
    zmargin_ = Interval<int>( -mNINT32(mMINNRSAMPLES/2), mNINT32(mMINNRSAMPLES/2) );
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
	csamp = z0 - mNINT32(nrsamp/2) + mNINT32(nrsamples/2);
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
	int offset = mNINT32(nrsamp/2) - mNINT32(nrsamples/2);
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
    FFTFilter filter;
    fftsz_ = filter.getFFTFastSize(nrsamp*3);
    if ( nrsamples < mMINNRSAMPLES )
    {
	nrsamp = mMINNRSAMPLES;
	z0safe = z0 - mNINT32(nrsamp/2) + mNINT32(nrsamples/2);
    }
    
    if ( nrsamp>signal_.info().getSize(0) )
    {
	setSz(nrsamp);

	if ( strcmp(windowtype_,"None") )
	{
	    delete window_;
	    window_ = new ArrayNDWindow( Array1DInfoImpl(nrsamp),
					 false, windowtype_, variable_ );
	}
    }

    for ( int idx=0; idx<fftsz_; idx++ )
        timedomain_.set( idx, 0 );

    const float df = Fourier::CC::getDf( refstep_, fftsz_);
    const int sz = fftsz_/2 - nrsamp/2;
    for ( int idx=0; idx<nrsamp; idx++ )
    {
	int csamp = idx + z0safe;
	int cidx = csamp - z0;
	int remaxidx = redata_->nrsamples_-1;
	int checkridx = csamp - redata_->z0_;
	float real = checkridx<0 
			 ? getInputValue( *redata_, realidx_, 0, z0 )
			 : checkridx>remaxidx 
			    ? getInputValue( *redata_, realidx_, remaxidx, z0 )
			    : getInputValue( *redata_, realidx_, cidx, z0 );
	int immaxidx = imdata_->nrsamples_-1;
	int checkiidx = csamp - imdata_->z0_;
	int useidx = checkiidx<0 ? 0 : (checkiidx>immaxidx ? immaxidx : cidx);
	float imag = getInputValue( *imdata_, imagidx_, useidx, z0 );
	signal_.set( idx, float_complex(real,-imag) );
    }

    if ( window_ ) window_->apply( &signal_ );
    float avg = 0; TypeSet<int> undefvalidxs;
    for ( int idx=0; idx<nrsamp; idx++ )
    {
	float_complex val = signal_.get( idx );
	if ( mIsUdf( val ) )
	    undefvalidxs += idx;
	else
	    avg += val.real(); 
    }
    if ( undefvalidxs.size() != nrsamp )
	avg /= ( nrsamp-undefvalidxs.size() );
    for ( int idx=0; idx<undefvalidxs.size(); idx++ )
	signal_.set( undefvalidxs[idx], avg );
    removeBias( &signal_ );
    for ( int idy=0; idy<nrsamp; idy++ )
	timedomain_.set( sz+idy, signal_.get(idy) );

    const int datasz = (int)( Fourier::CC::getNyqvist( SI().zStep()) ); 
    int winsz1 = 2*( (int)minfreq_ );
    if ( mIsZero( minfreq_ - highfreqvariable_, 0.5 ) )
	winsz1 = 0;
    int winsz2 = 2*( datasz-(int)maxfreq_ );
    if ( datasz<=0 || datasz<=(int)maxfreq_ || 
	    mIsZero( maxfreq_ - lowfreqvariable_, 0.5 ) )
	winsz2 = 0;

#define mSetFilterFreqWin( winsz, islow, win )\
    if ( winsz > 0 )\
    {\
	float var = islow ? 1-(lowfreqvariable_-maxfreq_) / (datasz - maxfreq_)\
			  : highfreqvariable_ / minfreq_;\
	if ( var >=0 && var <= 1 )\
	{\
	    ArrayNDWindow window(Array1DInfoImpl(winsz), false,"CosTaper",var);\
	    float* winvals = window.getValues();\
	    for ( int idx=0; idx<winsz/2 && winvals; idx++ )\
		win.set(idx, islow ? 1-winvals[idx] : 1-winvals[winsz/2+idx]);\
	    filter.setFreqBorderWindow( win.getData(), winsz/2, islow );\
	}\
    }

    Array1DImpl<float> lwin( winsz2/2 ), hwin( winsz1/2 );
    mSetFilterFreqWin( winsz2, true, lwin )
    mSetFilterFreqWin( winsz1, false, hwin )
    filter.set( df, minfreq_, maxfreq_, FFTFilter::Type( filtertype_ ), false );
    filter.apply( timedomain_.getData(), timecplxoutp_.getData(), fftsz_ ); 

    const int firstidx = nrsamples < mMINNRSAMPLES ? fftsz_/2 - nrsamples/2: sz;
    bool needrestorebias = filtertype_==FFTFilter::LowPass
			|| ( filtertype_==FFTFilter::BandPass && minfreq_==0 );
    const float correctbias = needrestorebias ? avg : 0;
    for ( int idx=0; idx<undefvalidxs.size(); idx++ )
	timecplxoutp_.set( firstidx + undefvalidxs[idx], mUdf(float) );
    for ( int idx=0; idx<nrsamples; idx++ )
	setOutputValue( output, 0, idx, z0, 
		    timecplxoutp_.get( firstidx + idx ).real() + correctbias );
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
