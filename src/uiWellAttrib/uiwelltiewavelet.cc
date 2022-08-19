/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwelltiewavelet.h"

#include "arrayndimpl.h"
#include "flatposdata.h"
#include "survinfo.h"
#include "wavelet.h"
#include "waveletio.h"

#include "uiaxishandler.h"
#include "uiflatviewer.h"
#include "uifunctiondisplay.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseiswvltattr.h"
#include "uitoolbutton.h"
#include "odcomplex.h"


namespace WellTie
{

#define mErrRet(msg,act) { uiMSG().error(msg); act; }
uiWaveletView::uiWaveletView( uiParent* p, ObjectSet<Wavelet>& wvs )
	: uiGroup(p)
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, activeWvltChged(this)
	, wvltset_(wvs)
{
    createWaveletFields( this );
    for ( int idx=0; idx<wvs.size(); idx++ )
    {
	uiwvlts_ += new uiWavelet( this, wvs[idx], idx==0 );
	uiwvlts_[idx]->attach( ensureBelow, activewvltfld_ );
	if ( idx ) uiwvlts_[idx]->attach( rightOf, uiwvlts_[idx-1] );
	uiwvlts_[idx]->wvltChged.notify( mCB(
				    this,uiWaveletView,activeWvltChanged ) );
    }
}


uiWaveletView::~uiWaveletView()
{
    deepErase( uiwvlts_ );
}


void uiWaveletView::createWaveletFields( uiGroup* grp )
{
    grp->setHSpacing( 40 );

    uiString initwnm = tr("Initial");
    uiString estwnm = tr("Deterministic");

    uiLabel* wvltlbl = new uiLabel( this, tr("Set active Wavelet : "));
    activewvltfld_ = new uiGenInput( this, uiString::emptyString(),
				     BoolInpSpec(true,initwnm,estwnm));
    wvltlbl->attach( alignedAbove, activewvltfld_ );
    activewvltfld_->valuechanged.notify(
			 mCB(this, uiWaveletView, activeWvltChanged) );
    setVSpacing ( 0 );
}


void uiWaveletView::redrawWavelets()
{
    for ( int idx=0; idx<uiwvlts_.size(); idx++ )
	uiwvlts_[idx]->drawWavelet();
}


void uiWaveletView::activeWvltChanged( CallBacker* )
{
    const bool isinitactive = activewvltfld_->getBoolValue();
    uiwvlts_[0]->setAsActive( isinitactive );
    uiwvlts_[1]->setAsActive( !isinitactive );
    CBCapsule<bool> caps( isinitactive, this );
    activeWvltChged.trigger( &caps );
}


void uiWaveletView::setActiveWavelet( bool initial )
{
    if ( !activewvltfld_ )
	return;

    activewvltfld_->setValue( initial );
}


bool uiWaveletView::isInitialWvltActive() const
{
    if ( !activewvltfld_ )
	return false;

    return activewvltfld_->getBoolValue();
}


uiWavelet::uiWavelet( uiParent* p, Wavelet* wvlt, bool isactive )
    : uiGroup(p)
    , isactive_(isactive)
    , wvlt_(wvlt)
    , wvltpropdlg_(0)
    , wvltChged(this)
{
    viewer_ = new uiFlatViewer( this );

    wvltbuts_ += new uiToolButton( this, "info", uiStrings::sProperties(),
	    mCB(this,uiWavelet,dispProperties) );
    wvltbuts_[0]->attach( alignedBelow, viewer_ );

    wvltbuts_ += new uiToolButton( this, "phase", tr("Rotate phase"),
	    mCB(this,uiWavelet,rotatePhase) );
    wvltbuts_[1]->attach( rightOf, wvltbuts_[0] );

    wvltbuts_ += new uiToolButton( this, "wavelet_taper", tr("Taper Wavelet"),
	    mCB(this,uiWavelet,taper) );
    wvltbuts_[2]->attach( rightOf, wvltbuts_[1] );

    initWaveletViewer();
    drawWavelet();
    setVSpacing ( 0 );
}


uiWavelet::~uiWavelet()
{
}


void uiWavelet::initWaveletViewer()
{
    FlatView::Appearance& app = viewer_->appearance();
    app.annot_.x1_.name_ = "Amplitude";
    app.annot_.x2_.name_ =  "Time";
    app.annot_.setAxesAnnot( false );
    app.setGeoDefaults( true );
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.overlap_ = 0;
    app.ddpars_.wva_.mappersetup_.cliprate_.set(0,0);
    app.ddpars_.wva_.refline_ = OD::Color::Black();
    app.ddpars_.wva_.mappersetup_.symmidval_ = 0;
    app.setDarkBG( false );
    app.annot_.x1_.hasannot_ = false;
    app.annot_.x2_.hasannot_ = false;
    viewer_->setInitialSize( uiSize(80,100) );
    viewer_->setStretch( 1, 2 );
}


void uiWavelet::rotatePhase( CallBacker* )
{
    Wavelet* orgwvlt = new Wavelet( *wvlt_ );
    uiSeisWvltRotDlg dlg( this, *wvlt_ );
    dlg.acting.notify( mCB(this,uiWavelet,wvltChanged) );
    if ( !dlg.go() )
    {
	*wvlt_ = *orgwvlt;
	wvltChanged(0);
    }
    delete orgwvlt;
}


void uiWavelet::taper( CallBacker* )
{
    Wavelet* orgwvlt = new Wavelet( *wvlt_ );
    uiSeisWvltTaperDlg dlg( this, *wvlt_ );
    dlg.acting.notify( mCB(this,uiWavelet,wvltChanged) );
    if ( !dlg.go() )
    {
	*wvlt_ = *orgwvlt;
	wvltChanged(0);
    }
    delete orgwvlt;
}


void uiWavelet::wvltChanged( CallBacker* )
{
    drawWavelet();
    if ( isactive_ ) wvltChged.trigger();
}


void uiWavelet::dispProperties( CallBacker* )
{
    delete wvltpropdlg_; wvltpropdlg_=0;
    wvltpropdlg_ = new uiWaveletDispPropDlg( this, *wvlt_ );
    wvltpropdlg_ ->go();
}


void uiWavelet::setAsActive( bool isactive )
{
    isactive_ = isactive;
}


void uiWavelet::drawWavelet()
{
    if ( !wvlt_ ) return;

    const int wvltsz = wvlt_->size();
    Array2DImpl<float>* fva2d = new Array2DImpl<float>( 1, wvltsz );
    OD::memCopy( fva2d->getData(), wvlt_->samples(), wvltsz * sizeof(float) );
    FlatDataPack* dp = new FlatDataPack( "Wavelet", fva2d );
    DPM( DataPackMgr::FlatID() ).add( dp );
    dp->setName( wvlt_->name() );

    const bool canupdate = viewer_->enableChange( false );
    viewer_->clearAllPacks();
    viewer_->setPack( FlatView::Viewer::WVA, dp->id(), false );
    StepInterval<double> posns; posns.setFrom( wvlt_->samplePositions() );
    if ( SI().zIsTime() ) posns.scale( SI().zDomain().userFactor() );
    dp->posData().setRange( false, posns );
    viewer_->setViewToBoundingBox();
    viewer_->enableChange( canupdate );
    viewer_->handleChange( sCast(od_uint32,uiFlatViewer::All) );
}

} // namespace WellTie
