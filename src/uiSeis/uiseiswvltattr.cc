/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Sept 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiswvltattr.cc,v 1.12 2009-11-09 06:24:38 cvsnageswara Exp $";


#include "uiseiswvltattr.h"

#include "uiaxishandler.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uifunctiondisplay.h"
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
    su.lbl_ = "Rotate phase";
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

uiSeisWvltTaperDlg::uiSeisWvltTaperDlg( uiParent* p, Wavelet& wvlt )
    : uiSeisWvltSliderDlg(p,wvlt)
    , window_(0) 
    , drawer_(0)	 
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
    
    const int wvltsz = wvlt.size();

    orgwvltvals_ = new Array1DImpl<float>( wvltsz );
    wvltvals_ = new Array1DImpl<float>( wvltsz );
    memcpy( orgwvltvals_->getData(), wvlt_->samples(), sizeof(float)*wvltsz );
    memcpy( wvltvals_->getData(), wvlt_->samples(), sizeof(float)*wvltsz );
    
    window_ = new ArrayNDWindow( Array1DInfoImpl(wvltsz),false,"CosTaper",0 );

    uiWaveletDispProp::Setup setup; setup.withphase_ = false;
    properties_ = new uiWaveletDispProp( this, *wvlt_, setup );
    drawer_ = properties_->getAttrDisp( 0 );
    
    sliderfld_->attach( ensureBelow, properties_ );
    act(0);
}     


uiSeisWvltTaperDlg::~uiSeisWvltTaperDlg()
{
    delete window_;
    delete wvltvals_;
    delete orgwvltvals_;
    delete properties_;
}


void uiSeisWvltTaperDlg::act( CallBacker* )
{
    float var = sliderfld_->sldr()->getValue();
    if ( !var ) var += 0.01;
    const int wvltsz = wvlt_->size();

    var = 1-var/100;
    const char* winname = "CosTaper";
    window_->setType( winname , var );
    window_->apply( orgwvltvals_, wvltvals_ );
    if ( mutefld_->isChecked() )
	wvltattr_->muteZeroFrequency( *wvltvals_ );

    WindowFunction* winfunc = WinFuncs().create( winname );
    TypeSet<float> xvals;
    for ( int idx=0; idx<wvltsz; idx++ )
    {
	xvals += ( idx - wvltsz/2 )*SI().zStep()*1000;
	wvlt_->samples()[idx] = wvltvals_->get(idx); 
    }

    winfunc->setVariable( var );
    properties_->setAttrCurves( *wvlt_ );
    drawer_->setY2Vals( xvals.arr(), window_->getValues(), wvltsz );

    acting.trigger();
}


/*
void uiSeisWvltFilterDlg::filter( float lowfreq, float highfreq )
{
    const int wvltsz = wvlt_->size();
    FFTFilter filter;
    Array1DImpl<float> orgvals( wvltsz ), vals( wvltsz );
    for ( int idx=0; idx<wvltsz; idx++ )
	orgvals.set( idx, orgwvlt_->samples()[idx] );
    float df = FFT::getDf( wvlt_->sampleRate(), wvltsz );

    filter.FFTBandPassFilter( df, lowfreq, float highfreq, orgvals, vals );
    
    for ( int idx=0; idx<wvltsz; idx++ )
	wvlt_->samples()[idx] = vals.get( idx );

    wvltfiltered.trigger();
}
*/


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
{
}


//Wavelet display properties

#define mPaddFac 3
uiWaveletDispProp::uiWaveletDispProp( uiParent* p, const Wavelet& wvlt, 
				      const Setup& su )
	    : uiGroup(p,"Properties")
	    , wvltsz_(0)
{
    wvltsz_ = wvlt.size();

    const char* phase = su.withphase_ ? "Phase" : 0;
    attrnms_[0] = "Amplitude"; attrnms_[1] = "Frequency", 
    attrnms_[2] = phase; attrnms_[3] = 0;
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
    float zstep = SI().zStep();
    Interval<float> intv( -wvltsz_*zstep*500 , wvltsz_*zstep*500 );
    uiFunctionDisplay::Setup fdsu; 
    fdsu.noy2axis_ = true; fdsu.noy2gridline_ = true;

    attrarrays_ += new Array1DImpl<float>( wvltsz_ );
    const int attridx = attrarrays_.size()-1;
    BufferString xname, yname = attrnms_[attridx];
    if ( isfreq )
    {
	xname += attrnms_[0];
	fdsu.fillbelow( true );
	attrarrays_[attridx]->setSize( mPaddFac*wvltsz_ );
	BufferString tmp; mSWAP( xname, yname, tmp );
    }
    else
	xname += "Time (ms)";
    attrdisps_ += new uiFunctionDisplay( this, fdsu );
    if ( attridx )
	attrdisps_[attridx]->attach( alignedBelow, attrdisps_[attridx-1] );

    attrdisps_[attridx]->xAxis()->setName( xname );
    attrdisps_[attridx]->yAxis(false)->setName( yname );
}


void uiWaveletDispProp::setAttrCurves( const Wavelet& wvlt )
{
    WaveletAttrib wvltattr ( wvlt );
    memcpy( attrarrays_[0]->getData(), wvlt.samples(), wvltsz_*sizeof(float) );
    const int wvltsz = wvlt.size();

    wvltattr.getFrequency( *attrarrays_[1], true );
    if ( attrnms_[2] )
	wvltattr.getPhase( *attrarrays_[2] );

    float zstep = SI().zStep();
    float maxfreq = 0.5/zstep;
    if ( SI().zIsTime() )maxfreq = mNINT( maxfreq );

    for ( int idx=0; idx<attrarrays_.size(); idx++ )
    {
	Interval<float> intv( -wvltsz_*zstep*500 , wvltsz_*zstep*500 );
	if ( idx == 1 )
	    intv.set( 0, maxfreq );
	attrdisps_[idx]->setVals( intv, attrarrays_[idx]->arr(), 
				  idx == 1 ? mPaddFac*wvltsz_/2 : wvltsz_ );
    }
}
