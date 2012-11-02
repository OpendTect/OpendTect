/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Sept 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiseiswvltattr.h"

#include "uiaxishandler.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uifreqtaper.h"
#include "uislider.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "fftfilter.h"
#include "survinfo.h"
#include "wavelet.h"
#include "waveletattrib.h"
#include "windowfunction.h"


uiSeisWvltSliderDlg::uiSeisWvltSliderDlg( uiParent* p, Wavelet& wvlt )
    : uiDialog(p,uiDialog::Setup("","","103.3.5"))
    , wvlt_(&wvlt)
    , orgwvlt_(new Wavelet(wvlt))
    , sliderfld_(0) 
    , wvltattr_(new WaveletAttrib(wvlt))
    , acting(this)       				
{
}     


void uiSeisWvltSliderDlg::constructSlider( uiSliderExtra::Setup& su,
				      const Interval<float>&  sliderrg )
{
    sliderfld_ = new uiSliderExtra( this, su, "wavelet slider" );
    sliderfld_->sldr()->setInterval( sliderrg );
    sliderfld_->sldr()->valueChanged.notify(mCB(this,uiSeisWvltSliderDlg,act));
}


uiSeisWvltSliderDlg::~uiSeisWvltSliderDlg()
{
    delete orgwvlt_;
    delete wvltattr_;
}


//Wavelet rotation dialog

uiSeisWvltRotDlg::uiSeisWvltRotDlg( uiParent* p, Wavelet& wvlt )
    : uiSeisWvltSliderDlg(p,wvlt)
{
    setCaption( "Phase rotation Slider" );
    uiSliderExtra::Setup su; 
    su.lbl_ = "Rotate phase (degrees)";
    su.isvertical_ = true;
    su.sldrsize_ = 250;
    su.withedit_ = true;

    StepInterval<float> sintv( -180.0, 180.0, 1 );
    constructSlider( su, sintv );
    sliderfld_->attach( hCentered );
}     


void uiSeisWvltRotDlg::act( CallBacker* )
{
    const float dphase = sliderfld_->sldr()->getValue();
    float* wvltsamps = wvlt_->samples();
    const float* orgwvltsamps = orgwvlt_->samples();
    const int wvltsz = wvlt_->size();
    Array1DImpl<float> hilsamps( wvltsz );
    wvltattr_->getHilbert( hilsamps );

    for ( int idx=0; idx<wvltsz; idx++ )
	wvltsamps[idx] = (float) ( orgwvltsamps[idx]*cos( dphase*M_PI/180 ) 
		       - hilsamps.get(idx)*sin( dphase*M_PI/180 ) );
    acting.trigger();
}


//Wavelet taper dialog
#define mPaddFac 3
#define mPadSz mPaddFac*wvltsz_/2
uiSeisWvltTaperDlg::uiSeisWvltTaperDlg( uiParent* p, Wavelet& wvlt )
    : uiSeisWvltSliderDlg(p,wvlt)
    , timedrawer_(0)	 
    , freqdrawer_(0)	 
    , isfreqtaper_(false)		 
    , wvltsz_(wvlt.size())
    , freqvals_(new Array1DImpl<float>(mPadSz))
{
    setCaption( "Taper Wavelet" );
    setHelpID("103.3.6");
    uiSliderExtra::Setup su; 
    su.lbl_ = "Taper Wavelet (%)";
    su.sldrsize_ = 220;
    su.withedit_ = true;
    su.isvertical_ = false;

    StepInterval<float> sintv( 0, 100, 1 );
    constructSlider( su, sintv );
    
    mutefld_ = new uiCheckBox( this, "mute zero frequency" ); 
    mutefld_->setChecked();
    mutefld_->attach( rightOf, sliderfld_ );
    mutefld_->activated.notify( mCB( this, uiSeisWvltTaperDlg, act )  );
    
    wvltvals_ = new Array1DImpl<float>( wvltsz_ );
    memcpy( wvltvals_->getData(), wvlt_->samples(), sizeof(float)*wvltsz_ );
  
    uiFuncTaperDisp::Setup s;
    bool istime = SI().zIsTime();
    s.datasz_ = istime ? (int) ( 0.5/SI().zStep() ) : 100;
    s.xaxnm_ = istime ? "Time (s)" : "Depth (m)";
    s.yaxnm_ = "Taper Apmplitude";

    timedrawer_ = new uiFuncTaperDisp( this, s );
    s.leftrg_ = Interval<float> ( 0, mCast(float,s.datasz_/6) );
    s.rightrg_ = Interval<float> ( mCast(float,s.datasz_-1), 
				   mCast(float,s.datasz_) );
    s.is2sided_ = true;
    s.xaxnm_ = istime ? "Frequency (Hz)" : "Wavenumber(/m)";
    s.yaxnm_ = "Gain (dB)";
    s.fillbelowy2_ = true;
    s.drawliney_ = false;
    freqdrawer_ = new uiFuncTaperDisp( this, s );
    freqdrawer_->attach( ensureBelow, timedrawer_ );
    freqdrawer_->taperChanged.notify(mCB(this,uiSeisWvltTaperDlg,act) );

    typefld_ = new uiGenInput( this, "Taper", 
		    BoolInpSpec(true, istime ? "Time" : "Depth","Frequency")); 
    typefld_->valuechanged.notify( mCB(this,uiSeisWvltTaperDlg,typeChoice) );
    typefld_->attach( centeredAbove, timedrawer_ );
    
    float zstep = wvlt_->sampleRate();
    timerange_.set( wvlt_->samplePositions().start, 
		    wvlt_->samplePositions().stop );
    timedrawer_->setFunction( *wvltvals_, timerange_ );
    
    float maxfreq = 0.5f/zstep;
    if ( SI().zIsTime() ) maxfreq = mNINT32( maxfreq );
    freqrange_.set( 0, maxfreq );
    
    FreqTaperSetup ftsu; ftsu.hasmin_ = true, 
    ftsu.minfreqrg_ = s.leftrg_;
    ftsu.allfreqssetable_ = true;
    ftsu.maxfreqrg_ = s.rightrg_;
    freqtaper_ = new uiFreqTaperGrp( this, ftsu, freqdrawer_ );
    freqtaper_->attach( ensureBelow, freqdrawer_ );
    
    typeChoice(0);

    sliderfld_->attach( ensureBelow, freqdrawer_ );
    postFinalise().notify( mCB( this, uiSeisWvltTaperDlg, act ) );
}     


uiSeisWvltTaperDlg::~uiSeisWvltTaperDlg()
{
    delete wvltvals_;
    delete freqvals_;
}


void uiSeisWvltTaperDlg::typeChoice( CallBacker* )
{
    isfreqtaper_ = !typefld_->getBoolValue();
    sliderfld_->display( !isfreqtaper_ );
    mutefld_->display( !isfreqtaper_ );
    freqtaper_->display( isfreqtaper_ );

    timedrawer_->setup().drawliney_ = !isfreqtaper_;
    freqdrawer_->setup().drawliney_ = isfreqtaper_;

    if ( isfreqtaper_ )
	freqdrawer_->setFunction( *freqvals_, freqrange_ );
    else
	timedrawer_->setFunction( *wvltvals_, timerange_ );
}


void uiSeisWvltTaperDlg::act( CallBacker* )
{
    if ( isfreqtaper_ )
    {
	Array1DImpl<float> tmpwvltvals( wvltsz_ );
	memcpy(tmpwvltvals.arr(),orgwvlt_->samples(),sizeof(float)*wvltsz_);
	wvltattr_->applyFreqWindow( *freqdrawer_->window(),
				mPaddFac, tmpwvltvals );
	for ( int idx=0; idx<wvltsz_; idx++ )
	{
	    wvltvals_->set( idx, tmpwvltvals.get(idx) );
	    wvlt_->samples()[idx] = tmpwvltvals.get(idx);
	}
	setTimeData();
    }
    else
    {
	float var = sliderfld_->sldr()->getValue();
	timedrawer_->setWindows( 1-var/100 );

	if ( !timedrawer_->getFuncValues() ) return;
	memcpy( wvlt_->samples(), timedrawer_->getFuncValues(), 
						    sizeof(float)*wvltsz_ );
	if ( mutefld_->isChecked() ) 
	    WaveletAttrib::muteZeroFrequency( *wvltvals_ );

	setFreqData();
    }

    wvltattr_->setNewWavelet( *wvlt_ );
    acting.trigger();
}


void uiSeisWvltTaperDlg::setTimeData()
{
    timedrawer_->setY2Vals( timerange_, wvltvals_->getData(), wvltsz_ );
}


void uiSeisWvltTaperDlg::setFreqData()
{
    Array1DImpl<float> spectrum ( 2*mPadSz );
    wvltattr_->getFrequency( spectrum, mPaddFac );

    for ( int idx=0; idx<mPadSz; idx++ )
	freqvals_->set( idx, spectrum.get(idx) );

    freqdrawer_->setY2Vals( freqrange_, freqvals_->getData(), mPadSz );
}



//Wavelet display property dialog
uiWaveletDispPropDlg::uiWaveletDispPropDlg( uiParent* p, const Wavelet& w )
            : uiDialog(p,Setup(w.name(),"","103.3.8")
		    	 .modal(false))
{
    setCtrlStyle( LeaveOnly );
    properties_ = new uiWaveletDispProp( this, w );
}


uiWaveletDispPropDlg::~uiWaveletDispPropDlg()
{}


//Wavelet display properties
uiWaveletDispProp::uiWaveletDispProp( uiParent* p, const Wavelet& wvlt )
	    : uiGroup(p,"Properties")
	    , wvltattr_(new WaveletAttrib(wvlt))
	    , wvltsz_(wvlt.size())
{
    timerange_.set( wvlt.samplePositions().start, wvlt.samplePositions().stop );
    float maxfreq = 0.5f/wvlt.sampleRate();
    if ( SI().zIsTime() ) maxfreq = mNINT32( maxfreq );
    freqrange_.set( 0, maxfreq );

    for ( int iattr=0; iattr<3; iattr++ )
	addAttrDisp( iattr );
    
    setAttrCurves( wvlt );
}


uiWaveletDispProp::~uiWaveletDispProp()
{
    deepErase( attrarrays_ );
    delete wvltattr_;
}


void uiWaveletDispProp::addAttrDisp( int attridx )
{
    uiFunctionDisplay::Setup fdsu;
    attrarrays_ += new Array1DImpl<float>( wvltsz_ );
    BufferString xname = SI().zIsTime() ? "Frequency (Hz)" : "Wavenumber (/m)"; 
    BufferString yname = "Amplitude";
    fdsu.ywidth_ = 2;

    if ( attridx == 0 )
	xname = SI().zIsTime() ? "Time (s)" : "Depth (m)";
    else if ( attridx == 1 )
    {
	fdsu.fillbelow( true );
	attrarrays_[attridx]->setSize( mPadSz );
    }
    else if ( attridx == 2 )
	yname =  "Phase (degrees)";

    attrdisps_ += new uiFunctionDisplay( this, fdsu );
    if ( attridx )
	attrdisps_[attridx]->attach( alignedBelow, attrdisps_[attridx-1] );

    attrdisps_[attridx]->xAxis()->setName( xname );
    attrdisps_[attridx]->yAxis(false)->setName( yname );
}


void uiWaveletDispProp::setAttrCurves( const Wavelet& wvlt )
{
    memcpy( attrarrays_[0]->getData(), wvlt.samples(), wvltsz_*sizeof(float) );

    wvltattr_->getFrequency( *attrarrays_[1], mPaddFac );
    wvltattr_->getPhase( *attrarrays_[2], true );

    for ( int idx=0; idx<attrarrays_.size(); idx++ )
	attrdisps_[idx]->setVals( idx==0 ? timerange_ : freqrange_, 
				  attrarrays_[idx]->arr(), 
				  idx==1 ? mPadSz : wvltsz_ );
}
