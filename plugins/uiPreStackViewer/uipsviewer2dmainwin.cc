/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uipsviewer2dmainwin.cc,v 1.23 2012-07-19 06:58:33 cvsbruno Exp $";

#include "uipsviewer2dmainwin.h"

#include "uilabel.h"
#include "uibutton.h"
#include "uiflatviewer.h"
#include "uiflatviewpropdlg.h"
#include "uiflatviewslicepos.h"
#include "uipsviewer2dposdlg.h"
#include "uipsviewer2d.h"
#include "uipsviewer2dinfo.h"
#include "uirgbarraycanvas.h"
#include "uislider.h"
#include "uiprogressbar.h"
#include "uistatusbar.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "flatposdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "prestackgather.h"
#include "seisioobjinfo.h"


namespace PreStackView 
{
    
uiViewer2DMainWin::uiViewer2DMainWin( uiParent* p, const char* title )
    : uiObjectItemViewWin(p,title)
    , posdlg_(0)
    , control_(0)
    , slicepos_(0)	 
    , seldatacalled_(this)
    , isinl_(false)
    , axispainter_(0)
    , linename_(0)
    , cs_(false)	  
{
}


void uiViewer2DMainWin::init( const MultiID& mid, int gatherid, bool isinl, 
	const StepInterval<int>& trcrg, const char* linename )
{
    mids_ += mid;
    isinl_ = isinl;
    SeisIOObjInfo info( mid );
    is2d_ = info.is2D();
    linename_ = linename;

    DataPack* dp = DPM(DataPackMgr::FlatID()).obtain( gatherid );
    mDynamicCastGet(PreStack::Gather*,gather,dp)
    if ( gather )
    {
	const BinID& bid = gather->getBinID();
	if ( is2d_ )
	{
	    cs_.hrg.setInlRange( Interval<int>( 1, 1 ) );
	    cs_.hrg.setCrlRange( trcrg );
	}
	else
	{
	    if ( isinl_ )
	    {
		cs_.hrg.setInlRange( Interval<int>( bid.inl, bid.inl ) );
		cs_.hrg.setCrlRange( trcrg );
	    }
	    else
	    {
		cs_.hrg.setCrlRange( Interval<int>( bid.crl, bid.crl ) );
		cs_.hrg.setInlRange( trcrg );
	    }
	    slicepos_ = new uiSlicePos2DView( this );
	    slicepos_->setCubeSampling( cs_ );
	    slicepos_->positionChg.notify(
		    		mCB(this,uiViewer2DMainWin,posSlcChgCB));
	}
	cs_.zrg.setFrom( gather->posData().range( false ) );
	setGathers( bid );
    }
    DPM(DataPackMgr::FlatID()).release( gatherid );

    reSizeSld(0);
    posDlgPushed(0);
}


void uiViewer2DMainWin::setIDs( const TypeSet<MultiID>& mids  )
{
    mids_.copy( mids );
    setUpView();
}


void uiViewer2DMainWin::dataDlgPushed( CallBacker* )
{
    seldatacalled_.trigger();
}


void uiViewer2DMainWin::posDlgPushed( CallBacker* )
{
    if ( !posdlg_ )
    {
	posdlg_ = new uiViewer2DPosDlg( this, is2d_, cs_ );
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

    uiMainWin win( this, "Creating gather displays ... " );
    uiProgressBar pb( &win );
    pb.setPrefWidthInChar( 50 );
    pb.setStretch( 2, 2 );
    pb.setTotalSteps( cs_.hrg.totalNr() );
    win.show();

    removeAllGathers();

    BinID bid; int nrvwr = 0;
    while ( hsit.next( bid ) )
    {
	setGathers( bid );
	pb.setProgress( nrvwr );
	nrvwr++;
    }
    reSizeSld(0);
}


void uiViewer2DMainWin::removeAllGathers()
{
    removeAllItems();
    if ( control_ )
	control_->removeAllViewers();
    vwrs_.erase();
}


void uiViewer2DMainWin::reSizeItems()
{
     uiObjectItemViewWin::reSizeItems();
}


void uiViewer2DMainWin::setGathers( const BinID& bid )
{
    uiGatherDisplay* gd;
    PreStack::Gather* gather; 
    Interval<float> zrg( mUdf(float), 0 );
    for ( int idx=0; idx<mids_.size(); idx++ )
    {
	gd = new uiGatherDisplay( 0 );
	gather = new PreStack::Gather;
	if ( (is2d_ && gather->readFrom(mids_[idx],bid.crl,linename_,0)) 
		|| (!is2d_ && gather->readFrom(mids_[idx],bid)) )
	{
	    DPM(DataPackMgr::FlatID()).addAndObtain( gather );
	    gd->setGather( gather->id() );
	    DPM(DataPackMgr::FlatID()).release( gather );
	    if ( mIsUdf( zrg.start ) )
	       zrg = gd->getZDataRange();
	    zrg.include( gd->getZDataRange() );
	}
	else
	{
	    gd->setGather( -1 );
	    delete gather;
	}

	gd->setInitialSize( uiSize( 400, 600 ) );
	gd->setPosition( bid );
	uiFlatViewer* fv = gd->getUiFlatViewer();
	gd->displayAnnotation( false );
	fv->appearance().annot_.x1_.showannot_ = false;
	vwrs_ += fv;
	uiGatherDisplayInfoHeader* gdi = new uiGatherDisplayInfoHeader( 0 );
	PtrMan<IOObj> ioobj = IOM().get( mids_[idx] );
	BufferString nm = ioobj ? ioobj->name() : "";
	gdi->setData( bid, cs_.defaultDir()==CubeSampling::Inl, is2d_, nm );
	gdi->setOffsetRange( gd->getOffsetRange() );
	addGroup( gd, gdi );

	if ( !control_ )
	{
	    uiFlatViewer* dummyfv = new uiFlatViewer(0);
	    uiViewer2DControl* ctrl = new uiViewer2DControl( *mainviewer_,
		   						*dummyfv ); 
	    delete dummyfv;
	    ctrl->posdlgcalled_.notify(
		    		mCB(this,uiViewer2DMainWin,posDlgPushed));
	    ctrl->datadlgcalled_.notify(
		    		mCB(this,uiViewer2DMainWin,dataDlgPushed));
	    ctrl->infoChanged.notify( mCB(this,uiViewer2DMainWin,displayInfo) );
	    control_ = ctrl;
	    uiToolBar* tb = control_->toolBar();
	    if ( tb )
		tb->addButton( 
		    new uiToolButton( tb, "contexthelp", "Help",
			mCB(this,uiViewer2DMainWin,doHelp) ) );
	}
	control_->addViewer( *fv );
    }
}


void uiViewer2DMainWin::doHelp( CallBacker* )
{
    provideHelp( "50.2.2" );
}

void uiViewer2DMainWin::displayInfo( CallBacker* cb )
{
    mCBCapsuleUnpack(IOPar,pars,cb);
    BufferString mesg;
    makeInfoMsg( mesg, pars );
    statusBar()->message( mesg.buf() );
}



#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( tb_, fnm, tt, mCB(this,uiViewer2DControl,cbnm) ); \
    tb_->addButton( but );

uiViewer2DControl::uiViewer2DControl( uiObjectItemView& mw, uiFlatViewer& vwr )
    : uiFlatViewStdControl(vwr,uiFlatViewStdControl::Setup(mw.parent())
			    .withthumbnail(false)
			    .withcoltabed(false)
			    .withedit(false))
    , posdlgcalled_(this)
    , datadlgcalled_(this)
{
    removeViewer( vwr );

    tb_->clear(); delete tb_;

    objectitemctrl_ = new uiObjectItemViewControl( mw );
    tb_ = objectitemctrl_->toolBar();

    mDefBut(posbut_,"orientation64",gatherPosCB,"Set positions");
    mDefBut(parsbut_,"2ddisppars",parsCB,"Set seismic display properties");
    mDefBut(databut_,"gatherdisplaysettings64",gatherDataCB,"Set gather data");
    tb_->addSeparator();
}


void uiViewer2DControl::applyProperties( CallBacker* )
{
    if ( !propdlg_ ) return;

    FlatView::Appearance& app0 = propdlg_->viewer().appearance();
    const int selannot = propdlg_->selectedAnnot();

    for( int ivwr=0; ivwr<vwrs_.size(); ivwr++ )
    {
	if ( !vwrs_[ivwr] ) continue;
	uiFlatViewer& vwr = *vwrs_[ivwr];
	vwr.appearance() = app0;

	const uiWorldRect cv( vwr.curView() );
	FlatView::Annotation& annot = vwr.appearance().annot_;
	if ( (cv.right() > cv.left()) == annot.x1_.reversed_ )
	    { annot.x1_.reversed_ = !annot.x1_.reversed_; flip( true ); }
	if ( (cv.top() > cv.bottom()) == annot.x2_.reversed_ )
	    { annot.x2_.reversed_ = !annot.x2_.reversed_; flip( false ); }

	for ( int idx=0; idx<vwr.availablePacks().size(); idx++ )
	{
	    const DataPack::ID& id = vwr.availablePacks()[idx];
	    if ( app0.ddpars_.wva_.show_ )
		vwr.usePack( true, id, false );
	    if ( app0.ddpars_.vd_.show_ )
		vwr.usePack( false, id, false );
	}

	vwr.setAnnotChoice( selannot );
	vwr.handleChange( FlatView::Viewer::DisplayPars );
	vwr.handleChange( FlatView::Viewer::Annot, false );
    }
}


void uiViewer2DControl::removeAllViewers()
{
    for ( int idx=vwrs_.size()-1; idx>=0; idx-- )
	removeViewer( *vwrs_[idx] );
}


void uiViewer2DControl::gatherPosCB( CallBacker* )
{
    posdlgcalled_.trigger();
}


void uiViewer2DControl::gatherDataCB( CallBacker* )
{
    datadlgcalled_.trigger();
}


void uiViewer2DControl::doPropertiesDialog( int vieweridx, bool dowva )
{
    int ivwr = 0;
    for ( ivwr=0; ivwr<vwrs_.size(); ivwr++ )
    {
	if ( vwrs_[ivwr]->pack( true ) || vwrs_[ivwr]->pack( false ) )
	    break;
    }
    return uiFlatViewControl::doPropertiesDialog( ivwr, dowva );
}

}; //namepsace
