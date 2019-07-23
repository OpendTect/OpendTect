/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/

#include "uipsviewer2dwin.h"

#include "uibutton.h"
#include "uicolseqsel.h"
#include "uiflatviewer.h"
#include "uiflatviewpropdlg.h"
#include "uiflatviewslicepos.h"
#include "uigraphicsitemimpl.h"
#include "uiioobjseldlg.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uiprestackanglemute.h"
#include "uiprestackprocessor.h"
#include "uiprogressbar.h"
#include "uipsviewer2d.h"
#include "uipsviewer2dinfo.h"
#include "uirgbarraycanvas.h"
#include "uisaveimagedlg.h"
#include "uislider.h"
#include "uistatusbar.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "coltabsequence.h"
#include "ctxtioobj.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "od_helpids.h"
#include "prestackanglemute.h"
#include "prestackanglecomputer.h"
#include "prestackgather.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"
#include "prestackprocessor.h"
#include "randcolor.h"
#include "settings.h"
#include "staticstring.h"
#include "survgeom.h"
#include "survinfo.h"
#include "windowfunction.h"


static int sStartNrViewers = 8;

namespace PreStackView
{

static const char* sKeySynthetic()	{ return "Synthteic"; }

static void setAnnotationPars( FlatView::Annotation& annot )
{
    annot.x1_.name_ = uiStrings::sOffset();
    annot.x1_.reversed_ = false;
    annot.x2_.name_ = SI().zIsTime() ? uiStrings::sTWT() : uiStrings::sDepth();
    annot.x2_.reversed_ = true;
    annot.x1_.showauxannot_ = false;
    annot.x2_.showauxannot_ = false;
    annot.x1_.annotinint_ = true;
    annot.x2_.annotinint_ = false;
    annot.x1_.showgridlines_ = true;
    annot.x2_.showgridlines_ = true;
    annot.color_ = Color::Black();
    annot.allowuserchangereversedaxis_ = false;
}


uiViewer2DWin::uiViewer2DWin( uiParent* p, const uiString& title )
    : uiObjectItemViewWin(p,uiObjectItemViewWin::Setup(title).startwidth(800))
    , posdlg_(0)
    , control_(0)
    , seldatacalled_(this)
    , axispainter_(0)
    , tkzs_(false)
    , preprocmgr_(0)
{
    setDeleteOnClose( true );
}


uiViewer2DWin::~uiViewer2DWin()
{
    deepErase( mutes_ );
    deepErase( gd_ );
    deepErase( gdi_ );
    delete posdlg_;
    delete preprocmgr_;
}


void uiViewer2DWin::snapshotCB( CallBacker* )
{
    uiSaveWinImageDlg snapshotdlg( this );
    snapshotdlg.go();
}


void uiViewer2DWin::dataDlgPushed( CallBacker* )
{
    seldatacalled_.trigger();
    if ( posdlg_ ) posdlg_->setSelGatherInfos( gatherinfos_ );
}


class uiPSPreProcessingDlg : public uiDialog
{ mODTextTranslationClass(uiPSPreProcessingDlg);
public:
uiPSPreProcessingDlg( uiParent* p, PreStack::ProcessManager& ppmgr,
		      const CallBack& cb )
    : uiDialog(p,uiDialog::Setup(uiStrings::sPreProcessing(),uiString::empty(),
                                 mODHelpKey(mPreStackProcSelHelpID) ) )
    , cb_(cb)
{
    preprocgrp_ = new PreStack::uiProcessorManager( this, ppmgr );
    uiButton* applybut = uiButton::getStd( this, OD::Apply,
			    mCB(this,uiPSPreProcessingDlg,applyCB), true );
    applybut->attach( alignedBelow, preprocgrp_ );
}


protected:

bool acceptOK()
{
    return apply();
}


void applyCB( CallBacker* )
{
    apply();
}


bool apply()
{
    if ( preprocgrp_->isChanged() )
    {
	 const int ret =
	     uiMSG().askSave(tr("Current settings are not saved.\n\n"
			        "Do you want to save them?"));
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



void uiViewer2DWin::preprocessingCB( CallBacker* )
{
    if ( !preprocmgr_ )
	preprocmgr_ = new PreStack::ProcessManager();
    uiPSPreProcessingDlg ppdlg( this, *preprocmgr_,
				mCB(this,uiViewer2DWin,applyPreProcCB) );
    ppdlg.go();
}


void uiViewer2DWin::applyPreProcCB( CallBacker* )
{ setUpView(); }


void uiViewer2DWin::setUpView()
{
    uiMainWin win( this, m3Dots(tr("Creating gather displays")) );
    uiProgressBar pb( &win );
    pb.setPrefWidthInChar( 50 );
    pb.setStretch( 2, 2 );
    pb.setTotalSteps( mCast(int,gatherinfos_.size()) );
    win.show();

    removeAllGathers();

    if ( preprocmgr_ && !preprocmgr_->reset() )
    {
	uiMSG().error( tr("Can not preprocess data") );
	return;
    }

    int nrvwr = 0;
    for ( int gidx=0; gidx<gatherinfos_.size(); gidx++ )
    {
	setGather( gatherinfos_[gidx] );
	pb.setProgress( nrvwr );
	nrvwr++;
    }

    GatherInfo dummyginfo;
    dummyginfo.isselected_ = true;
    setGather( dummyginfo );

    if ( control_ )
	control_->setGatherInfos( gatherinfos_ );

    displayMutes();
    reSizeSld(0);
}


void uiViewer2DWin::removeAllGathers()
{
    removeAllItems();
    if ( control_ )
	control_->removeAllViewers();
    vwrs_.erase();

    deepErase( gd_ );
    deepErase( gdi_ );
}

#define sWidthForX2Annot 70

void uiViewer2DWin::reSizeItems()
{
    const int nritems = mainviewer_->nrItems();
    if ( !nritems ) return;

    scaleVal( hslval_, true, true );
    scaleVal( vslval_, false, true );

    const int w =
	mNINT32( (hslval_*startwidth_- sWidthForX2Annot)/(float)nritems );
    const int h = mNINT32( vslval_*startheight_ );
    for ( int idx=0; idx<nritems-1; idx++ )
    {
	uiSize sz( idx ? w : w+sWidthForX2Annot, h);
	mainviewer_->reSizeItem( idx, sz );
    }

    uiSize dumyyitemsz( 20, h );
    mainviewer_->reSizeItem( nritems-1, dumyyitemsz );
    mainviewer_->getItem( nritems-1 )->setVisible( false );

    infobar_->reSizeItems();
    mainviewer_->resetViewArea(0);
    infobar_->resetViewArea(0);
}


void uiViewer2DWin::doHelp( CallBacker* )
{
    HelpProvider::provideHelp( HelpKey(mODHelpKey(mViewer2DPSMainWinHelpID) ) );
}


void uiViewer2DWin::displayMutes()
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
	    muteaux->linestyle_.type_ = OD::LineStyle::Solid;
	    muteaux->linestyle_.width_ = 3;

	    StepInterval<float> offsetrg = gd->getOffsetRange();
	    offsetrg.step = offsetrg.width()/20.0f;
	    const int sz = offsetrg.nrSteps()+1;
	    muteaux->poly_.setCapacity( sz, false );
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


void uiViewer2DWin::clearAuxData()
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


void uiViewer2DWin::loadMuteCB( CallBacker* cb )
{
    uiIOObjSelDlg::Setup sdsu( uiStrings::phrSelect(tr("Mute for display")) );
    sdsu.multisel( true );
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(MuteDef);
    uiIOObjSelDlg mutesel( this, sdsu, *ctio );
    if ( !mutesel.go() )
	return;

    clearAuxData();
    deepErase( mutes_ );
    mutecolors_.erase();
    const int nrsel = mutesel.nrChosen();
    for ( int idx=0; idx<nrsel; idx++ )
    {
	const DBKey& muteid = mutesel.chosenID( idx );
	PtrMan<IOObj> muteioobj = muteid.getIOObj();
	if ( !muteioobj )
	    continue;
	PreStack::MuteDef* mutedef = new PreStack::MuteDef;
	uiString errmsg;
	if ( !MuteDefTranslator::retrieve(*mutedef,muteioobj,errmsg) )
	    { uiMSG().error( errmsg ); continue; }

	mutes_ += mutedef;
	mutecolors_ += getRandStdDrawColor();
    }

    displayMutes();
    delete ctio->ioobj_;
}


RefMan<Gather> uiViewer2DWin::getAngleGather( const Gather& gather,
					      const Gather& angledata,
					      const Interval<int>& anglerange )
{
    const FlatPosData& fp = gather.posData();
    const StepInterval<double> x1rg( anglerange.start, anglerange.stop, 1 );
    const StepInterval<double> x2rg = fp.range( false );
    FlatPosData anglefp;
    anglefp.setRange( true, x1rg );
    anglefp.setRange( false, x2rg );

    RefMan<Gather> anglegather = new Gather ( anglefp );
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
	    const float angleval =
		Math::toDegrees( anglevals.get(ofsidx,zidx) );
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


void uiStoredViewer2DWin::convAngleDataToDegrees( Gather* angledata ) const
{
    if ( !angledata || !angledata->data().getData() )
	return;

    Array2D<float>& data = angledata->data();
    float* ptr = data.getData();
    const od_int64 size = data.totalSize();
    for( od_int64 idx=0; idx<size; idx++ )
	ptr[idx] = Math::toDegrees( ptr[idx] );
}


void uiViewer2DWin::displayInfo( CallBacker* cb )
{
    mCBCapsuleUnpack(IOPar,pars,cb);
    uiString mesg;
    makeInfoMsg( mesg, pars );
    statusBar()->message( mesg );
}

BufferString getSettingsKey( bool isstored )
{
    mDeclStaticString( key );
    if ( isstored )
	key = Gather::sDataPackCategory();
    else
	key = IOPar::compKey( Gather::sDataPackCategory(),
			      sKeySynthetic() ) ;
    return key;
}

class uiPSMultiPropDlg : public uiDialog
{ mODTextTranslationClass(uiPSMultiPropDlg);
public:
uiPSMultiPropDlg( uiParent* p, ObjectSet<uiFlatViewer>& vwrs,
		  const CallBack& cb, bool stored )
    : uiDialog(p,uiDialog::Setup(tr("Set properties for data"),
				 uiString::empty(),mNoHelpKey))
    , vwrs_(vwrs)
    , cb_(cb)
    , forstored_(stored)
{
    setCtrlStyle( uiDialog::CloseOnly );
    uiLabeledComboBox* lblcb =
	new uiLabeledComboBox( this, tr("Select gather") );
    datasetcb_ = lblcb->box();
    datasetcb_->selectionChanged.notify(
	    mCB(this,uiPSMultiPropDlg,gatherChanged) );
    uiPushButton* propbut =
	new uiPushButton( this, uiStrings::sProperties(),
			  uiPixmap("settings"),
			  mCB(this,uiPSMultiPropDlg,selectPropCB), false );
    propbut->attach( rightTo, lblcb );
}


void selectPropCB( CallBacker* )
{
    ObjectSet<uiFlatViewer> vwrs;
    for ( int idx=0; idx<activevwridxs_.size(); idx++ )
	vwrs += vwrs_[activevwridxs_[idx]];
    if ( vwrs.isEmpty() )
	return;

    vwrs[0]->appearance().annot_.allowuserchange_ = false;
    uiFlatViewPropDlg propdlg( this, *vwrs[0], cb_ );
    propdlg.setModal( true );
    if ( propdlg.go() )
    {
	 if ( propdlg.uiResult() == 1 )
	 {
	     if ( propdlg.saveButtonChecked() )
	     {
		 const BufferString key( getSettingsKey(forstored_) );
		 vwrs[0]->storeDefaults( key );
	     }

	     cb_.doCall( this );
	 }
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
    BufferString curgathernm = curGatherName();
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


BufferString curGatherName() const
{
    BufferString curgathernm =
	datasetcb_->itemText( datasetcb_->currentItem() );
    return curgathernm;
}

const TypeSet<int> activeViewerIdx() const	{ return activevwridxs_; }
ObjectSet<uiFlatViewer>&	vwrs_;
TypeSet<GatherInfo>		ginfos_;
TypeSet<int>			activevwridxs_;
CallBack			cb_;
uiComboBox*			datasetcb_;
bool				forstored_;
};


void uiViewer2DWin::setGatherView( uiGatherDisplay* gd,
				   uiGatherDisplayInfoHeader* gdi )
{
    const Interval<double> zrg( tkzs_.zsamp_.start, tkzs_.zsamp_.stop );
    gd->setPosition( gd->getBinID(), tkzs_.zsamp_.width()==0 ? 0 : &zrg );
    gd->updateViewRange();
    uiFlatViewer* fv = gd->getUiFlatViewer();

    vwrs_ += fv;
    addGroup( gd, gdi );

    if ( !control_ )
    {
	uiViewer2DControl* ctrl = new uiViewer2DControl( *mainviewer_, *fv,
							 isStored() );
	ctrl->posdlgcalled_.notify(
			    mCB(this,uiViewer2DWin,posDlgPushed));
	ctrl->datadlgcalled_.notify(
			    mCB(this,uiViewer2DWin,dataDlgPushed));
	ctrl->propChanged.notify(
			    mCB(this,uiViewer2DWin,propChangedCB));
	ctrl->infoChanged.notify( mCB(this,uiStoredViewer2DWin,displayInfo) );
	ctrl->toolBar()->addButton( "snapshot", uiStrings::sTakeSnapshot(),
		mCB(this,uiStoredViewer2DWin,snapshotCB) );
	ctrl->toolBar()->addButton( "preprocessing", tr("Pre processing"),
		mCB(this,uiViewer2DWin,preprocessingCB) );
	control_ = ctrl;

	uiToolBar* tb = control_->toolBar();
	if ( tb )
	{
	    tb->add( new uiToolButton( tb, "mute", tr("Load Mute"),
		     mCB(this,uiViewer2DWin,loadMuteCB) ) );

	    if  ( isStored() )
	    {
		mDynamicCastGet(const uiStoredViewer2DWin*,storedwin,this);
		if ( storedwin && !storedwin->is2D() )
		{
		    uiToolButtonSetup adtbsu(
			    "anglegather", tr("Display Angle Data"),
			    mCB(this,uiStoredViewer2DWin,angleDataCB) );
		    adtbsu.istoggle( true );
		    tb->add( new uiToolButton(tb,adtbsu) );
		}

		tb->add(
		    new uiToolButton( tb, "contexthelp", uiStrings::sHelp(),
			mCB(this,uiStoredViewer2DWin,doHelp) ) );
	    }
	}
    }

    PSViewAppearance dummypsapp;
    dummypsapp.datanm_ = gdi->getDataName();
    const int actappidx = appearances_.indexOf( dummypsapp );
    fv->appearance().ddpars_ =
	actappidx<0 ? control_->dispPars() : appearances_[actappidx].ddpars_;
    fv->appearance().annot_ =
	actappidx<0 ? control_->annot() : appearances_[actappidx].annot_;
    fv->appearance().annot_.x1_.showannot_ = true;
    fv->appearance().annot_.x2_.showannot_ = vwrs_.size()==1;
    setAnnotationPars( fv->appearance().annot_ );
    fv->handleChange( FlatView::Viewer::DisplayPars | FlatView::Viewer::Annot );
    control_->addViewer( *fv );
}


void uiViewer2DWin::propChangedCB( CallBacker* )
{
    PSViewAppearance curapp = control_->curViewerApp();
    const int appidx = appearances_.indexOf( curapp );
    if ( appidx>=0 )
	appearances_[appidx] = curapp;
}


void uiViewer2DWin::posDlgPushed( CallBacker* )
{
    if ( !posdlg_ )
    {
	BufferStringSet gathernms;
	getGatherNames( gathernms );
	posdlg_ = new uiViewer2DPosDlg( this, is2D(), tkzs_, gathernms,
					!isStored() );
	posdlg_->okpushed_.notify( mCB(this,uiViewer2DWin,posDlgChgCB) );
	posdlg_->windowClosed.notify( mCB(this,uiViewer2DWin,posDlgClosed));
	posdlg_->setSelGatherInfos( gatherinfos_ );
    }

    posdlg_->raise();
    posdlg_->show();
}


bool uiViewer2DWin::isStored() const
{
    mDynamicCastGet(const uiStoredViewer2DWin*,storedwin,this);
    return storedwin;
}


void uiViewer2DWin::getStartupPositions( const BinID& bid,
	const StepInterval<int>& trcrg, bool isinl, TypeSet<BinID>& bids )const
{
    bids.erase();
    int approxstep = trcrg.width()/sStartNrViewers;
    if ( !approxstep ) approxstep = 1;
    const int starttrcnr = isinl ? bid.crl() : bid.inl();
    for ( int trcnr=starttrcnr; trcnr<=trcrg.stop; trcnr+=approxstep )
    {
	const int trcidx = trcrg.nearestIndex( trcnr );
	const int acttrcnr = trcrg.atIndex( trcidx );
	BinID posbid( isinl ? bid.inl() : acttrcnr,
		      isinl ? acttrcnr : bid.crl() );
	bids.addIfNew( posbid );
	if ( bids.size() >= sStartNrViewers )
	    return;
    }

    for ( int trcnr=starttrcnr; trcnr>=trcrg.start; trcnr-=approxstep )
    {
	const int trcidx = trcrg.nearestIndex( trcnr );
	const int acttrcnr = trcrg.atIndex( trcidx );
	BinID posbid( isinl ? bid.inl() : acttrcnr,
		      isinl ? acttrcnr : bid.crl() );
	if ( bids.isPresent(posbid) )
	    continue;
	bids.insert( 0, posbid );
	if ( bids.size() >= sStartNrViewers )
	    return;
    }
}


void uiViewer2DWin::posDlgClosed( CallBacker* )
{
    if ( posdlg_ && posdlg_->uiResult() == 0 )
	posdlg_->setSelGatherInfos( gatherinfos_ );
}


void uiViewer2DWin::setAppearance( const FlatView::Appearance& app,
					int appidx )
{
    if ( !appearances_.validIdx(appidx) )
	return;
    PSViewAppearance& viewapp = appearances_[appidx];
    viewapp.annot_ = app.annot_;
    viewapp.ddpars_ = app.ddpars_;
    viewapp.annot_.allowuserchangereversedaxis_ = false;
    for ( int gidx=0; gidx<gd_.size(); gidx++ )
    {
	if ( viewapp.datanm_ != gdi_[gidx]->getDataName() )
	    continue;
	uiFlatViewer* vwr = gd_[gidx]->getUiFlatViewer();
	viewapp.annot_.x1_.showannot_ = true;
	viewapp.annot_.x2_.showannot_ = gidx==0;
	setAnnotationPars( viewapp.annot_ );
	vwr->appearance() = viewapp;
	vwr->handleChange( FlatView::Viewer::DisplayPars |
			   FlatView::Viewer::Annot );
    }
}


void uiViewer2DWin::prepareNewAppearances( BufferStringSet oldgathernms,
					       BufferStringSet newgathernms )
{
    while ( oldgathernms.size() )
    {
	BufferString gathertoberemoved = oldgathernms.get( 0 );
	const int newgatheridx = newgathernms.indexOf( gathertoberemoved );
	if ( newgatheridx>=0 )
	    newgathernms.removeSingle( newgatheridx );
	oldgathernms.removeSingle( 0 );
    }

    while ( newgathernms.size() )
    {
	PSViewAppearance psapp;
	psapp.datanm_ = newgathernms.get( 0 );
	newgathernms.removeSingle( 0 );
	if ( appearances_.isEmpty() && !getStoredAppearance(psapp) )
	{
	    if ( !isStored() )
	    {
		RefMan<ColTab::MapperSetup> newmapsu
				= psapp.ddpars_.vd_.mapper_->setup().clone();
		newmapsu->setFixedRange( Interval<float>(0.f,60.f) );
		psapp.ddpars_.vd_.mapper_->setup() = *newmapsu;
		psapp.ddpars_.vd_.colseqname_
				= ColTab::Sequence::sDefaultName();

		*newmapsu = psapp.ddpars_.wva_.mapper_->setup();
		newmapsu->setNoClipping();
		psapp.ddpars_.wva_.mapper_->setup() = *newmapsu;
	    }
	}
	else if ( !appearances_.isEmpty() )
	{
	    psapp.annot_ = appearances_[0].annot_;
	    psapp.ddpars_ = appearances_[0].ddpars_;
	}

	psapp.annot_.x1_.showannot_ = true;
	psapp.annot_.x2_.showannot_ = true;
	setAnnotationPars( psapp.annot_ );
	appearances_ +=psapp;
    }
}


DataPack::ID uiViewer2DWin::getPreProcessedID( const GatherInfo& ginfo )
{
    if ( !preprocmgr_->prepareWork() )
	return DataPack::ID::getInvalid();

    const BinID stepout = preprocmgr_->getInputStepout();
    BinID relbid;
    for ( relbid.inl()=-stepout.inl(); relbid.inl()<=stepout.inl();
		relbid.inl()++ )
    {
	for ( relbid.crl()=-stepout.crl(); relbid.crl()<=stepout.crl();
		relbid.crl()++ )
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
	return DataPack::ID::getInvalid();
    }

    return preprocmgr_->getOutput();
}


void uiViewer2DWin::setGatherforPreProc( const BinID& relbid,
					     const GatherInfo& ginfo )
{
    if ( ginfo.isstored_ )
    {
	RefMan<Gather> gather = new Gather;
	DPM(DataPackMgr::FlatID()).add( gather );
	mDynamicCastGet(const uiStoredViewer2DWin*,storedpsmw,this);
	if ( !storedpsmw )
	    return;
	BufferString linename = storedpsmw->lineName();
	TrcKey tk( ginfo.bid_ );
	if ( is2D() )
	    tk.setGeomID( SurvGeom::getGeomID(linename) );

	if ( gather->readFrom(ginfo.mid_,tk) )
	    preprocmgr_->setInput( relbid, gather->id() );
    }
    else
    {
	const int gidx = gatherinfos_.indexOf( ginfo );
	if ( gidx < 0 )
	    return;
	const GatherInfo& inputginfo = gatherinfos_[gidx];
	preprocmgr_->setInput( relbid,
			       inputginfo.vddpid_.isValid()
			       ? inputginfo.wvadpid_ : inputginfo.vddpid_ );
    }
}


bool uiViewer2DWin::getStoredAppearance( PSViewAppearance& psapp ) const
{
    Settings& setts = Settings::fetch( "flatview" );
    const BufferString key( getSettingsKey(isStored()) );
    PtrMan<IOPar> iop = setts.subselect( key );
    if ( !iop && isStored() ) // for older stored par files
	iop = setts.subselect( Gather::sDataPackCategory() );

    if ( !iop )
	return false;

    psapp.usePar( *iop );
    return true;
}


uiStoredViewer2DWin::uiStoredViewer2DWin( uiParent* p, const uiString& title,
					  bool is2d )
    : uiViewer2DWin(p,title)
    , linename_(0)
    , angleparams_(0)
    , doanglegather_(false)
    , is2d_(is2d)
    , slicepos_(0)
{
    hasangledata_ = false;
    if ( !is2d_ )
    {
	slicepos_ = new uiSlicePos2DView( this, ZDomain::Info(SI().zDomain()) );
	slicepos_->positionChg.notify(
			    mCB(this,uiStoredViewer2DWin,posSlcChgCB));
    }
}


void uiStoredViewer2DWin::getGatherNames( BufferStringSet& nms) const
{
    nms.erase();
    for ( int idx=0; idx<mids_.size(); idx++ )
    {
	PtrMan<IOObj> gatherioobj = mids_[idx].getIOObj();
	if ( !gatherioobj )
	    continue;
	nms.add( gatherioobj->name() );
    }
}


void uiStoredViewer2DWin::init( const DBKey& mid, const BinID& bid,
	bool isinl, const StepInterval<int>& trcrg, const char* linename )
{
    mids_ += mid;
    linename_ = linename;
    tkzs_.zsamp_ = SI().zRange( OD::UsrWork );

    if ( is2d_ )
    {
	tkzs_.hsamp_.setInlRange( Interval<int>( 1, 1 ) );
	tkzs_.hsamp_.setCrlRange( trcrg );
    }
    else
    {
	if ( isinl )
	{
	    tkzs_.hsamp_.setInlRange( Interval<int>( bid.inl(), bid.inl() ) );
	    tkzs_.hsamp_.setCrlRange( trcrg );
	}
	else
	{
	    tkzs_.hsamp_.setCrlRange( Interval<int>( bid.crl(), bid.crl() ) );
	    tkzs_.hsamp_.setInlRange( trcrg );
	}

	slicepos_->setTrcKeyZSampling( tkzs_ );
    }

    setUpNewPositions( isinl, bid, trcrg );
    setUpNewIDs();
}


void uiStoredViewer2DWin::setUpNewSlicePositions()
{
    const bool isinl = is2d_ || tkzs_.defaultDir()==OD::InlineSlice;
    const int newpos = isinl
	? tkzs_.hsamp_.start_.inl()
	: tkzs_.hsamp_.start_.crl();

    for ( int idx=0; idx<gatherinfos_.size(); idx++ )
    {
	int& pos = isinl ? gatherinfos_[idx].bid_.inl()
			 : gatherinfos_[idx].bid_.crl();
	pos = newpos;
    }
}


void uiStoredViewer2DWin::setUpNewPositions(bool isinl, const BinID& posbid,
						const StepInterval<int>& trcrg )
{
    TypeSet<BinID> bids;
    getStartupPositions( posbid, trcrg, isinl, bids );
    gatherinfos_.erase();
    for ( int idx=0; idx<bids.size(); idx++ )
    {
	GatherInfo ginfo;
	ginfo.isselected_ = true;
	ginfo.bid_ = bids[idx];
	ginfo.isselected_ = true;
	gatherinfos_ += ginfo;
    }
}


void uiStoredViewer2DWin::setIDs( const DBKeySet& mids  )
{
    mids_ = mids;
    setUpNewIDs();
}


void uiStoredViewer2DWin::setUpNewIDs()
{
    TypeSet<PSViewAppearance> oldapps = appearances_;
    appearances_.erase();
    TypeSet<GatherInfo> oldginfos = gatherinfos_;
    gatherinfos_.erase();

    BufferStringSet newgathernms, oldgathernms;
    BufferString firstgathernm;
    if ( !oldginfos.isEmpty() )
	firstgathernm = oldginfos[0].gathernm_;
    for ( int gidx=0; gidx<oldginfos.size(); gidx++ )
    {
	GatherInfo ginfo = oldginfos[gidx];
	if ( !firstgathernm.isEmpty() && firstgathernm != ginfo.gathernm_ )
	    continue;
	for ( int midx=0; midx<mids_.size(); midx++ )
	{
	    PtrMan<IOObj> gatherioobj = mids_[midx].getIOObj();
	    if ( !gatherioobj )
		continue;
	    ginfo.gathernm_ = gatherioobj->name();
	    ginfo.mid_ = mids_[midx];
	    newgathernms.addIfNew( ginfo.gathernm_ );
	    if ( gatherinfos_.addIfNew(ginfo) )
	    {
		PSViewAppearance dummypsapp;
		dummypsapp.datanm_ = gatherioobj->name();
		const int oldappidx = oldapps.indexOf( dummypsapp );
		if ( oldappidx>=0 )
		{
		    oldgathernms.addIfNew( ginfo.gathernm_ );
		    appearances_.addIfNew( oldapps[oldappidx] );
		}
	    }
	}
    }

    prepareNewAppearances( oldgathernms, newgathernms );
    if ( posdlg_ ) posdlg_->setSelGatherInfos( gatherinfos_ );
    setUpView();
}


void uiStoredViewer2DWin::setGatherInfo( uiGatherDisplayInfoHeader* info,
					     const GatherInfo& ginfo )
{
    PtrMan<IOObj> ioobj = ginfo.mid_.getIOObj();
    BufferString nm = ioobj ? ioobj->name().buf() : "";
    info->setData( ginfo.bid_, tkzs_.defaultDir()==OD::InlineSlice,
		   is2d_, nm);
}


void uiStoredViewer2DWin::posDlgChgCB( CallBacker* )
{
    if ( posdlg_ )
    {
	posdlg_->getTrcKeyZSampling( tkzs_ );
	posdlg_->getSelGatherInfos( gatherinfos_ );
	BufferStringSet gathernms;

	for ( int idx=0; idx<gatherinfos_.size(); idx++ )
	{
	    GatherInfo& ginfo = gatherinfos_[idx];
	    for ( int midx=0; midx<mids_.size(); midx++ )
	    {
		PtrMan<IOObj> gatherioobj = mids_[midx].getIOObj();
		if ( !gatherioobj )
		    continue;
		if ( ginfo.gathernm_ == gatherioobj->name() )
		    ginfo.mid_ = mids_[midx];
	    }
	}
    }

    if ( slicepos_ )
	slicepos_->setTrcKeyZSampling( tkzs_ );

    setUpView();
}


void uiStoredViewer2DWin::posSlcChgCB( CallBacker* )
{
    if ( slicepos_ )
	tkzs_ = slicepos_->getTrcKeyZSampling();
    if ( posdlg_ )
	posdlg_->setTrcKeyZSampling( tkzs_ );

    setUpNewSlicePositions();
    setUpNewIDs();
}



RefMan<Gather> uiStoredViewer2DWin::getAngleData( DataPack::ID gatherid )
{
    if ( !hasangledata_ || !angleparams_ )
	return 0;
    auto gather = DPM( DataPackMgr::FlatID() ).get<Gather>( gatherid );
    if ( !gather )
	return 0;

    RefMan<PreStack::VelocityBasedAngleComputer> velangcomp =
			    new PreStack::VelocityBasedAngleComputer;
    velangcomp->setDBKey( angleparams_->velvolmid_ );
    velangcomp->setRayTracerPars( angleparams_->raypar_ );
    velangcomp->setSmoothingPars( angleparams_->smoothingpar_ );
    const FlatPosData& fp = gather->posData();
    velangcomp->setOutputSampling( fp );
    velangcomp->gatherIsNMOCorrected( gather->isCorrected() );
    velangcomp->setTrcKey( TrcKey(gather->getBinID()) );
    RefMan<Gather> angledata = velangcomp->computeAngles();
    if ( !angledata ) return 0;
    BufferString angledpnm( gather->name(), " Incidence Angle" );
    angledata->setName( angledpnm );
    convAngleDataToDegrees( angledata );
    DPM( DataPackMgr::FlatID() ).add( angledata );
    if ( doanglegather_ )
    {
	RefMan<Gather> anglegather =
	     getAngleGather( *gather, *angledata, angleparams_->anglerange_ );
	DPM( DataPackMgr::FlatID() ).add( anglegather );
	return anglegather;
    }

    return angledata;
}


class uiAngleCompParDlg : public uiDialog
{ mODTextTranslationClass(uiAngleCompParDlg)
public:

uiAngleCompParDlg( uiParent* p, PreStack::AngleCompParams& acp, bool isag )
    : uiDialog(p,uiDialog::Setup(uiString::empty(),uiString::empty(),
			      mODHelpKey(mViewer2DPSMainWindisplayAngleHelpID)))
{
    uiString windowtitle = isag ? tr("Angle Gather Display")
				: tr("Angle Data Display");

    setTitleText( windowtitle );
    uiString windowcaption = uiStrings::phrSpecify(tr("Parameters for %1")
							     .arg(windowtitle));
    setCaption( windowcaption );
    anglegrp_ = new PreStack::uiAngleCompGrp( this, acp, false, false );
}

protected:

bool acceptOK()
{
    return anglegrp_->acceptOK();
}

PreStack::uiAngleCompGrp*	anglegrp_;
};


bool uiStoredViewer2DWin::getAngleParams()
{
    if ( !angleparams_ )
    {
	angleparams_ = new PreStack::AngleCompParams;
	if ( !doanglegather_ )
	    angleparams_->anglerange_ = Interval<int>( 0 , 60 );
    }

    uiAngleCompParDlg agcompdlg( this, *angleparams_, doanglegather_ );
    return agcompdlg.go();
}


void uiStoredViewer2DWin::angleGatherCB( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb);
    if ( !tb ) return;
    hasangledata_ = tb->isOn();
    doanglegather_ = true;
    if ( tb->isOn() )
    {
	if ( !getAngleParams() )
	{
	    tb->setOn( false );
	    hasangledata_ = false;
	    doanglegather_ = false;
	    return;
	}
    }

    displayAngle();
}


void uiStoredViewer2DWin::angleDataCB( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb);
    if ( !tb ) return;
    hasangledata_ = tb->isOn();
    doanglegather_ = false;
    if ( tb->isOn() )
    {
	if ( !getAngleParams() )
	{
	    tb->setOn( false );
	    hasangledata_ = false;
	    doanglegather_ = false;
	    return;
	}
    }

    displayAngle();
}


void uiStoredViewer2DWin::displayAngle()
{
    for ( int dataidx=0; dataidx<appearances_.size(); dataidx++ )
    {
	if ( doanglegather_ ) continue;
	PSViewAppearance& psapp = appearances_[dataidx];
	RefMan<ColTab::MapperSetup> newmapsu
		    = psapp.ddpars_.vd_.mapper_->setup().clone();
	if ( hasangledata_ )
	{
	    psapp.ddpars_.vd_.show_ = true;
	    psapp.ddpars_.wva_.show_ = true;
	    newmapsu->setFixedRange( Interval<float>(
			(float)angleparams_->anglerange_.start,
			(float)angleparams_->anglerange_.stop ) );
	    newmapsu->setSeqUseMode( ColTab::UnflippedCyclic );
	    psapp.ddpars_.vd_.colseqname_ = ColTab::Sequence::sDefaultName();
	}
	else
	{
	    psapp.ddpars_.vd_.show_ = true;
	    psapp.ddpars_.wva_.show_ = false;
	    psapp.ddpars_.vd_.colseqname_ = "Seismics";
	    newmapsu->setNotFixed();
	    newmapsu->setClipRate( ColTab::ClipRatePair(0.025f,0.025f) );
	}
	psapp.ddpars_.vd_.mapper_->setup() = *newmapsu;
    }

    setUpView();
}


void uiStoredViewer2DWin::setGather( const GatherInfo& gatherinfo )
{
    if ( !gatherinfo.isselected_ ) return;

    Interval<float> zrg( mUdf(float), 0 );
    uiGatherDisplay* gd = new uiGatherDisplay( 0 );
    RefMan<Gather> gather = new Gather;
    DPM(DataPackMgr::FlatID()).add( gather );
    TrcKey tk( gatherinfo.bid_ );
    if ( is2D() )
	tk.setGeomID( Survey::Geometry::getGeomID(linename_) );

    if ( gather->readFrom(gatherinfo.mid_,tk) )
    {
	DataPack::ID ppgatherid;
	if ( preprocmgr_ && preprocmgr_->nrProcessors() )
	    ppgatherid = getPreProcessedID( gatherinfo );

	const DataPack::ID gatherid = ppgatherid.isValid()
					? ppgatherid : gather->id();
	RefMan<Gather> anglegather = getAngleData( gatherid );
	gd->setVDGather( hasangledata_ ? anglegather->id() : gatherid );
	gd->setWVAGather( hasangledata_ ? gatherid
					: DataPack::ID::getInvalid() );
	if ( mIsUdf( zrg.start ) )
	   zrg = gd->getZDataRange();
	zrg.include( gd->getZDataRange() );
    }
    else
	gd->setVDGather( DataPack::ID::getInvalid() );

    uiGatherDisplayInfoHeader* gdi = new uiGatherDisplayInfoHeader( 0 );
    setGatherInfo( gdi, gatherinfo );
    gdi->setOffsetRange( gd->getOffsetRange() );
    setGatherView( gd, gdi );

    gd_ += gd;
    gdi_ += gdi;
}


uiSyntViewer2DWin::uiSyntViewer2DWin( uiParent* p, const uiString& title )
    : uiViewer2DWin(p,title)
{
    hasangledata_ = true;
}


void uiSyntViewer2DWin::setGatherNames( const BufferStringSet& nms )
{
    TypeSet<PSViewAppearance> oldapps = appearances_;
    appearances_.erase();
    TypeSet<GatherInfo> oldginfos = gatherinfos_;
    gatherinfos_.erase();

    BufferStringSet newgathernms, oldgathernms;
    for ( int gidx=0; gidx<oldginfos.size(); gidx++ )
    {
	for ( int nmidx=0; nmidx<nms.size(); nmidx++ )
	{
	    GatherInfo ginfo = oldginfos[gidx];
	    ginfo.gathernm_ = nms.get( nmidx );
	    newgathernms.addIfNew( ginfo.gathernm_ );
	    if ( gatherinfos_.addIfNew(ginfo) )
	    {
		PSViewAppearance dummypsapp;
		dummypsapp.datanm_ = ginfo.gathernm_;
		const int oldappidx = oldapps.indexOf( dummypsapp );
		if ( oldappidx>=0 )
		{
		    oldgathernms.addIfNew( ginfo.gathernm_ );
		    appearances_.addIfNew( oldapps[oldappidx] );
		}
	    }
	}
    }

    prepareNewAppearances( oldgathernms, newgathernms );
    if ( posdlg_ ) posdlg_->setSelGatherInfos( gatherinfos_ );
    setUpView();
}



void uiSyntViewer2DWin::getGatherNames( BufferStringSet& nms ) const
{
    nms.erase();
    for ( int idx=0; idx<gatherinfos_.size(); idx++ )
	nms.addIfNew( gatherinfos_[idx].gathernm_ );
}


uiSyntViewer2DWin::~uiSyntViewer2DWin()
{
    removeGathers();
}


void uiSyntViewer2DWin::posDlgChgCB( CallBacker* )
{
    if ( posdlg_ )
    {
	TypeSet<GatherInfo> gatherinfos;
	posdlg_->getTrcKeyZSampling( tkzs_ );
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

    setUpView();
}


void uiSyntViewer2DWin::setGathers( const TypeSet<GatherInfo>& dps )
{
    setGathers( dps, true );
}


void uiSyntViewer2DWin::setGathers( const TypeSet<GatherInfo>& dps,
				    bool getstartups )
{
    TypeSet<PSViewAppearance> oldapps = appearances_;
    appearances_.erase();
    BufferStringSet oldgathernms;
    for ( int idx=0; idx<gatherinfos_.size(); idx++ )
	oldgathernms.addIfNew( gatherinfos_[idx].gathernm_ );
    gatherinfos_ = dps;
    StepInterval<int> trcrg( mUdf(int), -mUdf(int), 1 );
    tkzs_.hsamp_.setInlRange( StepInterval<int>(gatherinfos_[0].bid_.inl(),
					   gatherinfos_[0].bid_.inl(),1) );
    BufferStringSet newgathernms;
    for ( int idx=0; idx<gatherinfos_.size(); idx++ )
    {
	PreStackView::GatherInfo ginfo = gatherinfos_[idx];
	trcrg.include( ginfo.bid_.crl(), false );
	PSViewAppearance dummypsapp;
	dummypsapp.datanm_ = ginfo.gathernm_;
	newgathernms.addIfNew( ginfo.gathernm_ );
	if ( oldapps.isPresent(dummypsapp) &&
	     !appearances_.isPresent(dummypsapp) )
	    oldgathernms.addIfNew( ginfo.gathernm_ );
    }

    prepareNewAppearances( oldgathernms, newgathernms );
    trcrg.step = SI().crlStep();
    TypeSet<BinID> selbids;
    if ( getstartups )
    {
	getStartupPositions( gatherinfos_[0].bid_, trcrg, true, selbids );
	for ( int idx=0; idx<gatherinfos_.size(); idx++ )
	{
	    gatherinfos_[idx].isselected_ =
		selbids.isPresent( gatherinfos_[idx].bid_ );
	}
    }

    tkzs_.hsamp_.setCrlRange( trcrg );
    if ( !posdlg_ )
	tkzs_.zsamp_.set( mUdf(float), -mUdf(float), SI().zStep() );
    setUpView();
    reSizeSld(0);
}


void uiSyntViewer2DWin::setGather( const GatherInfo& ginfo )
{
    if ( !ginfo.isselected_ ) return;

    uiGatherDisplay* gd = new uiGatherDisplay( 0 );
    const DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    auto vdgather = dpm.get<Gather>( ginfo.vddpid_ );
    auto wvagather = dpm.get<Gather>( ginfo.wvadpid_ );

    if ( !vdgather && !wvagather  )
    {
	gd->setVDGather( DataPack::ID::getInvalid() );
	gd->setWVAGather( DataPack::ID::getInvalid() );
	return;
    }

    if ( !posdlg_ )
	tkzs_.zsamp_.include( wvagather ? wvagather->zRange()
				        : vdgather->zRange(), false );
    DataPack::ID ppgatherid;
    if ( preprocmgr_ && preprocmgr_->nrProcessors() )
	ppgatherid = getPreProcessedID( ginfo );

    gd->setVDGather( ginfo.vddpid_.isInvalid() ?
	(ppgatherid.isValid() ? ppgatherid : ginfo.wvadpid_) : ginfo.vddpid_ );
    gd->setWVAGather( ginfo.vddpid_.isValid() ?
		(ppgatherid.isValid() ? ppgatherid : ginfo.wvadpid_)
				       : DataPack::ID::getInvalid() );
    uiGatherDisplayInfoHeader* gdi = new uiGatherDisplayInfoHeader( 0 );
    setGatherInfo( gdi, ginfo );
    gdi->setOffsetRange( gd->getOffsetRange() );
    setGatherView( gd, gdi );

    gd_ += gd;
    gdi_ += gdi;
}


void uiSyntViewer2DWin::removeGathers()
{
    gatherinfos_.erase();
}



void uiSyntViewer2DWin::setGatherInfo( uiGatherDisplayInfoHeader* info,
					const GatherInfo& ginfo )
{
    const int modelnr = ginfo.bid_.crl();
    info->setData( modelnr, ginfo.gathernm_ );
}



#define mDefBut(but,fnm,cbnm,tt) \
    uiToolButton* but = \
	new uiToolButton( tb_, fnm, tt, mCB(this,uiViewer2DControl,cbnm) ); \
    tb_->add( but );

uiViewer2DControl::uiViewer2DControl( uiObjectItemView& mw, uiFlatViewer& vwr,
				      bool isstored )
    : uiFlatViewStdControl(vwr,uiFlatViewStdControl::Setup(mw.parent())
			.withcoltabed(true)
			.withsnapshot(false)
			.withflip(false)
			.withrubber(false)
			.withedit(false))
    , posdlgcalled_(this)
    , datadlgcalled_(this)
    , propChanged(this)
    , pspropdlg_(0)
    , isstored_(isstored)
{
    removeViewer( vwr );
    clearToolBar();

    objectitemctrl_ = new uiObjectItemViewControl( mw );
    tb_ = objectitemctrl_->toolBar();

    mDefBut(posbut,"orientation64",gatherPosCB,tr("Set positions"));
    mDefBut(databut,"gatherdisplaysettings64",gatherDataCB,
	    tr("Set gather data"));
    mDefBut(parsbut,"2ddisppars",propertiesDlgCB,
	    tr("Set seismic display properties"));
    colseqsel_ = new uiColSeqSel( tb_, OD::Horizontal,
				    uiStrings::sColorTable() );
    colseqsel_->seqChanged.notify( mCB(this,uiViewer2DControl,coltabChg) );
    vwr_.dispParsChanged.notify( mCB(this,uiViewer2DControl,updateColTabCB) );
    colseqsel_->setSeqName( dispPars().vd_.colseqname_ );
    colseqsel_->addObjectsToToolBar( *tb_ );
    tb_->addSeparator();
}


void uiViewer2DControl::propertiesDlgCB( CallBacker* )
{
    if ( !pspropdlg_ )
	pspropdlg_ = new uiPSMultiPropDlg(
	    this, vwrs_, mCB(this,uiViewer2DControl,applyProperties),isstored_);
    pspropdlg_->setGatherInfos( gatherinfos_ );
    pspropdlg_->go();
}


void uiViewer2DControl::updateColTabCB( CallBacker* )
{
    app_ = vwr_.appearance();
    colseqsel_->setSeqName( dispPars().vd_.colseqname_ );
}


void uiViewer2DControl::coltabChg( CallBacker* )
{
    dispPars().vd_.colseqname_ = colseqsel_->seqName();
    for( int ivwr=0; ivwr<vwrs_.size(); ivwr++ )
    {
	if ( !vwrs_[ivwr] ) continue;
	uiFlatViewer& vwr = *vwrs_[ivwr];
	vwr.appearance().ddpars_ = app_.ddpars_;
	vwr.handleChange( FlatView::Viewer::DisplayPars );
    }
}


PSViewAppearance uiViewer2DControl::curViewerApp()
{
    PSViewAppearance psviewapp;
    psviewapp.annot_ = app_.annot_;
    psviewapp.ddpars_ = app_.ddpars_;
    psviewapp.datanm_ = pspropdlg_->curGatherName();
    return psviewapp;
};


void uiViewer2DControl::applyProperties( CallBacker* )
{
    if ( !pspropdlg_ ) return;

    TypeSet<int> vwridxs = pspropdlg_->activeViewerIdx();
    const int actvwridx = vwridxs[0];
    if ( !vwrs_.validIdx(actvwridx) )
	return;

    app_ = vwrs_[ actvwridx ]->appearance();
    propChanged.trigger();
    colseqsel_->setSeqName( app_.ddpars_.vd_.colseqname_ );

    ConstRefMan<FlatDataPack> vddatapack = vwrs_[actvwridx]->getPack( false );
    ConstRefMan<FlatDataPack> wvadatapack = vwrs_[actvwridx]->getPack( true );
    for( int ivwr=0; ivwr<vwridxs.size(); ivwr++ )
    {
	const int vwridx = vwridxs[ivwr];
	if ( !vwrs_[vwridx] ) continue;
	uiFlatViewer& vwr = *vwrs_[vwridx];
	vwr.appearance() = app_;
	vwr.appearance().annot_.x1_.showannot_ = true;
	vwr.appearance().annot_.x2_.showannot_ = vwridx==0;
	setAnnotationPars( vwr.appearance().annot_ );

	const uiWorldRect cv( vwr.curView() );
	FlatView::Annotation& annotations = vwr.appearance().annot_;
	if ( (cv.right() > cv.left()) == annotations.x1_.reversed_ )
	{
	    annotations.x1_.reversed_ = !annotations.x1_.reversed_;
	    flip( true );
	}
	if ( (cv.top() > cv.bottom()) == annotations.x2_.reversed_ )
	{
	    annotations.x2_.reversed_ = !annotations.x2_.reversed_;
	    flip( false );
	}

	for ( int idx=0; idx<vwr.availablePacks().size(); idx++ )
	{
	    const DataPack::ID& id = vwr.availablePacks()[idx];
	    FixedString datanm( DPM(DataPackMgr::FlatID()).nameOf(id) );
	    if ( vddatapack && vddatapack->hasName(datanm) &&
		 app_.ddpars_.vd_.show_ )
		vwr.usePack( false, id, false );
	    if ( wvadatapack && wvadatapack->hasName(datanm) &&
		 app_.ddpars_.wva_.show_ )
		vwr.usePack( true, id, false );
	}

	vwr.handleChange( FlatView::Viewer::DisplayPars |
			  FlatView::Viewer::Annot );
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


void uiViewer2DControl::doPropertiesDialog( int vieweridx )
{
    int ivwr = 0;
    for ( ivwr=0; ivwr<vwrs_.size(); ivwr++ )
    {
	if ( vwrs_[ivwr]->hasPack(true) || vwrs_[ivwr]->hasPack(false) )
	    break;
    }
    return uiFlatViewControl::doPropertiesDialog( ivwr );
}


void uiViewer2DControl::setGatherInfos( const TypeSet<GatherInfo>& gis )
{
    gatherinfos_ = gis;
    BufferStringSet gathernms;
    for ( int idx=0; idx<gatherinfos_.size(); idx++ )
	gathernms.addIfNew( gatherinfos_[idx].gathernm_ );
    const bool showcoltab = gathernms.size() <= 1;
    colseqsel_->setSensitive( showcoltab );
    uiString tooltipstr;
    if ( !showcoltab )
	tooltipstr = tr("Mutiple datasets selected. "
			 "Use 'Set seismic display properties' button");
    else
	tooltipstr = uiString::empty();
    colseqsel_->asUiObject().setToolTip( tooltipstr );
}


uiViewer2DControl::~uiViewer2DControl()
{
    delete pspropdlg_;
}


}; //namespace
