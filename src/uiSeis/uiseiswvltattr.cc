/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Sept 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiswvltattr.cc,v 1.15 2009-11-23 15:59:22 cvsbruno Exp $";


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
uiSeisWvltTaperDlg::uiSeisWvltTaperDlg( uiParent* p, Wavelet& wvlt )
    : uiSeisWvltSliderDlg(p,wvlt)
    , isfreqtaper_(true)  
    , timedrawer_(0)	 
    , freqdrawer_(0)	 
    , wvltsz_(wvlt.size())
    , freqvals_(new Array1DImpl<float>(wvltsz_*mPaddFac/2))
    , spectrum_(new Array1DImpl<float>(wvltsz_*mPaddFac))
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
    
    uiWaveletDispProp::Setup setup; setup.withphase_ = false;
    properties_ = new uiWaveletDispProp( this, *wvlt_, setup );
    timedrawer_ = properties_->getAttrDisp( 0 );
    freqdrawer_ = properties_->getAttrDisp( 1 );
    
    typefld_ = new uiGenInput( this, "Taper",
				    BoolInpSpec(true,"Time","Frequency") ); 
    typefld_->valuechanged.notify( mCB( this, uiSeisWvltTaperDlg, act ) );
    typefld_->attach( centeredAbove, properties_ );
    
    timedrawer_->setFunction( *wvltvals_, properties_->getTimeRange() );

    setFreqData();
    freqdrawer_->setFunction( *freqvals_, properties_->getFreqRange() );
    
    sliderfld_->attach( ensureBelow, properties_ );
    act(0);
}     


uiSeisWvltTaperDlg::~uiSeisWvltTaperDlg()
{
    delete wvltvals_;
    delete freqvals_;
    delete spectrum_;
}


void uiSeisWvltTaperDlg::act( CallBacker* )
{
    isfreqtaper_ = !typefld_->getBoolValue();
    uiFuncTaperDisp* drawer = isfreqtaper_ ? freqdrawer_ : timedrawer_;

    float var = sliderfld_->sldr()->getValue();
    drawer->setWindows( 1-var/100 );

    if ( mutefld_->isChecked() ) wvltattr_->muteZeroFrequency( *wvltvals_ );

    if ( isfreqtaper_ ) 
    {
	memcpy(freqvals_->getData(),drawer->getFuncValues(),mPaddFac*wvltsz_/2);
	setTimeData();
    }
    else
    {
	memcpy( wvlt_->samples(), drawer->getFuncValues(), wvltsz_ );
	setFreqData();
    }

    acting.trigger();
}


void uiSeisWvltTaperDlg::setTimeData()
{
    if ( !spectrum_ ) return;

    Array1DImpl<float> tmpdata( spectrum_->info().getSize(0) );
    WaveletAttrib wvltattr ( *wvlt_);
    wvltattr.getWvltFromFrequency( *spectrum_, tmpdata );
    for ( int idx=0; idx<wvltsz_; idx++ )
	wvltvals_->set( idx, spectrum_->get( idx + wvltsz_ ) );

    memcpy( wvlt_->samples(), wvltvals_->getData(), wvltsz_ );

    timedrawer_->setVals( properties_->getTimeRange(), 
	    		  wvlt_->samples(), wvlt_->size() );
}


void uiSeisWvltTaperDlg::setFreqData()
{
    WaveletAttrib wvltattr ( *wvlt_);

    wvltattr.getFrequency( *spectrum_, mPaddFac );
    for ( int idx=0; idx<wvltsz_*mPaddFac/2; idx++ )
	freqvals_->set( idx, spectrum_->get(idx) );

    freqdrawer_->setVals( properties_->getFreqRange(), 
	    		  freqvals_->getData(), 
			  mPaddFac*wvlt_->size()/2 );
}



//Wavelet display property dialog

uiWaveletDispPropDlg::uiWaveletDispPropDlg( uiParent* p, const Wavelet& w )
            : uiDialog(p,Setup("Wavelet Properties","","107.4.3").modal(false))
{
    setCtrlStyle( LeaveOnly );
    BufferString winname( w.name() ); winname += " properties";
    setCaption( winname );
    properties_ = new uiWaveletDispProp( this, w, uiWaveletDispProp::Setup());
}


uiWaveletDispPropDlg::~uiWaveletDispPropDlg()
{}


//Wavelet display properties
uiWaveletDispProp::uiWaveletDispProp( uiParent* p, const Wavelet& wvlt, 
				      const Setup& su )
	    : uiGroup(p,"Properties")
	    , wvltsz_(wvlt.size())
{
    attrnms_[0] = "Amplitude"; 
    attrnms_[1] = "Frequency", 
    attrnms_[2] = su.withphase_ ? "Phase" : 0;
    attrnms_[3] = 0;
    
    float zstep = SI().zStep();
    timerange_.set( -wvltsz_*zstep*500 , wvltsz_*zstep*500 );
    float maxfreq = 0.5/zstep;
    if ( SI().zIsTime() ) maxfreq = mNINT( maxfreq );
    freqrange_.set( 0, maxfreq );

    for ( int iattr=0; attrnms_[iattr]; iattr++ )
	addAttrDisp( iattr == 1 );
    
    setAttrCurves( wvlt );
}


uiWaveletDispProp::~uiWaveletDispProp()
{
    deepErase( attrarrays_ );
}


void uiWaveletDispProp::addAttrDisp( bool isfreq )
{
    uiFuncTaperDisp::Setup fdsu; 
    fdsu.noy2axis_ = true; fdsu.noy2gridline_ = true;
    fdsu.datasz_ = wvltsz_;

    attrarrays_ += new Array1DImpl<float>( wvltsz_ );
    const int attridx = attrarrays_.size()-1;
    BufferString xname, yname = attrnms_[attridx];
    if ( isfreq )
    {
	xname += attrnms_[0];
	fdsu.fillbelow( true );
	attrarrays_[attridx]->setSize( mPaddFac*wvltsz_/2 );
	BufferString tmp; mSWAP( xname, yname, tmp );
	fdsu.datasz_ = mPaddFac*wvltsz_/2;
    }
    else
	xname += "Time (ms)";
    fdsu.xaxnm_ = xname; 	fdsu.yaxnm_ = yname;

    attrdisps_ += new uiFuncTaperDisp( this, fdsu );
    if ( attridx )
	attrdisps_[attridx]->attach( alignedBelow, attrdisps_[attridx-1] );
}


void uiWaveletDispProp::setAttrCurves( const Wavelet& wvlt )
{
    WaveletAttrib wvltattr ( wvlt );
    memcpy( attrarrays_[0]->getData(), wvlt.samples(), wvltsz_*sizeof(float) );

    wvltattr.getFrequency( *attrarrays_[1], mPaddFac );
    if ( attrnms_[2] )
	wvltattr.getPhase( *attrarrays_[2], true );

    for ( int idx=0; idx<attrarrays_.size(); idx++ )
	attrdisps_[idx]->setFunction( *attrarrays_[idx], 
					idx==1 ? freqrange_ : timerange_ );
}
