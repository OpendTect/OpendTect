/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewer2dmainwin.cc,v 1.9 2011-05-16 13:43:59 cvsbruno Exp $";

#include "uipsviewer2dmainwin.h"

#include "uilabel.h"
#include "uibutton.h"
#include "uiflatviewer.h"
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

#include "ioman.h"
#include "ioobj.h"
#include "prestackgather.h"
#include "seisioobjinfo.h"


namespace PreStackView 
{
    
uiViewer2DMainWin::uiViewer2DMainWin( uiParent* p )
    : uiObjectItemViewWin(p,"PreStack Gather view")
    , posdlg_(0)
    , control_(0)		
    , seldatacalled_(this)
    , isinl_(false)
    , axispainter_(0)
{
}


void uiViewer2DMainWin::init( const MultiID& mid, int gatherid, bool isinl )
{
    mids_ += mid;
    isinl_ = isinl;
    SeisIOObjInfo info( mid );
    info.getRanges( cs_ );
    is2d_ = info.is2D();

    if ( !is2d_ )
    {
	DataPack* dp = DPM(DataPackMgr::FlatID()).obtain( gatherid );
	mDynamicCastGet(PreStack::Gather*,gather,dp)
	if ( gather )
	{
	    const BinID& bid = gather->getBinID();
	    if ( isinl_ )
		cs_.hrg.setInlRange( Interval<int>( bid.inl, bid.inl ) );
	    else
		cs_.hrg.setCrlRange( Interval<int>( bid.crl, bid.crl ) );

	    setGathers( bid ); 
	}
	DPM(DataPackMgr::FlatID()).release( gatherid );
    }

    slicepos_ = new uiSlicePos2DView( this );
    slicepos_->setCubeSampling( cs_ );
    slicepos_->positionChg.notify( mCB(this,uiViewer2DMainWin,posSlcChgCB) );

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
    const int curnrvwrs = mainviewer_->nrItems();

    uiMainWin win( this, "Creating gather displays ... " );
    uiProgressBar pb( &win );
    pb.setPrefWidthInChar( 50 );
    pb.setStretch( 2, 2 );
    pb.setTotalSteps( nrvwrs );
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
	if ( gather->readFrom( mids_[idx], bid ) )
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
	gd->setPosition( bid );
	uiFlatViewer* fv = gd->getUiFlatViewer();
	fv->appearance().annot_.x1_.showannot_ = false;
	vwrs_ += fv;
	uiGatherDisplayInfoHeader* gdi = new uiGatherDisplayInfoHeader( 0 );
	PtrMan<IOObj> ioobj = IOM().get( mids_[idx] );
	BufferString nm = ioobj ? ioobj->name() : "";
	gdi->setData( bid, cs_.defaultDir()==CubeSampling::Inl, nm );
	gdi->setOffsetRange( gd->getOffsetRange() );
	addGroup( gd, gdi );

	if ( !control_ )
	{
	    uiViewer2DControl* ctrl = new uiViewer2DControl( *mainviewer_, 
		    						*vwrs_[0] );
	    ctrl->posdlgcalled_.notify(
		    		mCB(this,uiViewer2DMainWin,posDlgPushed));
	    ctrl->datadlgcalled_.notify(
		    		mCB(this,uiViewer2DMainWin,dataDlgPushed));
	    ctrl->infoChanged.notify( mCB(this,uiViewer2DMainWin,displayInfo) );
	    control_ = ctrl;
	}
	control_->addViewer( *fv );
    }
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
    vwr.rgbCanvas().disableScrollZoom();
    tb_->clear(); delete tb_;

    objectitemctrl_ = new uiObjectItemViewControl( mw );
    tb_ = objectitemctrl_->toolBar();

    mDefBut(posbut_,"orientation64.png",gatherPosCB,"Set positions");
    mDefBut(parsbut_,"2ddisppars.png",parsCB,"Set seismic display properties");
    mDefBut(databut_,"gatherdisplaysettings64.png",gatherDataCB,"Set gather data");
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


}; //namepsace
