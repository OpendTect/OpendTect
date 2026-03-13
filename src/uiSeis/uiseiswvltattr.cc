/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseiswvltattr.h"

#include "fftfilter.h"
#include "od_helpids.h"
#include "survinfo.h"

#include "uiaxishandlerbase.h"
#include "uibutton.h"
#include "uifreqtaper.h"
#include "uifunctiondisplayserver.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uislider.h"
#include "wavelet.h"
#include "waveletattrib.h"

constexpr int cSliderSize = 250;

uiSeisWvltSliderDlg::uiSeisWvltSliderDlg( uiParent* p, Wavelet& wvlt )
    : uiDialog(p,Setup(uiString::emptyString(),
		       mODHelpKey(mSeisWvltSliderDlgHelpID)))
    , acting(this)
    , wvltattr_(new WaveletAttrib(wvlt))
    , wvlt_(&wvlt)
    , orgwvlt_(new Wavelet(wvlt))
{
}


void uiSeisWvltSliderDlg::constructSlider( uiSlider::Setup& su,
					   const Interval<float>& sliderrg )
{
    sliderfld_ = new uiSlider( this, su, "wavelet slider" );
    sliderfld_->setInterval( sliderrg );
    mAttachCB( sliderfld_->valueChanged, uiSeisWvltSliderDlg::actCB );
}


uiSeisWvltSliderDlg::~uiSeisWvltSliderDlg()
{
    detachAllNotifiers();
    delete orgwvlt_;
    delete wvltattr_;
}


//Wavelet rotation dialog

uiSeisWvltRotDlg::uiSeisWvltRotDlg( uiParent* p, Wavelet& wvlt )
    : uiSeisWvltSliderDlg(p,wvlt)
{
    setCaption( tr("Phase rotation Slider") );
    uiSlider::Setup su;
    su.lbl_ = tr("Rotate phase (degrees)");
    su.isvertical_ = true;
    su.sldrsize_ = cSliderSize;
    su.withedit_ = true;

    StepInterval<float> sintv( -180.0, 180.0, 1 );
    constructSlider( su, sintv );
    sliderfld_->attach( hCentered );
}


uiSeisWvltRotDlg::~uiSeisWvltRotDlg()
{}


void uiSeisWvltRotDlg::actCB( CallBacker* )
{
    const float dphase = sliderfld_->getFValue();
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


static float getFreqXAxisScaler()
{
    return SI().zIsTime() ? 1.f : 1000.f;
    /*		Hz unchanged
		/ft are converted to /kft
		/m are converted to /km
     */
}

//Wavelet taper dialog
constexpr int cPaddFac = 3;

#define mPadSz cPaddFac*wvltsz_/2
uiSeisWvltTaperDlg::uiSeisWvltTaperDlg( uiParent* p, Wavelet& wvlt )
    : uiSeisWvltSliderDlg(p,wvlt)
    , wvltsz_(wvlt.size())
    , freqvals_(new Array1DImpl<float>(mPadSz))
{
    setCaption( tr("Taper Wavelet") );
    setHelpKey( mODHelpKey(mSeisWvltTaperDlgHelpID) );
    uiSlider::Setup su;
    su.lbl_ = tr("Taper Wavelet (%)");
    su.sldrsize_ = cSliderSize;
    su.withedit_ = true;
    su.isvertical_ = false;

    StepInterval<float> sintv( 0, 100, 1 );
    constructSlider( su, sintv );

    mutefld_ = new uiCheckBox( this, tr("mute zero frequency") );
    mutefld_->setChecked();
    mutefld_->attach( rightOf, sliderfld_ );
    mAttachCB( mutefld_->activated, uiSeisWvltTaperDlg::actCB );

    wvltvals_ = new Array1DImpl<float>( wvltsz_ );
    OD::memCopy( wvltvals_->getData(), wvlt_->samples(), sizeof(float)*wvltsz_);

    const float zstep = wvlt_->sampleRate();
    const float maxfreq = getFreqXAxisScaler() * 0.5f / zstep;
    uiFuncTaperDisp::Setup s;
    const bool istime = SI().zIsTime();
    s.datasz_ = mNINT32(Math::Ceil( maxfreq ));
    s.xaxcaption_ = SI().zDomain().getLabel();
    s.yaxcaption_ = tr("Taper Amplitude");

    timedrawer_ = new uiFuncTaperDisp( this, s );
    s.leftrg_ = Interval<float> ( 0, mCast(float,s.datasz_/6) );
    s.rightrg_ = Interval<float> ( mCast(float,s.datasz_*5/6),
				   mCast(float,s.datasz_) );
    s.is2sided_ = true;
    s.xaxcaption_ = istime ? tr("Frequency (Hz)")
			   : SI().depthsInFeet() ? tr("Wavenumber (/miles)")
						 : tr("Wavenumber (/km)");
    s.yaxcaption_ = tr("Gain (dB)");
    s.fillbelowy2_ = true;
    s.drawliney_ = false;
    freqdrawer_ = new uiFuncTaperDisp( this, s );
    freqdrawer_->attach( ensureBelow, timedrawer_ );
    mAttachCB( freqdrawer_->taperChanged, uiSeisWvltTaperDlg::actCB );

    typefld_ = new uiGenInput( this, tr("Taper"),
			       BoolInpSpec(true,istime ? uiStrings::sTime()
						       : uiStrings::sDepth(),
					   uiStrings::sFrequency()) );
    mAttachCB( typefld_->valueChanged, uiSeisWvltTaperDlg::typeChoiceCB );
    typefld_->attach( centeredAbove, timedrawer_ );

    const float zfact = SI().showZ2UserFactor();
    timerange_.set( wvlt_->samplePositions().start_,
                    wvlt_->samplePositions().stop_ );
    timerange_.scale( zfact );

    freqrange_.set( 0.f, Math::Ceil( maxfreq ) );

    FreqTaperSetup ftsu; ftsu.hasmin_ = true,
    ftsu.minfreqrg_ = s.leftrg_;
    ftsu.allfreqssetable_ = true;
    ftsu.maxfreqrg_ = s.rightrg_;
    freqtaper_ = new uiFreqTaperGrp( this, ftsu, freqdrawer_ );
    freqtaper_->attach( ensureBelow, freqdrawer_ );

    sliderfld_->attach( ensureBelow, freqdrawer_ );
    mAttachCB( postFinalize(), uiSeisWvltTaperDlg::typeChoiceCB );
}


uiSeisWvltTaperDlg::~uiSeisWvltTaperDlg()
{
    detachAllNotifiers();
    delete wvltvals_;
    delete freqvals_;
}


void uiSeisWvltTaperDlg::typeChoiceCB( CallBacker* )
{
    isfreqtaper_ = !typefld_->getBoolValue();
    sliderfld_->display( !isfreqtaper_ );
    mutefld_->display( !isfreqtaper_ );
    freqtaper_->display( isfreqtaper_ );

    timedrawer_->disp().setup().drawliney( !isfreqtaper_ );
    freqdrawer_->disp().setup().drawliney( isfreqtaper_ );

    if ( isfreqtaper_ )
	freqdrawer_->setFunction( *freqvals_, freqrange_ );
    else
    {
	timedrawer_->setFunction( *wvltvals_, timerange_ );
	sliderfld_->valueChanged.trigger();
    }

    actCB( nullptr );

    //TODO: For some reason, it does not get updated when the
    //isfreqtaper_ is false. This fixes it, but why the behavior for freqdrawer_
    //is not the same as timedrawer_ needs to be found
    freqdrawer_->disp().draw();
    timedrawer_->disp().draw();
}


void uiSeisWvltTaperDlg::actCB( CallBacker* )
{
    if ( isfreqtaper_ )
    {
	Array1DImpl<float> tmpwvltvals( wvltsz_ );
	OD::memCopy( tmpwvltvals.arr(), orgwvlt_->samples(),
		     sizeof(float)*wvltsz_ );
	wvltattr_->applyFreqWindow( *freqdrawer_->window(),
				    cPaddFac, tmpwvltvals );
	for ( int idx=0; idx<wvltsz_; idx++ )
	{
	    wvltvals_->set( idx, tmpwvltvals.get(idx) );
	    wvlt_->samples()[idx] = tmpwvltvals.get(idx);
	}
	setTimeData();
    }
    else
    {
	const float var = sliderfld_->getFValue();
	timedrawer_->setWindows( 1-var/100 );

	if ( !timedrawer_->getFuncValues() )
	    return;

	OD::memCopy( wvlt_->samples(), timedrawer_->getFuncValues(),
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
    timedrawer_->disp().setY2Vals( timerange_, wvltvals_->getData(), wvltsz_ );
}


void uiSeisWvltTaperDlg::setFreqData()
{
    Array1DImpl<float> spectrum( 2*mPadSz );
    wvltattr_->getFrequency( spectrum, cPaddFac );

    for ( int idx=0; idx<mPadSz; idx++ )
	freqvals_->set( idx, spectrum.get(idx) );

    freqdrawer_->disp().setY2Vals( freqrange_, freqvals_->getData(), mPadSz );
}



//Wavelet display property dialog
uiWaveletDispPropDlg::uiWaveletDispPropDlg( uiParent* p, const Wavelet& w )
	    : uiDialog(p,Setup(toUiString(w.name()),
			       mODHelpKey(mWaveletDispPropDlgHelpID))
			    .modal(false))
{
    setCtrlStyle( CloseOnly );
    properties_ = new uiWaveletDispProp( this, w );
}


uiWaveletDispPropDlg::~uiWaveletDispPropDlg()
{}


constexpr int cCanvasWidth = 400;
constexpr int cCanvasHeight = 300;
//Wavelet display properties
uiWaveletDispProp::uiWaveletDispProp( uiParent* p, const Wavelet& wvlt )
    : uiGroup(p,"Properties")
    , wvltsz_(wvlt.size())
    , wvltattr_(new WaveletAttrib(wvlt))
{
    timerange_.set( wvlt.samplePositions().start_,
		    wvlt.samplePositions().stop_);
    timerange_.scale( SI().showZ2UserFactor() );
    const float maxfreq = 1.f / (2.f * wvlt.sampleRate());
    const float zfac = getFreqXAxisScaler();
    freqrange_.set( 0.f, Math::Ceil( maxfreq*zfac ) );

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
    uiFuncDispBase::Setup fdsu;
    attrarrays_ += new Array1DImpl<float>( wvltsz_ );
    fdsu.noy2axis( true ).noy2gridline( true );
    fdsu.canvaswidth( cCanvasWidth ).canvasheight( cCanvasHeight );

    const uiString xname = attridx==0
			 ? SI().zDomain().getLabel()
			 : SI().zIsTime() ? tr("Frequency (Hz)")
					  : SI().depthsInFeet()
						 ? tr("Wavenumber (/kft)")
						 : tr("Wavenumber (/km)");

    const uiString yname = attridx == 0 || attridx == 1
			 ? uiStrings::sAmplitude()
			 : tr("Phase (degrees)");

    if ( attridx == 1 )
    {
	fdsu.fillbelow( true );
	attrarrays_[attridx]->setSize( mPadSz );
    }

    attrdisps_ += GetFunctionDisplayServer().createFunctionDisplay( this,
								    fdsu );
    if ( attridx )
	attrdisps_[attridx]->uiobj()->attach( alignedBelow,
					      attrdisps_[attridx-1]->uiobj() );

    attrdisps_[attridx]->xAxis()->setCaption( xname );
    attrdisps_[attridx]->yAxis(false)->setCaption( yname );
}

constexpr float cFreqMaxStep = 7;
constexpr float cPhaseMin = -180.f;
constexpr float cPhaseMax = 180.f;
constexpr float cPhaseStep = 45.f;
constexpr float cMaxNyquistFreq = 1e6f;

void uiWaveletDispProp::setAttrCurves( const Wavelet& wvlt )
{
    OD::memCopy( attrarrays_[0]->getData(), wvlt.samples(),
		 wvltsz_*sizeof(float) );

    wvltattr_->getFrequency( *attrarrays_[1], cPaddFac );
    wvltattr_->getPhase( *attrarrays_[2], true );
    float ampmax = -1.f;
    for ( int idx=attrarrays_[1]->info().getSize(0)/2; idx>=0; idx-- )
    {
	if ( attrarrays_[1]->get(idx) > ampmax )
	    ampmax = attrarrays_[1]->get(idx);
    }

    int idxnoamp = mUdf(int);
    const float noisefloor = ampmax / 1000.f; // noise detection threshold
    for ( int idx=attrarrays_[1]->info().getSize(0)/2; idx>=0; idx-- )
    {
	if ( attrarrays_[1]->get(idx) > noisefloor )
	{
	    idxnoamp = idx;
	    break;
	}
    }

    const float maxfreq = freqrange_.stop_ * mCast(float,idxnoamp) /
			  mCast(float,attrarrays_[1]->info().getSize(0)/2);
    if ( maxfreq > cMaxNyquistFreq )
    {
	uiMSG().error( tr("Invalid Nyquist frequency: %1\n"
			  "The wavelet sampling rate may be too low.")
			 .arg(maxfreq) );
	return;
    }

    for ( int idx=0; idx<attrarrays_.size(); idx++ )
    {
	const int sz = idx==0 ? attrarrays_[idx]->info().getSize(0)
			      : attrarrays_[idx]->info().getSize(0)/2;
	attrdisps_[idx]->setVals( idx==0 ? timerange_ : freqrange_,
				  attrarrays_[idx]->arr(), sz );
    }

    const StepInterval<float> freqrg( 0.f, maxfreq, 1.f );
    attrdisps_[1]->xAxis()->setRange( freqrg.niceInterval(cFreqMaxStep) );
    attrdisps_[2]->xAxis()->setRange( freqrg.niceInterval(cFreqMaxStep) );

    const StepInterval<float> amprg( 0.f, ampmax, 1.f );
    attrdisps_[1]->yAxis(false)->setRange( amprg.niceInterval(cFreqMaxStep) );
    const StepInterval<float> phaserg( cPhaseMin, cPhaseMax, cPhaseStep );
    attrdisps_[2]->yAxis(false)->setRange( phaserg );
}
