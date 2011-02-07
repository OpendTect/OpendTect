/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewer2dmainwin.cc,v 1.3 2011-02-07 16:57:20 cvsbruno Exp $";

#include "uipsviewer2dmainwin.h"

#include "uilabel.h"
#include "uibutton.h"
#include "uiflatviewer.h"
#include "uiflatviewslicepos.h"
#include "uipsviewer2dposdlg.h"
#include "uipsviewer2d.h"
#include "uirgbarraycanvas.h"
#include "uislider.h"
#include "uiprogressbar.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "prestackgather.h"
#include "seisioobjinfo.h"


namespace PreStackView 
{
    
uiViewer2DMainWin::uiViewer2DMainWin( uiParent* p )
    : uiFlatViewMainWin(p,uiFlatViewMainWin::Setup("PreStack Gather view",true)
					     .nrviewers(0).deleteonclose(true))
    , posdlg_(0)
    , control_(0)
    , slicepos_(0)		 
    , startwidth_(80)
    , startheight_(100) 
{
    viewer2d_ = new uiViewer2D( this );
    viewer2d_->setPrefWidth( 5*startwidth_+10 );
    viewer2d_->setPrefHeight( 5*startheight_+10 );
    viewer2d_->enableScrollBars( true );

    makeSliders();
}


void uiViewer2DMainWin::init( const MultiID& mid, int gatherid, bool isinl )
{
    mid_ = mid;
    isinl_ = isinl;
    SeisIOObjInfo info( mid );
    info.getRanges( cs_ );
    is2d_ = info.is2D();

    vwrs_ += viewer2d_->addGatherDisplay( gatherid )->getUiFlatViewer();
    if ( !is2d_ )
    {
	DataPack* dp = DPM(DataPackMgr::FlatID()).obtain( gatherid );
	mDynamicCastGet(PreStack::Gather*,gather,dp)
	if ( gather )
	{
	    const BinID& bid = gather->getBinID();
	    if ( isinl_ )
		cs_.hrg.setInlRange( Interval<int>( bid.inl, bid.crl ) );
	    else
		cs_.hrg.setCrlRange( Interval<int>( bid.inl, bid.crl ) );
	}
	DPM(DataPackMgr::FlatID()).release( gatherid );
    }

    control_ = new uiViewer2DControl( this, *vwrs_[0] );
    addControl( control_ );
    control_->posdlgcalled_.notify(mCB(this,uiViewer2DMainWin,posDlgPushed));

    slicepos_ = new uiSlicePos2DView( this );
    slicepos_->setCubeSampling( cs_ );
    slicepos_->positionChg.notify( mCB(this,uiViewer2DMainWin,posSlcChgCB) );

    reSizeSld(0);
    posDlgPushed(0);
}


#define mSldNrUnits 50

void uiViewer2DMainWin::makeSliders()
{
    uiLabel* dummylbl = new uiLabel( this, "" );
    dummylbl->attach( rightOf, viewer2d_ );
    dummylbl->setStretch( 0, 2 );

    uiSliderExtra::Setup su;
    su.sldrsize_ = 150;
    su.withedit_ = false;
    StepInterval<float> sintv( 1, mSldNrUnits, 1 );
    su.isvertical_ = true;
    versliderfld_ = new uiSliderExtra( this, su, "Vertical Scale" );
    versliderfld_->sldr()->setInterval( sintv );
    versliderfld_->sldr()->setValue( 5 );
    versliderfld_->sldr()->setInverted( true );
    versliderfld_->sldr()->sliderReleased.notify(
	    			mCB(this,uiViewer2DMainWin,reSizeSld) );
    versliderfld_->attach( centeredBelow, dummylbl );
    versliderfld_->setStretch( 0, 0 );

    su.isvertical_ = false;
    horsliderfld_ = new uiSliderExtra( this, su, "Horizontal Scale" );
    horsliderfld_->sldr()->setInterval( sintv );
    horsliderfld_->sldr()->sliderReleased.notify(
				    mCB(this,uiViewer2DMainWin,reSizeSld));
    horsliderfld_->setStretch( 0, 0 );
    horsliderfld_->sldr()->setValue( 5 );
    horsliderfld_->attach( rightBorder, 20 );
    horsliderfld_->attach( ensureLeftOf, versliderfld_ );
    horsliderfld_->attach( ensureBelow, versliderfld_ );
    horsliderfld_->attach( ensureBelow, viewer2d_ );

    zoomratiofld_ = new uiCheckBox( this, "Keep zoom ratio" );
    zoomratiofld_->attach( leftOf, horsliderfld_ );
    zoomratiofld_->setChecked( true );
}


void uiViewer2DMainWin::reSizeSld( CallBacker* cb )
{
    uiSlider* hsldr = horsliderfld_->sldr();
    uiSlider* vsldr = versliderfld_->sldr();
    float hslval = hsldr->getValue();
    float vslval = vsldr->getValue();

    mDynamicCastGet(uiSlider*,sld,cb)
    if ( sld && zoomratiofld_->isChecked() ) 
    {
	bool ishor = sld == hsldr;
	uiSlider* revsld = ishor ? vsldr : hsldr;
	if ( ishor )
	    vslval = hslval;
	else
	    hslval = vslval;

	NotifyStopper ns( revsld->sliderReleased );
	revsld->setValue( hslval );
    }

    const int width = (int)( hslval*startwidth_ );
    const int height = (int)( vslval*startheight_ );
    viewer2d_->doReSize( uiSize( width, height ) );
}


void uiViewer2DMainWin::posDlgPushed( CallBacker* )
{
    if ( !posdlg_ )
    {
	posdlg_ = new uiViewer2DPosDlg( this, cs_ );
	posdlg_->okpushed_.notify( mCB(this,uiViewer2DMainWin,posDlgChgCB) );
    }
    else
    {
	posdlg_->setCubeSampling( cs_ );
    }
    posdlg_->show();
}


void uiViewer2DMainWin::posSlcChgCB( CallBacker* )
{
    if ( slicepos_ ) 
	cs_ = slicepos_->getCubeSampling();
    if ( posdlg_ ) 
	posdlg_->setCubeSampling( cs_ );

    setUpView();
}


void uiViewer2DMainWin::posDlgChgCB( CallBacker* )
{
    if ( posdlg_ ) 
	posdlg_->getCubeSampling( cs_ );
    if ( slicepos_ ) 
	slicepos_->setCubeSampling( cs_ );

    setUpView();
}


void uiViewer2DMainWin::setUpView()
{
    HorSamplingIterator hsit( cs_.hrg );
    const int nrvwrs = cs_.hrg.totalNr();
    const int curnrvwrs = viewer2d_->nrItems();

    uiMainWin win( this );
    uiProgressBar pb( &win );
    pb.setPrefWidthInChar( 50 );
    pb.setStretch( 2, 2 );
    pb.setTotalSteps( nrvwrs );
    win.show();

    viewer2d_->removeAllGatherDisplays();
    vwrs_.erase(); 
    if ( control_ )
	control_->removeAllGathers();

    BinID bid; int nrvwr = 0;
    while ( hsit.next( bid ) )
    {
	setGather( nrvwr, bid );
	pb.setProgress( nrvwr );
	nrvwr++;
    }
    reSizeSld(0);
}


void uiViewer2DMainWin::setGather( int idx, const BinID& bid )
{    
    PreStack::Gather* gather = new PreStack::Gather;
    uiGatherDisplay* gatherdisp;
    if ( gather->readFrom( mid_, bid ) )
    {
	DPM(DataPackMgr::FlatID()).addAndObtain( gather );
	gatherdisp = viewer2d_->addGatherDisplay( gather->id() );
	DPM(DataPackMgr::FlatID()).release( gather );
    }
    else
    {
	gatherdisp = viewer2d_->addGatherDisplay( -1 );
	gatherdisp->setPosition( bid );
	delete gather;
    }
    control_->addGather( *gatherdisp->getUiFlatViewer() );
    vwrs_ += gatherdisp->getUiFlatViewer();
}


#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( tb_, fnm, tt, mCB(this,uiViewer2DControl,cbnm) ); \
    tb_->addButton( but );

uiViewer2DControl::uiViewer2DControl( uiParent* p , uiFlatViewer& vwr )
    : uiFlatViewStdControl(vwr,uiFlatViewStdControl::Setup(p)
			    .withthumbnail(false)
			    .withcoltabed(false)
			    .withedit(true))
    , posdlgcalled_(this)
{
    vwr.rgbCanvas().disableScrollZoom();
    mDynamicCastGet(uiMainWin*,mw,p)
    if ( mw )
	tb_->clear();

    mDefBut(posbut_,"gatherdisplaysettings64.png",gatherPosCB,
	                		"Gather display positions");
    mDefBut(parsbut_,"2ddisppars.png",parsCB,"Set seismic display properties");
}


void uiViewer2DControl::gatherPosCB( CallBacker* )
{
    posdlgcalled_.trigger();
}


void uiViewer2DControl::addGather( uiFlatViewer& gv )
{
    gv.rgbCanvas().disableScrollZoom();
    addViewer( gv );
}


void uiViewer2DControl::removeGather( uiFlatViewer& gv )
{
    vwrs_ -= &gv;
}


void uiViewer2DControl::removeAllGathers()
{
    vwrs_.erase();
}

}; //namepsace
