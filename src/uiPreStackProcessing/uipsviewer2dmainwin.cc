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
#include "uicolortable.h"
#include "uiflatviewer.h"
#include "uiflatviewpropdlg.h"
#include "uiflatviewslicepos.h"
#include "uiprestackanglemute.h"
#include "uipsviewer2d.h"
#include "uipsviewer2dinfo.h"
#include "uiprestackprocessor.h"
#include "uiioobjsel.h"
#include "uigraphicsitemimpl.h"
#include "uirgbarraycanvas.h"
#include "uislider.h"
#include "uimsg.h"
#include "uiprogressbar.h"
#include "uisaveimagedlg.h"
#include "uistatusbar.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "coltabsequence.h"
#include "ctxtioobj.h"
#include "flatposdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "prestackprocessor.h"
#include "prestackanglemute.h"
#include "prestackanglecomputer.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"
#include "prestackgather.h"
#include "randcolor.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "windowfunction.h"


static int sStartNrViewers = 8;

namespace PreStackView 
{
    
uiViewer2DMainWin::uiViewer2DMainWin( uiParent* p, const char* title )
    : uiObjectItemViewWin(p,uiObjectItemViewWin::Setup(title).startwidth(800))
    , posdlg_(0)
    , control_(0)
    , slicepos_(0)	 
    , seldatacalled_(this)
    , axispainter_(0)
    , cs_(false)
    , preprocmgr_(0)
{
    setDeleteOnClose( true );
}


uiViewer2DMainWin::~uiViewer2DMainWin()
{
    deepErase( mutes_ );
    deepErase( gd_ );
    deepErase( gdi_ );
    delete posdlg_;
    delete preprocmgr_;
}


void uiViewer2DMainWin::snapshotCB( CallBacker* )
{
    uiSaveWinImageDlg snapshotdlg( this );
    snapshotdlg.go();
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


class uiPSPreProcessingDlg : public uiDialog
{
public:
uiPSPreProcessingDlg( uiParent* p, PreStack::ProcessManager& ppmgr,
		      const CallBack& cb )
    : uiDialog(p,uiDialog::Setup("Preprocessing","","103.2.13") )
    , cb_(cb)
{
    preprocgrp_ = new PreStack::uiProcessorManager( this, ppmgr );
}


protected:

bool acceptOK( CallBacker* )
{
    if ( preprocgrp_->isChanged() )
    {
	 const int ret =
	     uiMSG().askSave("Current settings are not saved.\n"
			     "Do you want to save them?");
	 if ( ret==-1 )
	     return false;
	 else if ( ret==1 )
	     preprocgrp_->save();
    }
    cb_.doCall( this );
    return true;
}


PreStack::uiProcessorManager*	preprocgrp_;
CallBack			cb_;
};



void uiViewer2DMainWin::preprocessingCB( CallBacker* )
{
    if ( !preprocmgr_ )
	preprocmgr_ = new PreStack::ProcessManager();
    uiPSPreProcessingDlg ppdlg( this, *preprocmgr_,
	    			mCB(this,uiViewer2DMainWin,applyPreProcCB) );
    ppdlg.go();
}


void uiViewer2DMainWin::applyPreProcCB( CallBacker* )
{ setUpView(); }


void uiViewer2DMainWin::setUpView()
{
    ColTab::MapperSetup prevvdmapper;
    ColTab::MapperSetup prevwvamapper;
    BufferString vdctab;
    if ( nrViewers() )
    {
	prevvdmapper = vwrs_[0]->appearance().ddpars_.vd_.mappersetup_;
	prevwvamapper = vwrs_[0]->appearance().ddpars_.wva_.mappersetup_;
	vdctab = vwrs_[0]->appearance().ddpars_.vd_.ctab_;
    }

    uiMainWin win( this, "Creating gather displays ... " );
    uiProgressBar pb( &win );
    pb.setPrefWidthInChar( 50 );
    pb.setStretch( 2, 2 );
    pb.setTotalSteps( mCast(int,gatherinfos_.size()) );
    win.show();

    removeAllGathers();

    if ( preprocmgr_ && (!preprocmgr_->nrProcessors() || !preprocmgr_->reset()))
	return uiMSG().error( "Can not preprocess data" );

    int nrvwr = 0;
    for ( int gidx=0; gidx<gatherinfos_.size(); gidx++ )
    {
	setGather( gatherinfos_[gidx] );
	pb.setProgress( nrvwr );
	nrvwr++;
    }

    control_->setGatherInfos( gatherinfos_ );
    for ( int idx=0; idx<nrViewers(); idx++ )
    {
	ColTab::MapperSetup& vdmapper =
	    vwrs_[idx]->appearance().ddpars_.vd_.mappersetup_;
	vdmapper = prevvdmapper;
	vwrs_[idx]->appearance().ddpars_.vd_.ctab_ = vdctab;
	ColTab::MapperSetup& wvamapper =
	    vwrs_[idx]->appearance().ddpars_.wva_.mappersetup_;
	wvamapper = prevwvamapper;
    }
    
    displayMutes();
    reSizeSld(0);
}


void uiViewer2DMainWin::removeAllGathers()
{
    removeAllItems();
    if ( control_ )
	control_->removeAllViewers();
    vwrs_.erase();

    deepErase( gd_ );
    deepErase( gdi_ );
}


void uiViewer2DMainWin::reSizeItems()
{
     uiObjectItemViewWin::reSizeItems();
}


void uiViewer2DMainWin::doHelp( CallBacker* )
{
    provideHelp( "51.1.0" );
}


void uiViewer2DMainWin::displayMutes()
{
    for ( int gidx=0; gidx<nrItems(); gidx++ )
    {
	uiObjectItem* item = mainViewer()->getItem( gidx );
	mDynamicCastGet(uiGatherDisplay*,gd,item->getGroup());
	if ( !gd ) continue;
	
	gd->getUiFlatViewer()->handleChange( FlatView::Viewer::Auxdata );
	for ( int muteidx=0; muteidx<mutes_.size(); muteidx++ )
	{
	    const PreStack::MuteDef* mutedef = mutes_[muteidx];
	    const BinID& bid = gd->getBinID();
	    FlatView::AuxData* muteaux =
		gd->getUiFlatViewer()->createAuxData( mutedef->name() );
	    muteaux->linestyle_.color_ = mutecolors_[muteidx];
	    muteaux->linestyle_.type_ = LineStyle::Solid;
	    muteaux->linestyle_.width_ = 3;

	    StepInterval<float> offsetrg = gd->getOffsetRange();
	    offsetrg.step = offsetrg.width()/20.0f;
	    const int sz = offsetrg.nrSteps()+1;
	    muteaux->poly_.setCapacity( sz );
	    for ( int offsidx=0; offsidx<sz; offsidx++ )
	    {
		const float offset = offsetrg.atIndex( offsidx );
		const float val = mutedef->value( offset, bid );
		muteaux->poly_ +=  FlatView::Point( offset, val );
	    }

	    muteaux->namepos_ = sz/2;
	    gd->getUiFlatViewer()->addAuxData( muteaux );
	}
    }
}


void uiViewer2DMainWin::clearAuxData()
{
    for ( int gidx=0; gidx<nrItems(); gidx++ )
    {
	uiObjectItem* item = mainViewer()->getItem( gidx );
	mDynamicCastGet(uiGatherDisplay*,gd,item->getGroup());
	if ( !gd ) continue;
	uiFlatViewer* vwr = gd->getUiFlatViewer();
	vwr->removeAllAuxData();
    }
}


void uiViewer2DMainWin::loadMuteCB( CallBacker* cb )
{
    uiIOObjSelDlg mutesel( this, *mMkCtxtIOObj(MuteDef),
	    		   "Select Mute for display", true );
    if ( mutesel.go() )
    {
	clearAuxData();
	deepErase( mutes_ );
	mutecolors_.erase();
	for ( int idx=0; idx<mutesel.selGrp()->nrSel(); idx++ )
	{
	    const MultiID& muteid = mutesel.selGrp()->selected( idx );
	    PtrMan<IOObj> muteioobj = IOM().get( muteid );
		if ( !muteioobj ) continue;
	    PreStack::MuteDef* mutedef = new PreStack::MuteDef;
	    BufferString errmsg;
	    if ( !MuteDefTranslator::retrieve(*mutedef,muteioobj,errmsg) )
	    {
		uiMSG().error( errmsg );
		continue;
	    }

	    mutes_ += mutedef;
	    mutecolors_ += getRandStdDrawColor();
	}

	displayMutes();
    }
}


PreStack::Gather* uiViewer2DMainWin::getAngleGather( 
					    const PreStack::Gather& gather, 
					    const PreStack::Gather& angledata,
					    const Interval<int>& anglerange )
{
    const FlatPosData& fp = gather.posData();
    const StepInterval<double> x1rg( anglerange.start, anglerange.stop, 1 );
    const StepInterval<double> x2rg = fp.range( false );
    FlatPosData anglefp;
    anglefp.setRange( true, x1rg );
    anglefp.setRange( false, x2rg );

    PreStack::Gather* anglegather = new PreStack::Gather ( anglefp );
    const int offsetsize = fp.nrPts( true );
    const int zsize = fp.nrPts( false );

    const Array2D<float>& anglevals = angledata.data();
    const Array2D<float>& ampvals = gather.data();
    Array2D<float>& anglegathervals = anglegather->data();
    anglegathervals.setAll( 0 );

    ManagedObjectSet<PointBasedMathFunction> vals;
    for ( int zidx=0; zidx<zsize; zidx++ )
    {
	vals += new PointBasedMathFunction( 
				    PointBasedMathFunction::Linear,
				    PointBasedMathFunction::None );

	float prevangleval = mUdf( float );
	for ( int ofsidx=0; ofsidx<offsetsize; ofsidx++ )
	{
	    const float angleval = anglevals.get( ofsidx, zidx ) * 180/M_PIf;
	    if ( mIsEqual(angleval,prevangleval,1e-3) )
		continue;

	    const float ampval = ampvals.get( ofsidx, zidx );
	    vals[zidx]->add( angleval, ampval );
	    prevangleval = angleval;
	}

	const int x1rgsize = x1rg.nrSteps() + 1;
	for ( int idx=0; idx<x1rgsize; idx++ )
	{
	    const float angleval = mCast( float, x1rg.atIndex(idx) );
	    const float ampval = vals[zidx]->getValue( angleval );
	    if ( mIsUdf(ampval) )
		continue;

	    anglegathervals.set( idx, zidx, ampval );
	}
    }

    return anglegather;
}


void uiViewer2DMainWin::setAngleGather( int idx, PreStack::Gather* anglegather )
{
    if ( !gd_.validIdx(idx) || !gd_[idx] )
    { delete anglegather; return; }

    DPM(DataPackMgr::FlatID()).addAndObtain( anglegather );
    gd_[idx]->setVDGather( anglegather->id() );
    gd_[idx]->updateViewRange();
    DPM(DataPackMgr::FlatID()).release( anglegather );
}


void uiViewer2DMainWin::convAngleDatatoDegrees( PreStack::Gather* angledata )
{
    if ( !angledata || !angledata->data().getData() )
	return;

    Array2D<float>& data = angledata->data();
    float* ptr = data.getData();
    const int size = mCast(int,data.info().getTotalSz());

    for( int idx = 0; idx<size; idx++ )
	ptr[idx] *= 180 / M_PIf;
}


void uiViewer2DMainWin::setAngleData( int idx,
				      PreStack::Gather* gather, 
				      PreStack::Gather* angledata,
				      const Interval<int>& anglerange )
{
    if ( !gd_.validIdx(idx) || !(gd_[idx] && gd_[idx]->getUiFlatViewer()) )
    { delete gather; delete angledata; return; }

    uiFlatViewer* vwr = gd_[idx]->getUiFlatViewer();
    vwr->appearance().ddpars_.vd_.ctab_ = ColTab::Sequence::sKeyRainbow();
    ColTab::MapperSetup& mapper = vwr->appearance().ddpars_.vd_.mappersetup_;
    mapper.type_ = ColTab::MapperSetup::Fixed;
    mapper.range_ = Interval<float>( (float) anglerange.start, 
				     (float) anglerange.stop );
    
    convAngleDatatoDegrees( angledata );
    GatherInfo dummyginfo;
    dummyginfo.isstored_ = true;
    dummyginfo.bid_ = gather->getBinID();
    dummyginfo.mid_ = gather->getStoredID();
    const int actginfoidx = gatherinfos_.indexOf( dummyginfo );
    DataPack::ID ppgatherid = -1;
    if ( preprocmgr_ && actginfoidx<0 )
	ppgatherid = getPreProcessedID( gatherinfos_[actginfoidx] );

    BufferString angledpnm( gather->name(), " Incidence Angle" );
    angledata->setName( angledpnm );
    DPM(DataPackMgr::FlatID()).addAndObtain( gather );
    DPM(DataPackMgr::FlatID()).addAndObtain( angledata );
    gd_[idx]->setWVAGather( ppgatherid>=0 ? ppgatherid : gather->id() );
    gd_[idx]->setVDGather( angledata->id() );
    gd_[idx]->updateViewRange();
    DPM(DataPackMgr::FlatID()).release( angledata );
    DPM(DataPackMgr::FlatID()).release( gather );
}


void uiViewer2DMainWin::angleGatherCB( CallBacker* )
{
    displayAngle( true );
}


void uiViewer2DMainWin::angleDataCB( CallBacker* )
{
    displayAngle( false );
}


void uiViewer2DMainWin::displayAngle( bool isanglegather )
{
    FixedString windowtitle = isanglegather ? "Angle Gather Display" 
					    : "Angle Data Display";

    BufferString windowcaption = "Specify Parameters for ";
    windowcaption += windowtitle;
    uiDialog angledisplaydlg( this, uiDialog::Setup(windowtitle,
						windowcaption,"51.1.3") );

    PreStack::AngleCompParams params;
    if ( !isanglegather ) params.anglerange_ = Interval<int>( 0 , 60 );
    PreStack::uiAngleCompGrp anglegrp( &angledisplaydlg, params, false, false );
    if ( !angledisplaydlg.go() || !anglegrp.acceptOK() )
	return;

    PreStack::VelocityBasedAngleComputer velangcomp;
    velangcomp.setMultiID( params.velvolmid_ );
    velangcomp.setRayTracer( params.raypar_ );
    velangcomp.setSmoothingPars( params.smoothingpar_ );
    const int nrgathers = gatherinfos_.size();
    PreStack::Gather* angledata = 0;
    
    int gatheridx = -1; 
    for ( int idx=0; idx<nrgathers; idx++ )
    {
	if ( !gatherinfos_[idx].isselected_ ) continue;

	gatheridx++;
	const MultiID mid = gatherinfos_[idx].mid_;
	const BinID bid = gatherinfos_[idx].bid_;
	PreStack::Gather* gather = new PreStack::Gather;

	if ( !gather->readFrom(mid,bid) )
	{ delete gather; continue; }
	
	const FlatPosData& fp = gather->posData();
	velangcomp.setOutputSampling( fp );
	velangcomp.setTraceID( gather->getBinID() );
	angledata = velangcomp.computeAngles();
	if ( !angledata )
	{ delete gather; continue; }

	if ( isanglegather )
	{
	    PreStack::Gather* anglegather = getAngleGather( *gather, *angledata, 
							   params.anglerange_ );
	    if ( anglegather )
		setAngleGather( gatheridx, anglegather );

	    delete gather; delete angledata;
	}
	else
	    setAngleData( gatheridx, gather, angledata, params.anglerange_ );
    
    }
}


void uiViewer2DMainWin::displayInfo( CallBacker* cb )
{
    mCBCapsuleUnpack(IOPar,pars,cb);
    BufferString mesg;
    makeInfoMsg( mesg, pars );
    statusBar()->message( mesg.buf() );
}


class uiPSMultiPropDlg : public uiDialog
{
public:
uiPSMultiPropDlg( uiParent* p, ObjectSet<uiFlatViewer>& vwrs,
		     const CallBack& cb )
    : uiDialog(p,uiDialog::Setup("Set properties for data","",""))
    , vwrs_(vwrs)
    , cb_(cb)
{
    uiLabeledComboBox* lblcb =
	new uiLabeledComboBox( this, "Select gather" );
    datasetcb_ = lblcb->box();
    datasetcb_->selectionChanged.notify(
	    mCB(this,uiPSMultiPropDlg,gatherChanged) );
    uiPushButton* propbut =
	new uiPushButton( this, "Properties", ioPixmap("settings"),
			  mCB(this,uiPSMultiPropDlg,selectPropCB), false );
    propbut->attach( rightTo, lblcb );
}


void selectPropCB( CallBacker* )
{
    ObjectSet<uiFlatViewer> vwrs;
    for ( int idx=0; idx<activevwridxs_.size(); idx++ )
	vwrs += vwrs_[activevwridxs_[idx]];
    uiFlatViewPropDlg propdlg( this, *vwrs[0], cb_ );
    if ( propdlg.go() )
    {
	 if ( propdlg.uiResult() == 1 )
	     cb_.doCall( this );
    }
}


void setGatherInfos( const TypeSet<GatherInfo>& ginfos )
{
    ginfos_ = ginfos;
    updateGatherNames();
    gatherChanged( 0 );
}

void updateGatherNames()
{
    BufferStringSet gathernms;
    getGatherNames( gathernms );
    datasetcb_->setEmpty();
    datasetcb_->addItems( gathernms );
}


void gatherChanged( CallBacker* )
{
    BufferString curgathernm =
	datasetcb_->textOfItem( datasetcb_->currentItem() );
    activevwridxs_.erase();
    int curvwridx = 0;
    for ( int gidx=0; gidx<ginfos_.size(); gidx++ )
    {
	const PreStackView::GatherInfo& ginfo = ginfos_[gidx];
	if ( !ginfo.isselected_ )
	    continue;
	if ( ginfo.gathernm_ == curgathernm )
	    activevwridxs_ += curvwridx;
	curvwridx++;
    }
}


void getGatherNames( BufferStringSet& gnms )
{
    gnms.erase();
    for ( int idx=0; idx<ginfos_.size(); idx++ )
	gnms.addIfNew( ginfos_[idx].gathernm_ );
}


const TypeSet<int> activeViewerIdx() const	{ return activevwridxs_; }
ObjectSet<uiFlatViewer>&	vwrs_;
TypeSet<GatherInfo>		ginfos_;
TypeSet<int>			activevwridxs_;
CallBack			cb_;
uiComboBox*			datasetcb_;
};


void uiViewer2DMainWin::setGatherView( uiGatherDisplay* gd,
				       uiGatherDisplayInfoHeader* gdi )
{
    const Interval<double> zrg( cs_.zrg.start, cs_.zrg.stop );
    gd->setPosition( gd->getBinID(), cs_.zrg.width()==0 ? 0 : &zrg );
    gd->updateViewRange();
    uiFlatViewer* fv = gd->getUiFlatViewer();
    gd->displayAnnotation( true );
    if ( vwrs_.size()>0 ) fv->appearance().annot_.x2_.showannot_ = false;

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
	ctrl->toolBar()->addButton(
		"snapshot", "Get snapshot",
		mCB(this,uiStoredViewer2DMainWin,snapshotCB) );
	ctrl->toolBar()->addButton(
		"preprocessing", "Pre processing",
		mCB(this,uiViewer2DMainWin,preprocessingCB) );
	control_ = ctrl;

	uiToolBar* tb = control_->toolBar();
	if ( tb )
	{
	    tb->addObject( 
		new uiToolButton( tb, "mute", "Load Mute",
		    mCB(this,uiViewer2DMainWin,loadMuteCB) ) );

	    if  ( isStored() )
	    {
		mDynamicCastGet(const uiStoredViewer2DMainWin*,storedwin,this);
		if ( storedwin && !storedwin->is2D() )
		{
		    tb->addObject( 
			new uiToolButton( tb,"anglegather","Display Angle Data",
			    mCB(this,uiViewer2DMainWin,angleDataCB) ) );
		}
		
		tb->addObject( 
		    new uiToolButton( tb, "contexthelp", "Help",
			mCB(this,uiStoredViewer2DMainWin,doHelp) ) );
	    }
	}
    }

    fv->appearance().ddpars_ = control_->dispPars();
    control_->addViewer( *fv );
}


void uiViewer2DMainWin::posDlgPushed( CallBacker* )
{
    if ( !posdlg_ )
    {
	BufferStringSet gathernms;
	getGatherNames( gathernms );
	cs_.zrg.step = (cs_.zrg.stop-cs_.zrg.start)/10.0f;
	posdlg_ = new uiViewer2DPosDlg( this, is2D(), cs_, gathernms,
					!isStored() );
	posdlg_->okpushed_.notify( mCB(this,uiViewer2DMainWin,posDlgChgCB) );
	posdlg_->setSelGatherInfos( gatherinfos_ );
    }

    posdlg_->raise();
    posdlg_->show();
}


bool uiViewer2DMainWin::isStored() const
{
    mDynamicCastGet(const uiStoredViewer2DMainWin*,storedwin,this);
    return storedwin;
}


void uiViewer2DMainWin::getStartupPositions( const BinID& bid,
	const StepInterval<int>& trcrg, bool isinl, TypeSet<BinID>& bids ) const
{
    bids.erase();
    int approxstep = trcrg.width()/sStartNrViewers;
    if ( !approxstep ) approxstep = 1;
    const int starttrcnr = isinl ? bid.crl : bid.inl;
    for ( int trcnr=starttrcnr; trcnr<=trcrg.stop; trcnr+=approxstep )
    {
	const int trcidx = trcrg.nearestIndex( trcnr );
	const int acttrcnr = trcrg.atIndex( trcidx );
	BinID posbid( isinl ? bid.inl : acttrcnr, isinl ? acttrcnr : bid.crl );
	bids.addIfNew( posbid );
	if ( bids.size() >= sStartNrViewers )
	    return;
    }

    for ( int trcnr=starttrcnr; trcnr>=trcrg.start; trcnr-=approxstep )
    {
	const int trcidx = trcrg.nearestIndex( trcnr );
	const int acttrcnr = trcrg.atIndex( trcidx );
	BinID posbid( isinl ? bid.inl : acttrcnr, isinl ? acttrcnr : bid.crl );
	if ( bids.isPresent(posbid) )
	    continue;
	bids.insert( 0, posbid );
	if ( bids.size() >= sStartNrViewers )
	    return;
    }
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
    cs_.zrg = SI().zRange(true);

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

    TypeSet<BinID> bids;
    getStartupPositions( bid, trcrg, isinl, bids );
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
    }

    setUpView();
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
	    gatherinfos_.addIfNew( ginfo );
	}
    }

    if ( posdlg_ ) posdlg_->setSelGatherInfos( gatherinfos_ );
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
	DataPack::ID ppgatherid = -1;
	if ( preprocmgr_ )
	    ppgatherid = getPreProcessedID( gatherinfo );

	gd->setVDGather( ppgatherid>=0 ? ppgatherid : gather->id() );
	if ( mIsUdf( zrg.start ) )
	   zrg = gd->getZDataRange();
	zrg.include( gd->getZDataRange() );
	DPM(DataPackMgr::FlatID()).release( gather );
    }
    else
    {
	gd->setVDGather( -1 );
	delete gather;
    }

    uiGatherDisplayInfoHeader* gdi = new uiGatherDisplayInfoHeader( 0 );
    setGatherInfo( gdi, gatherinfo );
    gdi->setOffsetRange( gd->getOffsetRange() );
    setGatherView( gd, gdi );

    gd_ += gd;
    gdi_ += gdi;
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
	    gatherinfos_.addIfNew( ginfo );
	}
    }

    if ( posdlg_ ) posdlg_->setSelGatherInfos( gatherinfos_ );
}


void uiSyntheticViewer2DMainWin::getGatherNames( BufferStringSet& nms) const
{
    nms.erase();
    for ( int idx=0; idx<gatherinfos_.size(); idx++ )
	nms.addIfNew( gatherinfos_[idx].gathernm_ );
}


uiSyntheticViewer2DMainWin::~uiSyntheticViewer2DMainWin()
{ removeGathers(); }


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


void uiSyntheticViewer2DMainWin::setGathers( const TypeSet<GatherInfo>& dps )
{
    setGathers( dps, true );
}


void uiSyntheticViewer2DMainWin::setGathers( const TypeSet<GatherInfo>& dps,
					     bool getstartups )
{
    gatherinfos_ = dps;
    StepInterval<int> trcrg( mUdf(int), -mUdf(int), 1 );
    cs_.hrg.setInlRange( StepInterval<int>(gatherinfos_[0].bid_.inl,
					   gatherinfos_[0].bid_.inl,1) );
    for ( int idx=0; idx<gatherinfos_.size(); idx++ )
	trcrg.include( gatherinfos_[idx].bid_.crl, false );
    trcrg.step = SI().crlStep();
    TypeSet<BinID> selbids;
    if ( getstartups )
    {
	getStartupPositions( gatherinfos_[0].bid_, trcrg, true, selbids );
	for ( int idx=0; idx<gatherinfos_.size(); idx++ )
	{
	    if ( selbids.isPresent(gatherinfos_[idx].bid_) )
		gatherinfos_[idx].isselected_ = true;
	    else
		gatherinfos_[idx].isselected_ = false;
	}
    }
    
    

    cs_.hrg.setCrlRange( trcrg );
    cs_.zrg.set( mUdf(float), -mUdf(float), SI().zStep() );
    setUpView();
    reSizeSld(0);
}

void uiSyntheticViewer2DMainWin::setGather( const GatherInfo& ginfo )
{
    if ( !ginfo.isselected_ ) return;

    uiGatherDisplay* gd = new uiGatherDisplay( 0 );
    DataPack* vddp = DPM(DataPackMgr::FlatID()).obtain( ginfo.vddpid_ );
    DataPack* wvadp = DPM(DataPackMgr::FlatID()).obtain( ginfo.wvadpid_ );

    mDynamicCastGet(PreStack::Gather*,vdgather,vddp);
    mDynamicCastGet(PreStack::Gather*,wvagather,wvadp);

    if ( !vdgather && !wvagather  )
    {
	gd->setVDGather( -1 );
	gd->setWVAGather( -1 );
	return;
    }

    cs_.zrg.include(wvagather ? wvagather->zRange()
	    		      : vdgather->zRange(),false);
    DataPack::ID ppgatherid = -1;
    if ( preprocmgr_ )
	ppgatherid = getPreProcessedID( ginfo );

    gd->setVDGather( ginfo.vddpid_<0 ? ppgatherid>=0 ? ppgatherid
	    					     : ginfo.wvadpid_
				     : ginfo.vddpid_ );
    if (  ginfo.vddpid_>=0 )
	gd->setWVAGather( ppgatherid>=0 ? ppgatherid : ginfo.wvadpid_ );
    uiGatherDisplayInfoHeader* gdi = new uiGatherDisplayInfoHeader( 0 );
    setGatherInfo( gdi, ginfo );
    gdi->setOffsetRange( gd->getOffsetRange() );
    setGatherView( gd, gdi );

    DPM(DataPackMgr::FlatID()).release( ginfo.vddpid_ );
    DPM(DataPackMgr::FlatID()).release( ginfo.wvadpid_ );

    gd_ += gd;
    gdi_ += gdi;
}


void uiSyntheticViewer2DMainWin::removeGathers()
{
    gatherinfos_.erase();
}



void uiSyntheticViewer2DMainWin::setGatherInfo(uiGatherDisplayInfoHeader* info,
					       const GatherInfo& ginfo )
{
    CubeSampling cs;
    const int modelnr = (ginfo.bid_.crl - cs.hrg.stop.crl)/cs.hrg.step.crl;
    info->setData( modelnr, ginfo.gathernm_ );
}



#define mDefBut(but,fnm,cbnm,tt) \
    uiToolButton* but = new uiToolButton( tb_, fnm, tt, mCB(this,uiViewer2DControl,cbnm) ); \
    tb_->addButton( but );

uiViewer2DControl::uiViewer2DControl( uiObjectItemView& mw, uiFlatViewer& vwr )
    : uiFlatViewStdControl(vwr,uiFlatViewStdControl::Setup(mw.parent())
			.withstates(false)
			.withthumbnail(false)
			.withcoltabed(true)
			.withsnapshot(false)
			.withflip(false)
			.withedit(false))
    , posdlgcalled_(this)
    , datadlgcalled_(this)
    , pspropdlg_(0)
{
    removeViewer( vwr );
    clearToolBar();

    objectitemctrl_ = new uiObjectItemViewControl( mw );
    tb_ = objectitemctrl_->toolBar();

    mDefBut(posbut,"orientation64",gatherPosCB,"Set positions");
    mDefBut(databut,"gatherdisplaysettings64",gatherDataCB, "Set gather data");
    mDefBut(parsbut,"2ddisppars",propertiesDlgCB,
	    "Set seismic display properties");
    ctabsel_ = new uiColorTableSel( tb_, "Select Color Table" );
    ctabsel_->selectionChanged.notify( mCB(this,uiViewer2DControl,coltabChg) );
    vwr_.dispParsChanged.notify( mCB(this,uiViewer2DControl,updateColTabCB) );
    ctabsel_->setCurrent( dispPars().vd_.ctab_ );
    tb_->addObject( ctabsel_ );
    tb_->addSeparator();
}


void uiViewer2DControl::propertiesDlgCB( CallBacker* )
{
    if ( !pspropdlg_ )
	pspropdlg_ = new uiPSMultiPropDlg( this, vwrs_,
			     mCB(this,uiViewer2DControl,applyProperties) );
    pspropdlg_->setGatherInfos( gatherinfos_ );
    pspropdlg_->go();
}


void uiViewer2DControl::updateColTabCB( CallBacker* )
{
    app_ = vwr_.appearance();
    ctabsel_->setCurrent( dispPars().vd_.ctab_.buf() );
}


void uiViewer2DControl::coltabChg( CallBacker* )
{
    dispPars().vd_.ctab_ = ctabsel_->getCurrent();
    for( int ivwr=0; ivwr<vwrs_.size(); ivwr++ )
    {
	if ( !vwrs_[ivwr] ) continue;
	uiFlatViewer& vwr = *vwrs_[ivwr];
	vwr.appearance().ddpars_ = app_.ddpars_;
	vwr.handleChange( FlatView::Viewer::DisplayPars );
    }
}


void uiViewer2DControl::applyProperties( CallBacker* )
{
    if ( !pspropdlg_ ) return;

    TypeSet<int> vwridxs = pspropdlg_->activeViewerIdx();
    const int actvwridx = vwridxs[0];
    if ( !vwrs_.validIdx(actvwridx) )
	return;

    app_ = vwrs_[ actvwridx ]->appearance();
    //const int selannot = pspropdlg_->selectedAnnot();

    const FlatDataPack* vddatapack = vwrs_[actvwridx]->pack( false );
    const FlatDataPack* wvadatapack = vwrs_[actvwridx]->pack( true );
    for( int ivwr=0; ivwr<vwridxs.size(); ivwr++ )
    {
	const int vwridx = vwridxs[ivwr];
	if ( !vwrs_[vwridx] ) continue;
	uiFlatViewer& vwr = *vwrs_[vwridx];
	vwr.appearance() = app_;
	if ( vwridx>0 ) vwr.appearance().annot_.x2_.showannot_ = false;

	const uiWorldRect cv( vwr.curView() );
	FlatView::Annotation& annot = vwr.appearance().annot_;
	if ( (cv.right() > cv.left()) == annot.x1_.reversed_ )
	    { annot.x1_.reversed_ = !annot.x1_.reversed_; flip( true ); }
	if ( (cv.top() > cv.bottom()) == annot.x2_.reversed_ )
	    { annot.x2_.reversed_ = !annot.x2_.reversed_; flip( false ); }

	for ( int idx=0; idx<vwr.availablePacks().size(); idx++ )
	{
	    const DataPack::ID& id = vwr.availablePacks()[idx];
	    FixedString datanm( DPM(DataPackMgr::FlatID()).nameOf(id) );
	    if ( vddatapack && datanm == vddatapack->name() &&
		 app_.ddpars_.vd_.show_ )
		vwr.usePack( false, id, false );
	    if ( wvadatapack && datanm == wvadatapack->name() &&
		 app_.ddpars_.wva_.show_ )
		vwr.usePack( true, id, false );
	}

	//vwr.setAnnotChoice( selannot );
	vwr.handleChange(
		FlatView::Viewer::DisplayPars | FlatView::Viewer::Annot );
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

uiViewer2DControl::~uiViewer2DControl()
{
    delete pspropdlg_;
}

DataPack::ID uiViewer2DMainWin::getPreProcessedID( const GatherInfo& ginfo )
{
    if ( !preprocmgr_->prepareWork() )
	return -1;

    const BinID stepout = preprocmgr_->getInputStepout();
    BinID relbid;
    for ( relbid.inl=-stepout.inl; relbid.inl<=stepout.inl; relbid.inl++ )
    {
	for ( relbid.crl=-stepout.crl; relbid.crl<=stepout.crl; relbid.crl++ )
	{
	    if ( !preprocmgr_->wantsInput(relbid) )
		continue;
	    BinID facbid( 1, 1 );
	    if ( isStored() && !is2D() )
		facbid = BinID( SI().inlStep(), SI().crlStep() );
	    const BinID bid = ginfo.bid_ + (relbid*facbid);
	    GatherInfo relposginfo;
	    relposginfo.isstored_ = ginfo.isstored_;
	    relposginfo.gathernm_ = ginfo.gathernm_;
	    relposginfo.bid_ = bid;
	    if ( isStored() )
		relposginfo.mid_ = ginfo.mid_;

	    setGatherforPreProc( relbid, relposginfo );
	}
    }

    if ( !preprocmgr_->process() )
    {
	uiMSG().error( preprocmgr_->errMsg() );
	return -1;
    }

    return preprocmgr_->getOutput();
}


void uiViewer2DMainWin::setGatherforPreProc( const BinID& relbid,
					     const GatherInfo& ginfo )
{
    PreStack::Gather* gather = new PreStack::Gather;
    if ( ginfo.isstored_ )
    {
	mDynamicCastGet(const uiStoredViewer2DMainWin*,storedpsmw,this);
	if ( !storedpsmw ) return;
	BufferString linename = storedpsmw->lineName();
	if ( (is2D() && gather->readFrom(ginfo.mid_,ginfo.bid_.crl,linename,0))
	     || (!is2D() && gather->readFrom(ginfo.mid_,ginfo.bid_)) )
	{
	    DPM( DataPackMgr::FlatID() ).addAndObtain( gather );
	    preprocmgr_->setInput( relbid, gather->id() );
	    DPM( DataPackMgr::FlatID() ).release( gather );
	}
    }
    else
    {
	const int gidx = gatherinfos_.indexOf( ginfo );
	if ( gidx < 0 )
	    return;
	const GatherInfo& inputginfo = gatherinfos_[gidx];
	preprocmgr_->setInput( relbid,
			       inputginfo.vddpid_>=0 ? inputginfo.wvadpid_
			       			     : inputginfo.vddpid_ );
    }
}

}; //namepsace
