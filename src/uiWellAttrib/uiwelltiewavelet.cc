/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltiewavelet.cc,v 1.30 2009-09-23 13:27:47 cvsbruno Exp $";

#include "uiwelltiewavelet.h"

#include "flatposdata.h"
#include "ioman.h"
#include "pixmap.h"
#include "survinfo.h"
#include "wavelet.h"
#include "welltiedata.h"
#include "welltiesetup.h"
#include "welltiegeocalculator.h"

#include "uiaxishandler.h"
#include "uiseiswvltattr.h"
#include "uibutton.h"
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
uiWaveletView::uiWaveletView( uiParent* p, WellTie::DataHolder* dh )
	: uiGroup(p)
	, dataholder_(dh)  
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, activeWvltChged(this)		
{
    createWaveletFields( this );
} 


uiWaveletView::~uiWaveletView()
{
    for ( int idx=0; idx<uiwvlts_.size(); idx++ )
	uiwvlts_[idx]->wvltChged.remove( 
				mCB(this,uiWaveletView,activeWvltChanged) );
    deepErase( uiwvlts_ );
}


void uiWaveletView::createWaveletFields( uiGroup* grp )
{
    grp->setHSpacing( 40 );
    
    activewvltfld_ = new uiGenInput( this, "Active Wavelet : ",
	 			BoolInpSpec(true,"Initial","Estimated")  );
    activewvltfld_->attach( hCentered );
    activewvltfld_->valuechanged.notify(
	   		 mCB(this, uiWaveletView, activeWvltChanged) );
}


void uiWaveletView::initWavelets( )
{
    deepErase( uiwvlts_ );
    for ( int idx=0; idx<dataholder_->wvltset().size(); idx++ )
    {
	uiwvlts_ += new uiWavelet( this, dataholder_->wvltset()[idx], idx==0 );
	uiwvlts_[idx]->attach( ensureBelow, activewvltfld_ );
	if ( idx ) uiwvlts_[idx]->attach( rightOf, uiwvlts_[idx-1] );
	uiwvlts_[idx]->wvltChged.notify( 
				mCB(this,uiWaveletView,activeWvltChanged) );
    }
}


void uiWaveletView::activeWvltChanged( CallBacker* )
{
    const bool isinitactive = activewvltfld_->getBoolValue();
    dataholder_->dpms()->isinitwvltactive_ = activewvltfld_->getBoolValue();
    uiwvlts_[0]->setAsActive( isinitactive );
    uiwvlts_[1]->setAsActive( !isinitactive );
    activeWvltChged.trigger();
}



uiWavelet::uiWavelet( uiParent* p, Wavelet* wvlt, bool isactive )
    : uiGroup(p)
    , isactive_(isactive)  
    , wvlt_(wvlt)
    , wvltpropdlg_(new uiWaveletDispPropDlg(this,wvlt))		 
    , wvltChged(this)							 
{
    viewer_ = new uiFlatViewer( this );
    uiLabel* wvltlbl = new uiLabel( this, wvlt->name() );
    wvltlbl->attach( alignedAbove, viewer_);
    
    wvltbuts_ += new uiToolButton( this, "Properties", "info.png",
	    mCB(this,uiWavelet,dispProperties) );
    wvltbuts_[0]->setToolTip( "Properties" );
    wvltbuts_[0]->attach( alignedBelow, viewer_ );

    wvltbuts_ += new uiToolButton( this, "Rotate", "phase.png",
	    mCB(this,uiWavelet,rotatePhase) );
    wvltbuts_[1]->setToolTip( "Rotate Phase" );
    wvltbuts_[1]->attach( rightOf, wvltbuts_[0] );

    initWaveletViewer();
    drawWavelet();
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


void uiWavelet::rotatePhase( CallBacker* cb )
{
    Wavelet* orgwvlt = new Wavelet( *wvlt_ );
    uiSeisWvltRotDlg dlg( this, wvlt_ );
    dlg.phaserotating.notify( mCB(this,uiWavelet,wvltChanged) );
    if ( !dlg.go() )
    {
	memcpy(wvlt_->samples(),orgwvlt->samples(),wvlt_->size()*sizeof(float));
	wvltChanged(0);
    }
    dlg.phaserotating.remove( mCB(this,uiWavelet,wvltChanged) );
}


void uiWavelet::wvltChanged( CallBacker* )
{
    drawWavelet();
    if ( isactive_ ) wvltChged.trigger();
}


void uiWavelet::dispProperties( CallBacker* )
{
    wvltpropdlg_ ->go();
}


void uiWavelet::setAsActive( bool isactive )
{
    isactive_ = isactive;
}


void uiWavelet::drawWavelet()
{
    if ( !wvlt_) return;

    const int wvltsz = wvlt_->size();
    Array2DImpl<float>* fva2d = new Array2DImpl<float>( 1, wvltsz );
    memcpy( fva2d->getData(), wvlt_->samples(), wvltsz * sizeof(float) );
    
    FlatDataPack* dp = new FlatDataPack( "Wavelet", fva2d );
    DPM( DataPackMgr::FlatID() ).add( dp ); dp->setName( wvlt_->name() );
    viewer_->setPack( true, dp->id(), false );
    DPM( DataPackMgr::FlatID() ).addAndObtain( dp );
    
    StepInterval<double> posns; posns.setFrom( wvlt_->samplePositions() );
    if ( SI().zIsTime() ) posns.scale( SI().zFactor() );
    dp->posData().setRange( false, posns );
    
    viewer_->setPack( true, dp->id(), false );
    viewer_->handleChange( uiFlatViewer::All );
}




/*
class uiWvltSaveDlg : public uiDialog
{
public:

uiWvltSaveDlg( uiParent* p, const Wavelet* wvlt )
            : uiDialog(p,uiDialog::Setup("Save Estimated Wavelet",
	    "Specify wavelet name","107.4.4"))
	    , wvltctio_(*mMkCtxtIOObj(Wavelet))
	    , wvlt_(wvlt)				       
{
    wvltctio_.ctxt.forread = false;
    wvltfld_ = new uiIOObjSel( this, wvltctio_, "Output wavelet" );
}


bool acceptOK( CallBacker* )
{
    if ( !wvltfld_->commitInput() )
	mErrRet( "Please enter a name for the new Wavelet", return false );

    const int wvltsize = wvlt_->size();
    Wavelet wvlt( wvltfld_->getInput(), -wvltsize/2, SI().zStep() );
    wvlt.reSize( wvltsize );
    for( int idx=0; idx<wvltsize; idx++ )
	wvlt.samples()[idx] = wvlt_->samples()[idx];
    wvlt.put( wvltctio_.ioobj );
    return true;
}

    CtxtIOObj&  	wvltctio_;
    uiIOObjSel* 	wvltfld_;
    const Wavelet* 	wvlt_;
};

void uiWaveletView::saveWvltPushed( CallBacker* )
{
    WellTie::uiWvltSaveDlg dlg( this, wvlts_[1] );
    if ( !dlg.go() ) return;
}
*/

}; //namespace WellTie
