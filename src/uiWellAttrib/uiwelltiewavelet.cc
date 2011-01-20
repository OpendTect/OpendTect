/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltiewavelet.cc,v 1.40 2011-01-20 10:21:39 cvsbruno Exp $";

#include "uiwelltiewavelet.h"

#include "arrayndimpl.h"
#include "flatposdata.h"
#include "ioman.h"
#include "survinfo.h"
#include "wavelet.h"
#include "welltiedata.h"
#include "welltiesetup.h"
#include "welltiegeocalculator.h"

#include "uiaxishandler.h"
#include "uiseiswvltattr.h"
#include "uitoolbutton.h"
#include "uigeninput.h"
#include "uiflatviewer.h"
#include "uifunctiondisplay.h"
#include "uilabel.h"
#include "uimsg.h"

#include <complex>

namespace WellTie
{

#define mErrRet(msg,act) \
{ uiMSG().error(msg); act; }
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
    for ( int idx=0; idx<uiwvlts_.size(); idx++ )
	uiwvlts_[idx]->wvltChged.remove( 
				mCB(this,uiWaveletView,activeWvltChanged) );
    deepErase( uiwvlts_ );
}


#define mSetNm(nm,w) if ( w ) { nm += "("; nm += w->name(); nm += ")"; }
void uiWaveletView::createWaveletFields( uiGroup* grp )
{
    grp->setHSpacing( 40 );
   
    const Wavelet* initw = wvltset_[0];
    BufferString initwnm( "Initial " ); 
    BufferString estwnm( "Estimated" ); 
    mSetNm( initwnm, initw ) 
    uiLabel* wvltlbl = new uiLabel( this, "Set active Wavelet : "); 
    activewvltfld_ = new uiGenInput( this, "",
			    BoolInpSpec(true, initwnm, estwnm)  );
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



uiWavelet::uiWavelet( uiParent* p, Wavelet* wvlt, bool isactive )
    : uiGroup(p)
    , isactive_(isactive)  
    , wvlt_(wvlt)
    , wvltpropdlg_(0)		 
    , wvltChged(this)							 
{
    viewer_ = new uiFlatViewer( this );
    
    wvltbuts_ += new uiToolButton( this, "info.png", "Properties",
	    mCB(this,uiWavelet,dispProperties) );
    wvltbuts_[0]->attach( alignedBelow, viewer_ );

    wvltbuts_ += new uiToolButton( this, "phase.png", "Rotate phase",
	    mCB(this,uiWavelet,rotatePhase) );
    wvltbuts_[1]->attach( rightOf, wvltbuts_[0] );

    wvltbuts_ += new uiToolButton( this, "wavelet_taper.png", "Taper Wavelet",
	    mCB(this,uiWavelet,taper) );
    wvltbuts_[2]->attach( rightOf, wvltbuts_[1] );

    initWaveletViewer();
    drawWavelet();
    setVSpacing ( 0 );
}


uiWavelet::~uiWavelet()
{
    const TypeSet<DataPack::ID> ids = viewer_->availablePacks();
    for ( int idx=ids.size()-1; idx>=0; idx-- )
	DPM( DataPackMgr::FlatID() ).release( ids[idx] );
    delete wvltpropdlg_;
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
    app.ddpars_.wva_.clipperc_.start = app.ddpars_.wva_.clipperc_.stop = 0;
    app.ddpars_.wva_.left_ = Color::NoColor();
    app.ddpars_.wva_.right_ = Color::Black();
    app.ddpars_.wva_.mid_ = Color::Black();
    app.ddpars_.wva_.symmidvalue_ = mUdf(float);
    app.setDarkBG( false );
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
	memcpy(wvlt_->samples(),orgwvlt->samples(),wvlt_->size()*sizeof(float));
	wvltChanged(0);
    }
    dlg.acting.remove( mCB(this,uiWavelet,wvltChanged) );
}


void uiWavelet::taper( CallBacker* )
{
    Wavelet* orgwvlt = new Wavelet( *wvlt_ );
    uiSeisWvltTaperDlg dlg( this, *wvlt_ );
    dlg.acting.notify( mCB(this,uiWavelet,wvltChanged) );
    if ( !dlg.go() )
    {
	memcpy(wvlt_->samples(),orgwvlt->samples(),wvlt_->size()*sizeof(float));
	wvltChanged(0);
    }
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
    memcpy( fva2d->getData(), wvlt_->samples(), wvltsz * sizeof(float) );
    
    FlatDataPack* dp = new FlatDataPack( "Wavelet", fva2d );
    DPM( DataPackMgr::FlatID() ).addAndObtain( dp );
    dp->setName( wvlt_->name() );
    viewer_->setPack( true, dp->id(), false );
    
    StepInterval<double> posns; posns.setFrom( wvlt_->samplePositions() );
    if ( SI().zIsTime() ) posns.scale( SI().zFactor() );
    dp->posData().setRange( false, posns );
    
    viewer_->handleChange( uiFlatViewer::All );
}


}; //namespace WellTie
