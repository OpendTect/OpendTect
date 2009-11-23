/*+
_______________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Nov 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uifreqtaper.cc,v 1.1 2009-11-23 15:59:22 cvsbruno Exp $";

#include "uifreqtaper.h"
#include "uiamplspectrum.h"
#include "uiaxishandler.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uifunctiondisplay.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uislicesel.h"
#include "uislider.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "iostrm.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "scaler.h"
#include "survinfo.h"
#include "seisbuf.h"
#include "seisread.h"
#include "seistrc.h"
#include "seisselectionimpl.h"

static const char* winname = "CosTaper";
#define mGetData() isminactive_ ? dd1_ : dd2_;
#define mMaxRg Interval<float>( dd2_.reffreqrg_.start+0.05, datasz_ )
#define mMinRg Interval<float>( 0.05, dd1_.reffreqrg_.stop )
#define mCheckLimitRanges()\
    dd1_.freqrg_.limitTo( mMinRg );\
    dd2_.freqrg_.limitTo( mMaxRg );\
    dd1_.freqrg_.stop = dd1_.reffreqrg_.stop;\
    dd2_.freqrg_.start = dd2_.reffreqrg_.start;
uiFreqTaperDlg::uiFreqTaperDlg( uiParent* p, const Setup& s )
    : uiDialog( p, uiDialog::Setup("Frequency taper",
		    "Select taper parameters at cut-off frequency",mNoHelpID) )
    , freqinpfld_(0)  
    , hasmin_(s.hasmin_)			
    , hasmax_(s.hasmax_)
    , isminactive_(s.hasmin_)
    , datasz_((int)(0.5/(SI().zStep())))
    , cs_(new CubeSampling())		
    , funcvals_(0)					
{
    setCtrlStyle( LeaveOnly );

    dd1_.freqrg_ = s.minfreqrg_;  dd1_.reffreqrg_ = s.minfreqrg_; 
    dd2_.freqrg_ = s.maxfreqrg_;  dd2_.reffreqrg_ = s.maxfreqrg_;
    mCheckLimitRanges();
    setSlopeFromFreq();
    
    uiFuncTaperDisp::Setup su;
    su.leftrg_ = s.minfreqrg_;    
    su.rightrg_ = s.maxfreqrg_; 
    su.datasz_ = datasz_;
    su.is2sided_ = true; 
 
    bool withpreview_ = true; bool is2d = false; bool isinl = true;
    if ( withpreview_ )
    {
	CallBack cbview = mCB(this,uiFreqTaperDlg,previewPushed);
	previewfld_ = new uiPushButton( this, "&Preview spectrum...", cbview, true);
	ZDomain::Info info;
	uiSliceSel::Type tp = is2d ? uiSliceSel::TwoD
				   : (isinl ? uiSliceSel::Inl 
					    : uiSliceSel::Crl);
	CallBack dummycb;

	posdlg_ = new uiSliceSelDlg( this, *cs_, *cs_, dummycb, tp, info );
	posdlg_->grp()->enableApplyButton( false );
	posdlg_->grp()->enableScrollButton( false );
	posdlg_->setModal( true );
    }

    su.xaxnm_ = "Frequency (Hz)"; 	
    su.yaxnm_ = "Gain (dB)";
    su.noxgridline_ = true;
    su.noygridline_ = true;

    drawer_ = new uiFuncTaperDisp( this, su );

    seisnm_ = s.seisnm_;
    const char* tapertxt = "Slope (dB/Octave)";
    varinpfld_ = new uiGenInput( this, "", FloatInpSpec() );
    varinpfld_->attach( leftAlignedBelow, drawer_ );
    varinpfld_->setTitleText ( tapertxt );
    varinpfld_->setValue( dd1_.variable_ );
    varinpfld_->valuechanged.notify(mCB( this, uiFreqTaperDlg, slopeChanged ));
    varinpfld_->valuechanged.notify( mCB(this, uiFreqTaperDlg, taperChged) );
    varinpfld_->setElemSzPol( uiObject::Small );

    inffreqfld_ = new uiGenInput( this, "Start/Stop frequency(Hz)",
				    FloatInpSpec().setName("Min frequency") );
    inffreqfld_->valuechanged.notify( mCB( this, uiFreqTaperDlg, freqChanged));
    inffreqfld_->attach( rightOf, varinpfld_ );
    inffreqfld_->setElemSzPol( uiObject::Small );
				    
    sliderfld_ = new uiSliderExtra( this, uiSliderExtra::Setup("")
				    .withedit(false)
				    .sldrsize(180)
				    .isvertical(false),
				    "slider" );
    sliderfld_->sldr()->setInterval( hasmin_ ? mMinRg : mMaxRg );
    sliderfld_->attach( rightOf, inffreqfld_ );
    sliderfld_->sldr()->valueChanged.notify( 
	    			mCB( this, uiFreqTaperDlg, sliderChanged ) );
    
    supfreqfld_ = new uiGenInput( this, "",
				    FloatInpSpec().setName("Max frequency") );
    supfreqfld_->valuechanged.notify( mCB( this, uiFreqTaperDlg, freqChanged));

    supfreqfld_->attach( rightOf, sliderfld_ );
    supfreqfld_->setElemSzPol( uiObject::Small );

    if ( hasmin_ && hasmax_ )
    {
	freqinpfld_ = new uiGenInput( this, "View ", BoolInpSpec(true, 
					"Min frequency", "Max frequency") );
	freqinpfld_->valuechanged.notify( 
			mCB(this,uiFreqTaperDlg,freqChoiceChged) );
	freqinpfld_->attach( ensureBelow, inffreqfld_ );
	freqinpfld_->attach( centeredBelow, drawer_ );
    }
    uiSeparator* sep = new uiSeparator( this, "Seismic2Log Sep" );
    previewfld_->attach( ensureBelow, sep );
    previewfld_->attach( centeredBelow, drawer_ );

    sep->attach( stretchedBelow, freqinpfld_ ? freqinpfld_ : inffreqfld_  );

    setPercentsFromFreq();
    finaliseDone.notify( mCB(this, uiFreqTaperDlg, taperChged ) );
}


uiFreqTaperDlg::~uiFreqTaperDlg()
{
    delete cs_;
    delete funcvals_;
}


void uiFreqTaperDlg::freqChanged( CallBacker* )
{
    DrawData& dd = mGetData();
    dd.freqrg_ = Interval<float> ( inffreqfld_->getfValue(), 
				   supfreqfld_->getfValue() ) ;
    mCheckLimitRanges();
    setPercentsFromFreq();
    setSlopeFromFreq();
    
    taperChged(0);
}


void uiFreqTaperDlg::sliderChanged( CallBacker* )
{
    NotifyStopper nsf1( inffreqfld_->valuechanged );
    NotifyStopper nsf2( supfreqfld_->valuechanged );
    float sval = sliderfld_->sldr()->getValue();
    if ( isminactive_ )
	inffreqfld_->setValue( sval );
    else
	supfreqfld_->setValue( sval );

    freqChanged(0);
}


void uiFreqTaperDlg::slopeChanged( CallBacker* )
{
    DrawData& dd = mGetData();
    dd.slope_ = varinpfld_->getfValue();
    setFreqFromSlope( dd.slope_ );
    setPercentsFromFreq();
    taperChged(0);
}


#define setToNearestInt(val)\
{\
    int ifr = mNINT( val  );\
    if ( mIsZero(val-ifr,1e-2) )\
	val = ifr;\
}
#define setTo1Decimal(val)\
{\
    val*=10;\
    val = (int)val;\
    val = (float)val/10;\
}
void uiFreqTaperDlg::putToScreen( CallBacker* )
{
    NotifyStopper nsf1( varinpfld_->valuechanged );
    NotifyStopper nsf2( inffreqfld_->valuechanged );
    NotifyStopper nsf3( supfreqfld_->valuechanged );
    NotifyStopper nsf4( sliderfld_->sldr()->valueChanged );

    DrawData& dd = mGetData();

    float freq1 = dd.freqrg_.start;
    float freq2 = dd.freqrg_.stop;

    setTo1Decimal( freq1 );
    setToNearestInt( freq1 ); 
    setTo1Decimal( freq2 );
    setToNearestInt( freq2 );
    
    inffreqfld_->setValue( freq1 );
    supfreqfld_->setValue( freq2 );

    sliderfld_->sldr()->setValue( isminactive_ ? freq1 : freq2 );

    float slope = dd.slope_;
    setTo1Decimal( slope );
    setToNearestInt( slope ); 
    varinpfld_->setValue( slope );
    
    inffreqfld_->setSensitive( hasmin_ && isminactive_ );
    supfreqfld_->setSensitive( hasmax_ && !isminactive_ );
} 


void uiFreqTaperDlg::setPercentsFromFreq()
{
    NotifyStopper nsf1( inffreqfld_->valuechanged );
    NotifyStopper nsf2( supfreqfld_->valuechanged );
    dd1_.variable_ = hasmin_ ? dd1_.freqrg_.start / dd1_.freqrg_.stop : 0;
    dd2_.variable_ = hasmax_ ? ( dd2_.freqrg_.stop - dd2_.freqrg_.start )
			       / ( datasz_ - dd2_.freqrg_.start ) : 0;

    drawer_->setWindows( dd1_.variable_, dd2_.variable_ );
}


#define mDec2Oct 0.301029996 //log(2)
void uiFreqTaperDlg::setFreqFromSlope( float slope )
{
    NotifyStopper nsf1( inffreqfld_->valuechanged );
    NotifyStopper nsf2( supfreqfld_->valuechanged );
    const float slopeindecade = (float)(slope/mDec2Oct);
    const float slopeinhertz = pow( 10, 1/slopeindecade );
    DrawData& dd = mGetData();

    if ( isminactive_ )
	dd1_.freqrg_.start = dd.freqrg_.stop/slopeinhertz;
    else
	dd2_.freqrg_.stop = dd.freqrg_.start*slopeinhertz;

    mCheckLimitRanges();
}


void uiFreqTaperDlg::setSlopeFromFreq()
{
    DrawData& d = mGetData();
    float slope = fabs( 1/Math::Log10( d.freqrg_.stop / d.freqrg_.start ) );
    d.slope_ = slope*mDec2Oct;
}


void uiFreqTaperDlg::taperChged( CallBacker* cb )
{
    drawer_->taperChged(0);
    putToScreen(0);
}


void uiFreqTaperDlg::freqChoiceChged( CallBacker* )
{
    NotifyStopper ns( sliderfld_->sldr()->valueChanged );
    if ( freqinpfld_ ) 
	isminactive_ = freqinpfld_->getBoolValue();
    else
	isminactive_ = hasmin_;

    sliderfld_->sldr()->setInterval( isminactive_ ? mMinRg : mMaxRg );

    setSlopeFromFreq();
    putToScreen(0);
}


void uiFreqTaperDlg::setFreqRange( Interval<float> fqrg )
{ 
    dd1_.freqrg_.start = fqrg.start;
    dd2_.freqrg_.stop = fqrg.stop;
    mCheckLimitRanges()
}


Interval<float> uiFreqTaperDlg::getFreqRange() const
{
    return Interval<float> ( dd1_.freqrg_.start, dd2_.freqrg_.stop );
} 


#define mErrRet(msg) \
{ uiMSG().error(msg); return; }
void uiFreqTaperDlg::previewPushed(CallBacker*)
{
    if ( posdlg_->go() )
    {
	const CubeSampling cs = posdlg_->getCubeSampling();
	IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id));
	const IOObj* ioobj = IOM().getLocal( seisnm_ );
	if ( !ioobj )
	    mErrRet( "Cannot read data" );

	IOObj* seisobj = ioobj->clone();
	SeisTrcReader rdr( seisobj );

	Seis::RangeSelData* sd = new Seis::RangeSelData( cs );
	rdr.setSelData( sd );
	rdr.prepareWork();

	SeisTrcBuf trcset( true );
	SeisBufReader sbfr( rdr, trcset );
	sbfr.execute();

	if ( !trcset.size() )
	    mErrRet( "No data extracted" );

	Array2DImpl<float> arr2d( trcset.size(), trcset.get(0)->size() );

	for ( int trcidx=0; trcidx<trcset.size(); trcidx++ )
	{
	    SeisTrc* trc = trcset.get(trcidx);
	    if ( !trc ) continue; 

	    for ( int idx=0; idx<trc->size(); idx++ )
	    {
		float val = trc->get( idx, 0 );
		if ( mIsUdf(val) )
		    val = 0;
		arr2d.set( trcidx, idx, val );
	    }
	}

	uiAmplSpectrum spec( this );
	spec.setData( arr2d  );
	delete funcvals_;
	funcvals_ = new Array1DImpl<float>( 0 ); 
	spec.getSpectrumData( *funcvals_ );
	drawer_->setup().fillbelowy2_ = true;
	drawer_->setFunction( *funcvals_, spec.getPosRange() );
    }
}



uiFuncTaperDisp::uiFuncTaperDisp( uiParent* p, const Setup& s )
    : uiFunctionDisplay( p, s )
    , is2sided_(s.is2sided_)  
    , window_(0)				
    , funcvals_(0)
    , orgfuncvals_(0)
{
    datasz_ = s.datasz_; 
    leftd_.rg_ = s.leftrg_;   
    rightd_.rg_ = s.rightrg_;
    setWindows( 0, 0 );

    xAxis()->setName( s.xaxnm_ ); 	
    yAxis(false)->setName( s.yaxnm_ );
}


uiFuncTaperDisp::~uiFuncTaperDisp()
{
    delete window_;
    delete leftd_.window_; 
    delete rightd_.window_;
    delete orgfuncvals_; 
}


void uiFuncTaperDisp::setFunction( Array1DImpl<float>& data, Interval<float> rg)
{
    xvals_.erase(); yvals_.erase();
    y2xvals_.erase(); y2yvals_.erase();

    int newsz_ = data.info().getSize(0);
    LinScaler scaler( 0, 0, datasz_, newsz_  );
    leftd_.rg_.stop = scaler.scale( leftd_.rg_.stop );
    rightd_.rg_.start = scaler.scale( rightd_.rg_.start );

    datasz_ = newsz_;
    delete orgfuncvals_;
    orgfuncvals_ = new Array1DImpl<float>( data );
    funcvals_ = &data;
    funcrg_ = rg;

    setWindows( leftd_.paramval_, rightd_.paramval_ );
}



void uiFuncTaperDisp::setWindows( float leftvar, float rightvar )
{
    if ( leftvar == 1 ) leftvar -= 0.01; 

    if ( is2sided_ )
    {
	delete leftd_.window_; leftd_.window_ = 0;
	leftd_.paramval_ = leftvar;
	leftd_.winsz_ = 2*(int)leftd_.rg_.stop;
	if ( leftvar )	
	    leftd_.window_ = new ArrayNDWindow( Array1DInfoImpl(leftd_.winsz_), 
						false, winname, leftvar );

	rightd_.paramval_ = rightvar;
	rightd_.winsz_ = 2*( datasz_ - (int)rightd_.rg_.start );
	delete rightd_.window_; rightd_.window_ = 0;
	if ( rightvar )	
	    rightd_.window_ = new ArrayNDWindow(Array1DInfoImpl(rightd_.winsz_),
						false, winname, 1-rightvar );
    }
    delete window_; window_ =0;
    window_ = new ArrayNDWindow( Array1DInfoImpl(datasz_), false, winname, leftvar );

    taperChged(0);
}


void uiFuncTaperDisp::taperChged( CallBacker* cb )
{
    TypeSet<float> xvals;
    if ( is2sided_ )
    {
	for ( int idx=0; idx<datasz_; idx++ )
	{
	    float val = 0;
	    if ( leftd_.window_ && idx < (int)leftd_.rg_.stop )
		val = leftd_.window_->getValues()[leftd_.winsz_/2+idx];

	    if ( rightd_.window_ && idx > (int)rightd_.rg_.start ) 
		val= rightd_.window_->getValues()[idx-(int)rightd_.rg_.start];

	    xvals += idx;
	    window_->setValue( idx, 1-val );
	}
    }

    if ( funcvals_ )
    {
	window_->apply( orgfuncvals_, funcvals_ );
	setY2Vals( funcrg_, funcvals_->getData(), datasz_ );
	setVals( funcrg_, window_->getValues(), datasz_ );
    }
    else if ( xvals.size() ) 
	setVals( xvals.arr(), window_->getValues(), datasz_ );
}





uiFreqTaperSel::uiFreqTaperSel( uiParent* p, const Setup& su )
	    : uiWindowFunctionSel( p, su )
	    , isminfreq_(su.isminfreq_)
	    , ismaxfreq_(su.ismaxfreq_)
	    , freqtaperdlg_(0)
	    , seisnm_(su.seisnm_)
{
}


void uiFreqTaperSel::winfuncseldlgCB( CallBacker* )
{
    setSelFreqs(0);
    uiFreqTaperDlg::Setup su;
    su.hasmin_ = isminfreq_;            
    su.hasmax_ = ismaxfreq_;
    su.minfreqrg_.set( selfreqrg_.start,  freqrg_.start );
    su.maxfreqrg_.set( freqrg_.stop, selfreqrg_.stop );
    su.seisnm_ = seisnm_;

    delete freqtaperdlg_;
    freqtaperdlg_ = new uiFreqTaperDlg( this, su );
    freqtaperdlg_->windowClosed.notify( mCB(this,uiFreqTaperSel,windowClosed));
    freqtaperdlg_->show();
}


void uiFreqTaperSel::windowClosed( CallBacker* )
{
    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    if ( !windowfuncs_.validIdx(winidx) && !onlytaper_ ) return;

    varinpfld_->setValue( freqtaperdlg_->getFreqRange() );
    windowChangedCB(0);
}


void uiFreqTaperSel::setIsMinMaxFreq(bool min, bool max)
{
    isminfreq_ = min; ismaxfreq_ = max;
    varinpfld_->setSensitive( min, 0, 0 );
    varinpfld_->setSensitive( max, 0 ,1 );
}


Interval<float> uiFreqTaperSel::freqValues() const
{
    return varinpfld_->getFInterval();
}


void uiFreqTaperSel::setInputFreqValue( float val, int fldnr )
{
    varinpfld_->setValue( val, fldnr );
    windowChangedCB(0);
}


void uiFreqTaperSel::setRefFreqs( Interval<float> fint )
{
    freqrg_ = fint;
}


void uiFreqTaperSel::setSelFreqs( CallBacker* )
{
    selfreqrg_.set( freqValues().start, freqValues().stop );
}

