/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewer2dmainwin.cc,v 1.4 2011-05-04 15:20:02 cvsbruno Exp $";

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
    , seldatacalled_(this)
    , isinl_(false)
{}


void uiViewer2DMainWin::init( const MultiID& mid, int gatherid, bool isinl )
{
    mids_ += mid;
    isinl_ = isinl;
    SeisIOObjInfo info( mid );
    info.getRanges( cs_ );
    is2d_ = info.is2D();

    uiGatherDisplay* gd = new uiGatherDisplay( 0 );
    gd->setGather( gatherid );
    uiGatherDisplayInfoHeader* gdi = new uiGatherDisplayInfoHeader( 0);
    addGroup( gd, gdi );
    vwrs_ += gd->getUiFlatViewer();

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

	    PtrMan<IOObj> ioobj = IOM().get( mid );
	    BufferString nm = ioobj ? ioobj->name() : "";
	    gdi->setData( bid, isinl, nm ); 
	    gdi->setOffsetRange( gd->getOffsetRange() );
	}
	DPM(DataPackMgr::FlatID()).release( gatherid );
    }

    uiViewer2DControl* ctrl = new uiViewer2DControl( this, *vwrs_[0] );
    ctrl->posdlgcalled_.notify(mCB(this,uiViewer2DMainWin,posDlgPushed));
    ctrl->datadlgcalled_.notify(mCB(this,uiViewer2DMainWin,dataDlgPushed));
    control_ = ctrl;
    control_->addViewer( *gd->getUiFlatViewer() );

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

    uiMainWin win( this );
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


void uiViewer2DMainWin::setGathers( const BinID& bid )
{
    PreStack::Gather* gather = new PreStack::Gather;
    uiGatherDisplay* gd;
    for ( int idx=0; idx<mids_.size(); idx++ )
    {
	const MultiID& mid = mids_[idx];
	gd = new uiGatherDisplay( 0 );
	if ( gather->readFrom( mid, bid ) )
	{
	    DPM(DataPackMgr::FlatID()).addAndObtain( gather );
	    gd->setGather( gather->id() );
	    DPM(DataPackMgr::FlatID()).release( gather );
	}
	else
	{
	    gd->setGather( -1 );
	}

	gd->setPosition( bid );
	uiGatherDisplayInfoHeader* gdi = new uiGatherDisplayInfoHeader( 0 );
	PtrMan<IOObj> ioobj = IOM().get( mid );
	BufferString nm = ioobj ? ioobj->name() : "";
	gdi->setData( bid, cs_.defaultDir()==CubeSampling::Inl, nm );
	gdi->setOffsetRange( gd->getOffsetRange() );
	addGroup( gd, gdi );
	vwrs_ += gd->getUiFlatViewer();
	control_->addViewer( *gd->getUiFlatViewer() );
    }
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
    , datadlgcalled_(this)
{
    vwr.rgbCanvas().disableScrollZoom();
    mDynamicCastGet(uiMainWin*,mw,p)
    if ( mw )
	tb_->clear();

    mDefBut(posbut_,"",gatherPosCB,"Gather display positions");
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
