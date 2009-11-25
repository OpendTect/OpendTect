/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Sept 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiswvltattr.cc,v 1.16 2009-11-25 13:33:06 cvsbruno Exp $";


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
    : uiDialog(p,uiDialog::Setup("","",mTODOHelpID))
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
    sliderfld_ = new uiSliderExtra( this, uiSliderExtra::Setup(su.lbl_)
				    .withedit(true)
				    .sldrsize(su.sldrsize_)
				    .isvertical(su.isvertical_),
				    "wavelet slider" );
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
	wvltsamps[idx] = orgwvltsamps[idx]*cos( dphase*M_PI/180 ) 
		       - hilsamps.get(idx)*sin( dphase*M_PI/180 );
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
    s.datasz_ = (int) ( 0.5/SI().zStep() );
    s.xaxnm_ = "Time (ms)";
    s.yaxnm_ = "Apmplitude";
    s.noxgridline_ = true;
    s.noygridline_ = true;

    timedrawer_ = new uiFuncTaperDisp( this, s );
    s.leftrg_ = Interval<float> ( s.datasz_/2-1, s.datasz_/2 );
    s.rightrg_ = Interval<float> ( s.datasz_/2-1, s.datasz_ );
    s.is2sided_ = true;
    s.xaxnm_ = "Frequency (Hz)";
    s.yaxnm_ = "Gain (dB)";
    s.fillbelow_ = true;
    freqdrawer_ = new uiFuncTaperDisp( this, s );
    freqdrawer_->attach( ensureBelow, timedrawer_ );
    freqdrawer_->taperchanged.notify(mCB(this,uiSeisWvltTaperDlg,freqChanged) );

    typefld_ = new uiGenInput( this, "Taper",
				    BoolInpSpec(true,"Time","Frequency") ); 
    typefld_->valuechanged.notify( mCB(this,uiSeisWvltTaperDlg,typeChoice) );
    typefld_->attach( centeredAbove, timedrawer_ );
    
    float zstep = SI().zStep();
    timerange_.set( -wvltsz_*zstep*500 , wvltsz_*zstep*500 );
    timedrawer_->setFunction( *wvltvals_, timerange_ );
    
    float maxfreq = 0.5/zstep;
    if ( SI().zIsTime() ) maxfreq = mNINT( maxfreq );
    freqrange_.set( 0, maxfreq );
    freqdrawer_->setFunction( *freqvals_, freqrange_ );
    
    FreqTaperSetup ftsu; ftsu.hasmin_ = true, 
    ftsu.minfreqrg_ = s.leftrg_;
    ftsu.allfreqssetable_ = true;
    ftsu.maxfreqrg_ = s.rightrg_;
    freqtaper_ = new uiFreqTaperGrp( this, ftsu, freqdrawer_ );
    freqtaper_->attach( ensureBelow, freqdrawer_ );
    
    typeChoice(0);

    sliderfld_->attach( ensureBelow, freqdrawer_ );
    finaliseDone.notify( mCB( this, uiSeisWvltTaperDlg, act ) );

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

    timedrawer_->displayTaper( !isfreqtaper_ );
    freqdrawer_->displayTaper( isfreqtaper_ );

    if ( isfreqtaper_ )
    {
	freqdrawer_->setFunction( *freqvals_, freqrange_ );
	freqChanged(0);
    }
    else
    {
	timedrawer_->setFunction( *wvltvals_, timerange_ );
	act(0);
    }
}


void uiSeisWvltTaperDlg::act( CallBacker* )
{
    float var = sliderfld_->sldr()->getValue();
    timedrawer_->setWindows( 1-var/100 );

    if ( !timedrawer_->getFuncValues() ) return;
    memcpy( wvlt_->samples(), timedrawer_->getFuncValues(), wvltsz_ );
    if ( mutefld_->isChecked() ) wvltattr_->muteZeroFrequency( *wvltvals_ );
    setFreqData();

    wvltattr_->setNewWavelet( *wvlt_ );
    acting.trigger();
}


void uiSeisWvltTaperDlg::freqChanged( CallBacker* )
{
    if ( !freqdrawer_->getFuncValues() ) return;
    memcpy(freqvals_->getData(),freqdrawer_->getFuncValues(),mPadSz);
    setTimeData();
    
    wvltattr_->setNewWavelet( *wvlt_ );
    acting.trigger();
}


void uiSeisWvltTaperDlg::setTimeData()
{
    Array1DImpl<float> doublefreqdata( 2*mPadSz );
    Array1DImpl<float> wvlttmpdata( 2*mPadSz );

    for ( int idx=0; idx<mPadSz; idx++ )
    {
	doublefreqdata.set( idx, freqvals_->get( idx ) );
	doublefreqdata.set( 2*mPadSz-idx-1, freqvals_->get( idx ) );
    }
    wvltattr_->getWvltFromFrequency( doublefreqdata, wvlttmpdata );

    for ( int idx=0; idx<wvltsz_; idx++ )
    {
	float wvltval = wvlttmpdata.get( idx + wvltsz_ );
	wvltvals_->set( idx, wvltval );
	wvlt_->samples()[idx] = wvltval;
    }

    timedrawer_->setVals( timerange_, wvltvals_->getData(), wvltsz_ );
}


void uiSeisWvltTaperDlg::setFreqData()
{
    Array1DImpl<float> spectrum ( 2*mPadSz );
    wvltattr_->getFrequency( spectrum, mPaddFac );

    for ( int idx=0; idx<mPadSz; idx++ )
	freqvals_->set( idx, spectrum.get(idx) );

    freqdrawer_->setVals( freqrange_, freqvals_->getData(), mPadSz );
}



//Wavelet display property dialog

uiWaveletDispPropDlg::uiWaveletDispPropDlg( uiParent* p, const Wavelet& w )
            : uiDialog(p,Setup("Wavelet Properties","","107.4.3").modal(false))
{
    setCtrlStyle( LeaveOnly );
    BufferString winname( w.name() ); winname += " properties";
    setCaption( winname );
    properties_ = new uiWaveletDispProp( this, w );
}


uiWaveletDispPropDlg::~uiWaveletDispPropDlg()
{}


const char* attrnms[] = { "Amplitude", "Frequency", "Phase (degrees)", 0 };
//Wavelet display properties
uiWaveletDispProp::uiWaveletDispProp( uiParent* p, const Wavelet& wvlt )
	    : uiGroup(p,"Properties")
	    , wvltattr_(new WaveletAttrib(wvlt))
	    , wvltsz_(wvlt.size())
{
    float zstep = SI().zStep();
    timerange_.set( -wvltsz_*zstep*500 , wvltsz_*zstep*500 );
    float maxfreq = 0.5/zstep;
    if ( SI().zIsTime() ) maxfreq = mNINT( maxfreq );
    freqrange_.set( 0, maxfreq );

    for ( int iattr=0; attrnms[iattr]; iattr++ )
	addAttrDisp( iattr == 1 );
    
    setAttrCurves( wvlt );
}


uiWaveletDispProp::~uiWaveletDispProp()
{
    deepErase( attrarrays_ );
    delete wvltattr_;
}


void uiWaveletDispProp::addAttrDisp( bool isfreq )
{
    uiFuncTaperDisp::Setup fdsu;
    fdsu.datasz_ = wvltsz_;

    attrarrays_ += new Array1DImpl<float>( wvltsz_ );
    const int attridx = attrarrays_.size()-1;
    BufferString xname, yname = attrnms[attridx];
    if ( isfreq )
    {
	xname += attrnms[0];
	fdsu.fillbelow( true );
	attrarrays_[attridx]->setSize( mPadSz );
	BufferString tmp; mSWAP( xname, yname, tmp );
	fdsu.datasz_ = mPadSz;
    }
    else
	xname += "Time (ms)";
    fdsu.xaxnm_ = xname; 	fdsu.yaxnm_ = yname;

    attrdisps_ += new uiFunctionDisplay( this, fdsu );
    if ( attridx )
	attrdisps_[attridx]->attach( alignedBelow, attrdisps_[attridx-1] );
}


void uiWaveletDispProp::setAttrCurves( const Wavelet& wvlt )
{
    memcpy( attrarrays_[0]->getData(), wvlt.samples(), wvltsz_*sizeof(float) );

    wvltattr_->getFrequency( *attrarrays_[1], mPaddFac );
    wvltattr_->getPhase( *attrarrays_[2], true );

    for ( int idx=0; idx<attrarrays_.size(); idx++ )
	attrdisps_[idx]->setVals( idx==1 ? freqrange_ : timerange_, 
				  attrarrays_[idx]->arr(), 
				  idx==1 ? mPadSz : wvltsz_ );
}
