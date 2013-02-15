/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipsviewer2dmainwin.h"

#include "uilabel.h"
#include "uibutton.h"
#include "uiflatviewer.h"
#include "uiflatviewpropdlg.h"
#include "uiflatviewslicepos.h"
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

static int sStartNrViewers = 10;

namespace PreStackView 
{
    
uiViewer2DMainWin::uiViewer2DMainWin( uiParent* p, const char* title )
    : uiObjectItemViewWin(p,title)
    , posdlg_(0)
    , control_(0)
    , slicepos_(0)	 
    , seldatacalled_(this)
    , axispainter_(0)
    , cs_(false)	  
{
}


void uiViewer2DMainWin::dataDlgPushed( CallBacker* )
{
    seldatacalled_.trigger();
    if ( posdlg_ ) posdlg_->setSelGatherInfos( gatherinfos_ );
}


void uiViewer2DMainWin::posSlcChgCB( CallBacker* )
{
    if ( slicepos_ ) 
	cs_ = slicepos_->getCubeSampling();
    if ( posdlg_ ) 
	posdlg_->setCubeSampling( cs_ );

    setUpView();
}


void uiViewer2DMainWin::setUpView()
{
    uiMainWin win( this, "Creating gather displays ... " );
    uiProgressBar pb( &win );
    pb.setPrefWidthInChar( 50 );
    pb.setStretch( 2, 2 );
    pb.setTotalSteps( mCast(int,gatherinfos_.size()) );
    win.show();

    removeAllGathers();

    int nrvwr = 0;
    for ( int gidx=0; gidx<gatherinfos_.size(); gidx++ )
    {
	setGather( gatherinfos_[gidx] );
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


void uiViewer2DMainWin::setGatherView( uiGatherDisplay* gd,
				       uiGatherDisplayInfoHeader* gdi )
{
    const Interval<double> zrg( cs_.zrg.start, cs_.zrg.stop );
    gd->setPosition( gd->getBinID(), cs_.zrg.width()==0 ? 0 : &zrg );
    gd->updateViewRange();
    uiFlatViewer* fv = gd->getUiFlatViewer();
    gd->displayAnnotation( false );
    fv->appearance().annot_.x1_.showannot_ = false;
    vwrs_ += fv;
    addGroup( gd, gdi );

    if ( !control_ )
    {
	uiViewer2DControl* ctrl = new uiViewer2DControl( *mainviewer_, *fv ); 
	ctrl->posdlgcalled_.notify(
			    mCB(this,uiViewer2DMainWin,posDlgPushed));
	ctrl->datadlgcalled_.notify(
			    mCB(this,uiViewer2DMainWin,dataDlgPushed));
	ctrl->infoChanged.notify(
		mCB(this,uiStoredViewer2DMainWin,displayInfo) );
	control_ = ctrl;
	uiToolBar* tb = control_->toolBar();
	if ( tb )
	    tb->addButton( 
		new uiToolButton( tb, "contexthelp", "Help",
		    mCB(this,uiStoredViewer2DMainWin,doHelp) ) );
    }

    control_->addViewer( *fv );
}


void uiViewer2DMainWin::posDlgPushed( CallBacker* )
{
    if ( !posdlg_ )
    {
	BufferStringSet gathernms;
	getGatherNames( gathernms );
	posdlg_ = new uiViewer2DPosDlg( this, is2D(), cs_, gathernms,
					!isStored() );
	posdlg_->okpushed_.notify( mCB(this,uiViewer2DMainWin,posDlgChgCB) );
    }

    posdlg_->setSelGatherInfos( gatherinfos_ );
    posdlg_->raise();
    posdlg_->show();
}


bool uiViewer2DMainWin::isStored() const
{
    mDynamicCastGet(const uiStoredViewer2DMainWin*,storedwin,this);
    return storedwin;
}


TypeSet<BinID> uiViewer2DMainWin::getStartupPositions( const BinID& bid,
	const StepInterval<int>& trcrg, bool isinl ) const
{
    TypeSet<BinID> bids;
    const int approxstep = trcrg.width()/sStartNrViewers;
    const int starttrcnr = isinl ? bid.crl : bid.inl;
    for ( int trcnr=starttrcnr; trcnr<=trcrg.stop; trcnr+=approxstep )
    {
	const int trcidx = trcrg.nearestIndex( trcnr );
	const int acttrcnr = trcrg.atIndex( trcidx );
	BinID posbid( isinl ? bid.inl : acttrcnr, isinl ? acttrcnr : bid.crl );
	bids.addIfNew( posbid );
    }

    for ( int trcnr=starttrcnr; trcnr>=trcrg.start; trcnr-=approxstep )
    {
	const int trcidx = trcrg.nearestIndex( trcnr );
	const int acttrcnr = trcrg.atIndex( trcidx );
	BinID posbid( isinl ? bid.inl : acttrcnr, isinl ? acttrcnr : bid.crl );
	if ( bids.isPresent(posbid) )
	    continue;
	bids.insert( 0, posbid );
    }

    return bids;
}


uiStoredViewer2DMainWin::uiStoredViewer2DMainWin(uiParent* p,const char* title )
    : uiViewer2DMainWin(p,title)
    , linename_(0)
{
}


void uiStoredViewer2DMainWin::getGatherNames( BufferStringSet& nms) const
{
    nms.erase();
    for ( int idx=0; idx<mids_.size(); idx++ )
    {
	PtrMan<IOObj> gatherioobj = IOM().get( mids_[idx] );
	if ( !gatherioobj ) continue;
	nms.add( gatherioobj->name() );
    }
}


void uiStoredViewer2DMainWin::init( const MultiID& mid, const BinID& bid,
	bool isinl, const StepInterval<int>& trcrg, const char* linename )
{
    mids_ += mid;
    SeisIOObjInfo info( mid );
    is2d_ = info.is2D();
    linename_ = linename;

    if ( is2d_ )
    {
	cs_.hrg.setInlRange( Interval<int>( 1, 1 ) );
	cs_.hrg.setCrlRange( trcrg );
    }
    else
    {
	if ( isinl )
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
			    mCB(this,uiStoredViewer2DMainWin,posSlcChgCB));
    }

    TypeSet<BinID> bids = getStartupPositions( bid, trcrg, isinl );
    gatherinfos_.erase();
    for ( int idx=0; idx<bids.size(); idx++ )
    {
	GatherInfo ginfo;
	ginfo.isselected_ = true;
	ginfo.mid_ = mid;
	ginfo.bid_ = bids[idx];
	ginfo.isselected_ = true;
	ginfo.gathernm_ = info.ioObj()->name();
	gatherinfos_ += ginfo;
	setGather( ginfo );
    }

    reSizeSld(0);
}


void uiStoredViewer2DMainWin::setIDs( const TypeSet<MultiID>& mids  )
{
    mids_.copy( mids );
    TypeSet<GatherInfo> oldginfos = gatherinfos_;
    gatherinfos_.erase();

    for ( int gidx=0; gidx<oldginfos.size(); gidx++ )
    {
	for ( int midx=0; midx<mids_.size(); midx++ )
	{
	    PtrMan<IOObj> gatherioobj = IOM().get( mids_[midx] );
	    if ( !gatherioobj ) continue;
	    GatherInfo ginfo = oldginfos[gidx];
	    ginfo.gathernm_ = gatherioobj->name();
	    ginfo.mid_ = mids_[midx];
	    gatherinfos_ += ginfo;
	}
    }

    setUpView();
}


void uiStoredViewer2DMainWin::setGatherInfo( uiGatherDisplayInfoHeader* info,
					     const GatherInfo& ginfo )
{
    PtrMan<IOObj> ioobj = IOM().get( ginfo.mid_ );
    BufferString nm = ioobj ? ioobj->name() : "";
    info->setData( ginfo.bid_, cs_.defaultDir()==CubeSampling::Inl, is2d_, nm );
}


void uiStoredViewer2DMainWin::posDlgChgCB( CallBacker* )
{
    if ( posdlg_ )
    {
	posdlg_->getCubeSampling( cs_ );
	posdlg_->getSelGatherInfos( gatherinfos_ );
	BufferStringSet gathernms;

	for ( int idx=0; idx<gatherinfos_.size(); idx++ )
	{
	    GatherInfo& ginfo = gatherinfos_[idx];
	    for ( int midx=0; midx<mids_.size(); midx++ )
	    {
		PtrMan<IOObj> gatherioobj = IOM().get( mids_[midx] );
		if ( !gatherioobj ) continue;
		if ( ginfo.gathernm_ == gatherioobj->name() )
		    ginfo.mid_ = mids_[midx];
	    }
	}
    }

    if ( slicepos_ ) 
	slicepos_->setCubeSampling( cs_ );

    setUpView();
}


void uiStoredViewer2DMainWin::setGather( const GatherInfo& gatherinfo )
{
    if ( !gatherinfo.isselected_ ) return;

    Interval<float> zrg( mUdf(float), 0 );
    uiGatherDisplay* gd = new uiGatherDisplay( 0 );
    PreStack::Gather* gather = new PreStack::Gather;
    MultiID mid = gatherinfo.mid_;
    BinID bid = gatherinfo.bid_;
    if ( (is2d_ && gather->readFrom(mid,bid.crl,linename_,0)) 
	|| (!is2d_ && gather->readFrom(mid,bid)) )
    {
	DPM(DataPackMgr::FlatID()).addAndObtain( gather );
	gd->setGather( gather->id() );
	if ( mIsUdf( zrg.start ) )
	   zrg = gd->getZDataRange();
	zrg.include( gd->getZDataRange() );
	DPM(DataPackMgr::FlatID()).release( gather );
    }
    else
    {
	gd->setGather( -1 );
	delete gather;
    }

    uiGatherDisplayInfoHeader* gdi = new uiGatherDisplayInfoHeader( 0 );
    setGatherInfo( gdi, gatherinfo );
    gdi->setOffsetRange( gd->getOffsetRange() );
    setGatherView( gd, gdi );
}


uiSyntheticViewer2DMainWin::uiSyntheticViewer2DMainWin( uiParent* p,
							const char* title )
    : uiViewer2DMainWin(p,title)
{
}


void uiSyntheticViewer2DMainWin::setGatherNames( const BufferStringSet& nms) 
{
    TypeSet<GatherInfo> oldginfos = gatherinfos_;
    gatherinfos_.erase();

    for ( int gidx=0; gidx<oldginfos.size(); gidx++ )
    {
	for ( int nmidx=0; nmidx<nms.size(); nmidx++ )
	{
	    GatherInfo ginfo = oldginfos[gidx];
	    ginfo.gathernm_ = nms.get( nmidx );
	    gatherinfos_ += ginfo;
	}
    }
}


void uiSyntheticViewer2DMainWin::getGatherNames( BufferStringSet& nms) const
{
    nms.erase();
    for ( int idx=0; idx<gatherinfos_.size(); idx++ )
	nms.addIfNew( gatherinfos_[idx].gathernm_ );
}


uiSyntheticViewer2DMainWin::~uiSyntheticViewer2DMainWin()
{ removeDataPacks(); }


void uiSyntheticViewer2DMainWin::posDlgChgCB( CallBacker* )
{
    if ( posdlg_ )
    {
	TypeSet<GatherInfo> gatherinfos;
	posdlg_->getCubeSampling( cs_ );
	posdlg_->getSelGatherInfos( gatherinfos );
	for ( int idx=0; idx<gatherinfos_.size(); idx++ )
	    gatherinfos_[idx].isselected_ = false;
	for ( int idx=0; idx<gatherinfos.size(); idx++ )
	{
	    GatherInfo ginfo = gatherinfos[idx];
	    for ( int idy=0; idy<gatherinfos_.size(); idy++ )
	    {
		GatherInfo& dpinfo = gatherinfos_[idy];
		if ( dpinfo.gathernm_==ginfo.gathernm_ &&
		     dpinfo.bid_==ginfo.bid_ )
		    dpinfo.isselected_ = ginfo.isselected_;
	    }
	}
    }

    if ( slicepos_ ) 
	slicepos_->setCubeSampling( cs_ );

    setUpView();
}


void uiSyntheticViewer2DMainWin::setDataPacks( const TypeSet<GatherInfo>& dps )
{
    gatherinfos_ = dps;
    StepInterval<int> trcrg( mUdf(int), -mUdf(int), 1 );
    cs_.hrg.setInlRange( StepInterval<int>(gatherinfos_[0].bid_.inl,
					   gatherinfos_[0].bid_.inl,1) );
    for ( int idx=0; idx<gatherinfos_.size(); idx++ )
	trcrg.include( gatherinfos_[idx].bid_.crl, false );
    cs_.hrg.setCrlRange( trcrg );
    posDlgPushed( 0 );
    setUpView();
    reSizeSld(0);
}


void uiSyntheticViewer2DMainWin::setGather( const GatherInfo& ginfo )
{
    if ( !ginfo.isselected_ ) return;

    Interval<float> zrg( mUdf(float), 0 );
    uiGatherDisplay* gd = new uiGatherDisplay( 0 );
    PreStack::Gather* gather = new PreStack::Gather;
    DataPack* dp = DPM(DataPackMgr::FlatID()).obtain( ginfo.dpid_ );
    mDynamicCast(PreStack::Gather*,gather,dp);

    if ( !gather )
    {
	gd->setGather( -1 );
	delete gather;
	return;
    }

    gd->setGather( gather->id() );
    uiGatherDisplayInfoHeader* gdi = new uiGatherDisplayInfoHeader( 0 );
    setGatherInfo( gdi, ginfo );
    gdi->setOffsetRange( gd->getOffsetRange() );
    setGatherView( gd, gdi );
}


void uiSyntheticViewer2DMainWin::removeDataPacks()
{
    for ( int idx=0; idx<gatherinfos_.size(); idx++ )
	DPM(DataPackMgr::FlatID()).release( gatherinfos_[idx].dpid_ );
    gatherinfos_.erase();
}



void uiSyntheticViewer2DMainWin::setGatherInfo(uiGatherDisplayInfoHeader* info,
					       const GatherInfo& ginfo )
{
    CubeSampling cs;
    const int modelnr = ginfo.bid_.crl - cs.hrg.stop.crl;
    info->setData( modelnr, ginfo.gathernm_ );
}



#define mDefBut(but,fnm,cbnm,tt) \
    uiToolButton* but = new uiToolButton( tb_, fnm, tt, mCB(this,uiViewer2DControl,cbnm) ); \
    tb_->addButton( but );

uiViewer2DControl::uiViewer2DControl( uiObjectItemView& mw, uiFlatViewer& vwr )
    : uiFlatViewStdControl(vwr,uiFlatViewStdControl::Setup(mw.parent())
			.withstates(true)
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

    mDefBut(posbut,"orientation64",gatherPosCB,"Set positions");
    mDefBut(databut,"gatherdisplaysettings64",gatherDataCB, "Set gather data");
    mDefBut(parsbut,"2ddisppars",parsCB,"Set seismic display properties");

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
