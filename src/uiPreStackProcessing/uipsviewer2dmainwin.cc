/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipsviewer2dmainwin.h"

#include "uibutton.h"
#include "uicolortable.h"
#include "uiflatviewer.h"
#include "uiflatviewpropdlg.h"
#include "uiflatviewslicepos.h"
#include "uigraphicsitemimpl.h"
#include "uiioobjseldlg.h"
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

#include "ailayer.h"
#include "arrayndalgo.h"
#include "coltabsequence.h"
#include "ctxtioobj.h"
#include "flatposdata.h"
#include "ioman.h"
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
#include "survinfo.h"
#include "unitofmeasure.h"
#include "windowfunction.h"


static int sStartNrViewers = 8;

namespace PreStackView
{

PSViewAppearance::PSViewAppearance()
{}


PSViewAppearance::~PSViewAppearance()
{}


static void setAnnotationPars( FlatView::Annotation& annot )
{
    annot.x1_.name_ = "Offset";
    annot.x1_.reversed_ = false;
    annot.x2_.name_ = SI().zIsTime() ? "TWT" : "Depth";
    annot.x2_.reversed_ = true;
    annot.x1_.showauxannot_ = false;
    annot.x2_.showauxannot_ = false;
    annot.x1_.annotinint_ = true;
    annot.x2_.annotinint_ = false;
    annot.x1_.showgridlines_ = true;
    annot.x2_.showgridlines_ = true;
    annot.color_ = OD::Color::Black();
    annot.allowuserchangereversedaxis_ = false;
}


uiViewer2DMainWin::uiViewer2DMainWin( uiParent* p, const char* title,
				      bool hasangledata )
    : uiObjectItemViewWin(p,uiObjectItemViewWin::Setup(title).startwidth(800))
    , seldatacalled_(this)
    , tkzs_(false)
    , hasangledata_(hasangledata)
{
    setDeleteOnClose( true );
}


uiViewer2DMainWin::~uiViewer2DMainWin()
{
    detachAllNotifiers();
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
    if ( posdlg_ )
	posdlg_->setSelGatherInfos( gatherinfos_ );
}


OD::GeomSystem uiViewer2DMainWin::geomSystem() const
{
    if ( !gatherinfos_.isEmpty() )
	return gatherinfos_.first().tk_.geomSystem();

    return OD::Geom3D;
}


class uiPSPreProcessingDlg : public uiDialog
{ mODTextTranslationClass(uiPSPreProcessingDlg);
public:
uiPSPreProcessingDlg( uiParent* p, PreStack::ProcessManager& ppmgr,
		      const CallBack& cb )
    : uiDialog(p,uiDialog::Setup(tr("Preprocessing"),uiString::emptyString(),
                                 mODHelpKey(mPreStackProcSelHelpID) ) )
    , cb_(cb)
{
    preprocgrp_ = new PreStack::uiProcessorManager( this, ppmgr );
    uiButton* applybut = uiButton::getStd( this, OD::Apply,
			    mCB(this,uiPSPreProcessingDlg,applyCB), true );
    applybut->attach( alignedBelow, preprocgrp_ );
}


private:

bool acceptOK( CallBacker* ) override
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

    return apply();
}


void applyCB( CallBacker* )
{
    apply();
}


bool apply()
{
    cb_.doCall( this );
    return true;
}


    PreStack::uiProcessorManager*	preprocgrp_;
    CallBack				cb_;
};



void uiViewer2DMainWin::preprocessingCB( CallBacker* )
{
    if ( !preprocmgr_ )
	preprocmgr_ = new PreStack::ProcessManager( geomSystem() );

    uiPSPreProcessingDlg ppdlg( this, *preprocmgr_,
				mCB(this,uiViewer2DMainWin,applyPreProcCB) );
    ppdlg.go();
}


void uiViewer2DMainWin::applyPreProcCB( CallBacker* )
{ setUpView(); }


void uiViewer2DMainWin::setUpView()
{
    uiMainWin win( this, m3Dots(tr("Creating gather displays")) );
    uiProgressBar pb( &win );
    pb.setPrefWidthInChar( 50 );
    pb.setStretch( 2, 2 );
    pb.setTotalSteps( mCast(int,gatherinfos_.size()) );
    win.show();

    removeAllGathers();

    if ( preprocmgr_ && !preprocmgr_->reset() )
	return uiMSG().error( tr("Can not preprocess data") );

    int nrvwr = 0;
    for ( const auto& ginfo : gatherinfos_ )
    {
	setGather( ginfo );
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


void uiViewer2DMainWin::removeAllGathers()
{
    removeAllItems();
    if ( control_ )
	control_->removeAllViewers();
    vwrs_.erase();

    deepErase( gd_ );
    deepErase( gdi_ );
}

#define sWidthForX2Annot 70

void uiViewer2DMainWin::reSizeItems()
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


void uiViewer2DMainWin::doHelp( CallBacker* )
{
    HelpProvider::provideHelp( HelpKey(mODHelpKey(mViewer2DPSMainWinHelpID) ) );
}


void uiViewer2DMainWin::displayMutes()
{
    for ( int gidx=0; gidx<nrItems(); gidx++ )
    {
	uiObjectItem* item = mainViewer()->getItem( gidx );
	mDynamicCastGet(uiGatherDisplay*,gd,item->getGroup());
	if ( !gd )
	    continue;

	uiFlatViewer* uivwr = gd->getUiFlatViewer();
	for ( int muteidx=0; muteidx<mutes_.size(); muteidx++ )
	{
	    const PreStack::MuteDef* mutedef = mutes_[muteidx];
	    const TrcKey tk = gd->getTrcKey();
	    FlatView::AuxData* muteaux =
			       uivwr->createAuxData( mutedef->name() );
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
		const float zval = mutedef->value( offset, tk.position() );
		muteaux->poly_ += FlatView::Point( offset, zval );
	    }

	    muteaux->namepos_ = FlatView::AuxData::Last;
	    muteaux->namealignment_ = mAlignment(Right,Bottom);
	    uivwr->addAuxData( muteaux );
	}
	uivwr->handleChange( sCast(od_uint32,FlatView::Viewer::Auxdata) );
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
    uiIOObjSelDlg::Setup sdsu( uiStrings::phrSelect(tr("Mute for display")) );
    sdsu.multisel( true );
    IOObjContext ctxt = mIOObjContext( MuteDef );
    uiIOObjSelDlg mutesel( this, sdsu, ctxt );
    if ( !mutesel.go() )
	return;

    clearAuxData();
    deepErase( mutes_ );
    mutecolors_.erase();
    const int nrsel = mutesel.nrChosen();
    for ( int idx=0; idx<nrsel; idx++ )
    {
	const MultiID& muteid = mutesel.chosenID( idx );
	PtrMan<IOObj> muteioobj = IOM().get( muteid );
	if ( !muteioobj )
	    continue;
	PreStack::MuteDef* mutedef = new PreStack::MuteDef;
	uiString errmsg;
	if ( !MuteDefTranslator::retrieve(*mutedef,muteioobj,errmsg) )
	    { uiMSG().error( errmsg ); continue; }

	mutes_ += mutedef;
	mutecolors_ += OD::getRandStdDrawColor();
    }

    displayMutes();
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

    auto* anglegather = new PreStack::Gather( anglefp, angledata.offsetType(),
					      angledata.zDomain() );
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


void uiStoredViewer2DMainWin::convAngleDataToDegrees(
					PreStack::Gather& angledata ) const
{
    const auto& agdata = const_cast<const Array2D<float>&>( angledata.data() );
    ArrayMath::getScaledArray<float>( agdata, nullptr, mRad2DegD, 0.,
				      false, true );
}


void uiViewer2DMainWin::displayInfo( CallBacker* cb )
{
    mCBCapsuleUnpack(IOPar,pars,cb);
    BufferString mesg;
    makeInfoMsg( mesg, pars );
    statusBar()->message( toUiString(mesg.buf()) );
}

BufferString getSettingsKey( bool isstored )
{
    mDeclStaticString( key );
    if ( isstored )
	key = PreStack::Gather::sDataPackCategory();
    else
	key = IOPar::compKey( PreStack::Gather::sDataPackCategory(),
			      sKey::Synthetic() ) ;
    return key;
}

class uiPSMultiPropDlg : public uiDialog
{ mODTextTranslationClass(uiPSMultiPropDlg);
public:
uiPSMultiPropDlg( uiParent* p, ObjectSet<uiFlatViewer>& vwrs,
		  const CallBack& cb, bool stored )
    : uiDialog(p,uiDialog::Setup(tr("Set properties for data"),
				 uiString::emptyString(),mNoHelpKey))
    , vwrs_(vwrs)
    , cb_(cb)
    , forstored_(stored)
{
    setCtrlStyle( uiDialog::CloseOnly );
    auto* lblcb = new uiLabeledComboBox( this, tr("Select gather") );
    datasetcb_ = lblcb->box();
    mAttachCB( datasetcb_->selectionChanged, uiPSMultiPropDlg::gatherChanged );
    auto* propbut = new uiPushButton( this, uiStrings::sProperties(),
			  uiPixmap("settings"),
			  mCB(this,uiPSMultiPropDlg,selectPropCB), false );
    propbut->attach( rightTo, lblcb );
}


~uiPSMultiPropDlg()
{
    detachAllNotifiers();
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
	datasetcb_->textOfItem( datasetcb_->currentItem() );
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


void uiViewer2DMainWin::setGatherView( uiGatherDisplay* gd,
				       uiGatherDisplayInfoHeader* gdi )
{
    const Interval<double> zrg( tkzs_.zsamp_.start, tkzs_.zsamp_.stop );
    gd->setZRange( tkzs_.zsamp_.width()==0 ? 0 : &zrg );
    gd->updateViewRange();
    uiFlatViewer* fv = gd->getUiFlatViewer();

    vwrs_ += fv;
    addGroup( gd, gdi );

    if ( !control_ )
    {
	auto* ctrl = new uiViewer2DControl( *mainviewer_, *fv, isStored() );
	mAttachCB( ctrl->posdlgcalled_, uiViewer2DMainWin::posDlgPushed );
	mAttachCB( ctrl->datadlgcalled_, uiViewer2DMainWin::dataDlgPushed );
	mAttachCB( ctrl->propChanged, uiViewer2DMainWin::propChangedCB );
	mAttachCB( ctrl->infoChanged, uiStoredViewer2DMainWin::displayInfo );
	ctrl->toolBar()->addButton( "snapshot", uiStrings::sTakeSnapshot(),
				mCB(this,uiStoredViewer2DMainWin,snapshotCB) );
	ctrl->toolBar()->addButton( "preprocessing", tr("Pre processing"),
				mCB(this,uiViewer2DMainWin,preprocessingCB) );
	control_ = ctrl;

	uiToolBar* tb = control_->toolBar();
	if ( tb )
	{
	    tb->addObject( new uiToolButton( tb, "mute", tr("Load Mute"),
				mCB(this,uiViewer2DMainWin,loadMuteCB) ) );

	    if  ( isStored() )
	    {
		mDynamicCastGet(const uiStoredViewer2DMainWin*,storedwin,this);
		if ( storedwin && !storedwin->is2D() )
		{
		    uiToolButtonSetup adtbsu(
			    "anglegather", tr("Display Angle Data"),
			    mCB(this,uiStoredViewer2DMainWin,angleDataCB) );
		    adtbsu.istoggle( true );
		    tb->addObject( new uiToolButton(tb,adtbsu) );
		}

		tb->addObject(
		    new uiToolButton( tb, "contexthelp", uiStrings::sHelp(),
			mCB(this,uiStoredViewer2DMainWin,doHelp) ) );
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
    fv->handleChange( sCast(od_uint32,FlatView::Viewer::DisplayPars) |
		      sCast(od_uint32,FlatView::Viewer::Annot) );
    control_->addViewer( *fv );
}


void uiViewer2DMainWin::propChangedCB( CallBacker* )
{
    PSViewAppearance curapp = control_->curViewerApp();
    const int appidx = appearances_.indexOf( curapp );
    if ( appidx>=0 )
	appearances_[appidx] = curapp;
}


void uiViewer2DMainWin::posDlgPushed( CallBacker* )
{
    if ( !posdlg_ )
    {
	BufferStringSet gathernms;
	getGatherNames( gathernms );
	posdlg_ = new uiViewer2DPosDlg( this, tkzs_, gathernms );
	mAttachCB( posdlg_->okpushed_, uiViewer2DMainWin::posDlgChgCB );
	mAttachCB( posdlg_->windowClosed, uiViewer2DMainWin::posDlgClosed );
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


void uiViewer2DMainWin::getStartupPositions( const TrcKey& tk,
				const StepInterval<int>& trcrg, bool isinl,
				TypeSet<TrcKey>& tks ) const
{
    int approxstep = trcrg.width()/sStartNrViewers;
    if ( !approxstep )
	approxstep = 1;

    const int starttrcnr = isinl ? tk.crl() : tk.inl();
    for ( int trcnr=starttrcnr; trcnr<=trcrg.stop; trcnr+=approxstep )
    {
	const int trcidx = trcrg.nearestIndex( trcnr );
	const int acttrcnr = trcrg.atIndex( trcidx );
	TrcKey postk = tk;
	if ( tk.is2D() || tk.isSynthetic() )
	    postk.setTrcNr( acttrcnr );
	else
	    postk.setInl( isinl ? tk.inl() : acttrcnr )
		 .setCrl( isinl ? acttrcnr : tk.crl() );
	tks.addIfNew( postk );
	if ( tks.size() >= sStartNrViewers )
	    return;
    }

    for ( int trcnr=starttrcnr-approxstep; trcnr>=trcrg.start;trcnr-=approxstep)
    {
	const int trcidx = trcrg.nearestIndex( trcnr );
	const int acttrcnr = trcrg.atIndex( trcidx );
	TrcKey postk = tk;
	if ( tk.is2D() || tk.isSynthetic() )
	    postk.setTrcNr( acttrcnr );
	else
	    postk.setInl( isinl ? tk.inl() : acttrcnr )
		 .setCrl( isinl ? acttrcnr : tk.crl() );
	tks.insert( 0, postk );
	if ( tks.size() >= sStartNrViewers )
	    return;
    }
}


void uiViewer2DMainWin::posDlgClosed( CallBacker* )
{
    if ( posdlg_ && posdlg_->uiResult() == 0 )
	posdlg_->setSelGatherInfos( gatherinfos_ );
}


void uiViewer2DMainWin::setAppearance( const FlatView::Appearance& app,
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
	vwr->handleChange( sCast(od_uint32,FlatView::Viewer::DisplayPars) |
			   sCast(od_uint32,FlatView::Viewer::Annot) );
    }
}


void uiViewer2DMainWin::prepareNewAppearances( BufferStringSet oldgathernms,
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
		ColTab::MapperSetup& vdmapper = psapp.ddpars_.vd_.mappersetup_;
		vdmapper.autosym0_ = false;
		vdmapper.symmidval_ = mUdf(float);
		vdmapper.type_ = ColTab::MapperSetup::Fixed;
		vdmapper.range_ = Interval<float>(0,60);
		psapp.ddpars_.vd_.ctab_ = ColTab::Sequence::sKeyRainbow();
		ColTab::MapperSetup& wvamapper =psapp.ddpars_.wva_.mappersetup_;
		wvamapper.cliprate_ = Interval<float>(0.0,0.0);
		wvamapper.autosym0_ = true;
		wvamapper.symmidval_ = 0.0f;
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


ConstRefMan<PreStack::Gather> uiViewer2DMainWin::getPreProcessed(
						    const GatherInfo& ginfo )
{
    if ( !preprocmgr_->prepareWork() )
	return nullptr;

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

	    TrcKey tk = ginfo.tk_;
	    BinID bid = relbid * facbid;
	    if ( tk.is3D() )
	    {
		bid.inl() += tk.inl();
		tk.setInl( bid.inl() );
	    }

	    bid.trcNr() += const_cast<const TrcKey&>( tk ).trcNr();
	    tk.setTrcNr( bid.trcNr() );
	    GatherInfo relposginfo;
	    relposginfo.isstored_ = ginfo.isstored_;
	    relposginfo.gathernm_ = ginfo.gathernm_;
	    relposginfo.tk_ = tk;
	    if ( isStored() )
		relposginfo.mid_ = ginfo.mid_;

	    setGatherforPreProc( relbid, relposginfo );
	}
    }

    if ( !preprocmgr_->process() )
    {
	uiMSG().error( preprocmgr_->errMsg() );
	return nullptr;
    }

    return preprocmgr_->getOutput();
}


void uiViewer2DMainWin::setGatherforPreProc( const BinID& relbid,
					     const GatherInfo& ginfo )
{
    if ( ginfo.isstored_ )
    {
	RefMan<PreStack::Gather> gather = new PreStack::Gather;
	DPM(DataPackMgr::FlatID()).add( gather );
	mDynamicCastGet(const uiStoredViewer2DMainWin*,storedpsmw,this);
	if ( !storedpsmw )
	    return;

	if ( gather->readFrom(ginfo.mid_,ginfo.tk_) )
	    preprocmgr_->setInput( relbid, gather->id() );
    }
    else
    {
	const int gidx = gatherinfos_.indexOf( ginfo );
	if ( gidx < 0 )
	    return;

	const GatherInfo& inputginfo = gatherinfos_[gidx];
	const ElasticModel* model = getModel( inputginfo.tk_ );
	preprocmgr_->setInput( relbid, inputginfo.vddp_ ? inputginfo.wvadp_
							: inputginfo.vddp_ );
	preprocmgr_->setModel( relbid, model );
    }
}


bool uiViewer2DMainWin::getStoredAppearance( PSViewAppearance& psapp ) const
{
    Settings& setts = Settings::fetch( "flatview" );
    const BufferString key( getSettingsKey(isStored()) );
    PtrMan<IOPar> iop = setts.subselect( key );
    if ( !iop && isStored() ) // for older stored par files
	iop = setts.subselect( PreStack::Gather::sDataPackCategory() );

    if ( !iop )
	return false;

    psapp.usePar( *iop );
    return true;
}


// uiStoredViewer2DMainWin

uiStoredViewer2DMainWin::uiStoredViewer2DMainWin( uiParent* p,
						  const char* title, bool is2d )
    : uiViewer2DMainWin(p,title,false)
    , is2d_(is2d)
{
    if ( !is2d_ )
    {
	slicepos_ = new uiSlicePos2DView( this, ZDomain::Info(SI().zDomain()) );
	mAttachCB( slicepos_->positionChg,
		   uiStoredViewer2DMainWin::posSlcChgCB );
    }
}


uiStoredViewer2DMainWin::~uiStoredViewer2DMainWin()
{
    detachAllNotifiers();
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


void uiStoredViewer2DMainWin::init( const MultiID& mid, const TrcKey& tk,
				    bool isinl, const StepInterval<int>& trcrg )
{
    if ( tk.isSynthetic() )
	{ pErrMsg("Incorrect TrcKey type"); }

    mids_ += mid;
    tkzs_.hsamp_.setGeomID( tk.geomID() );
    tkzs_.zsamp_ = SI().zRange(true);

    if ( tk.is2D() )
	tkzs_.hsamp_.setTrcRange( trcrg );
    else
    {
	tkzs_.hsamp_.set( tk );
	if ( isinl )
	    tkzs_.hsamp_.setCrlRange( trcrg );
	else
	    tkzs_.hsamp_.setInlRange( trcrg );

	slicepos_->setTrcKeyZSampling( tkzs_ );
    }

    setUpNewPositions( isinl, tk, trcrg );
    setUpNewIDs();
}


void uiStoredViewer2DMainWin::setUpNewSlicePositions()
{
    const bool usetraces = tkzs_.hsamp_.is2D();
    const bool isinl = usetraces || tkzs_.defaultDir()==TrcKeyZSampling::Inl;
    const int newpos = isinl ? tkzs_.hsamp_.start_.inl()
			     : tkzs_.hsamp_.start_.crl();

    for ( auto& ginfo : gatherinfos_ )
    {
	TrcKey& tk = ginfo.tk_;
	if ( usetraces )
	    tk.setTrcNr( newpos );
	else
	{
	    if ( isinl )
		tk.setInl( newpos );
	    else
		tk.setCrl( newpos );
	}
    }
}


void uiStoredViewer2DMainWin::setUpNewPositions(bool isinl, const TrcKey& tk,
						const StepInterval<int>& trcrg )
{
    TypeSet<TrcKey> tks;
    getStartupPositions( tk, trcrg, isinl, tks );
    gatherinfos_.setEmpty();
    for ( const auto& tkpos : tks )
    {
	GatherInfo ginfo;
	ginfo.isselected_ = true;
	ginfo.tk_ = tkpos;
	ginfo.isselected_ = true;
	gatherinfos_ += ginfo;
    }
}


void uiStoredViewer2DMainWin::setIDs( const TypeSet<MultiID>& mids  )
{
    mids_ = mids;
    setUpNewIDs();
}


void uiStoredViewer2DMainWin::setUpNewIDs()
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
	    PtrMan<IOObj> gatherioobj = IOM().get( mids_[midx] );
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
    if ( posdlg_ )
	posdlg_->setSelGatherInfos( gatherinfos_ );

    setUpView();
}


void uiStoredViewer2DMainWin::setGatherInfo( uiGatherDisplayInfoHeader* info,
					     const GatherInfo& ginfo )
{
    PtrMan<IOObj> ioobj = IOM().get( ginfo.mid_ );
    BufferString nm = ioobj ? ioobj->name().buf() : "";
    info->setData( ginfo.tk_, tkzs_.defaultDir()==TrcKeyZSampling::Inl,
		   is2d_, nm );
}


void uiStoredViewer2DMainWin::posDlgChgCB( CallBacker* )
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
		PtrMan<IOObj> gatherioobj = IOM().get( mids_[midx] );
		if ( !gatherioobj ) continue;
		if ( ginfo.gathernm_ == gatherioobj->name() )
		    ginfo.mid_ = mids_[midx];
	    }
	}
    }

    if ( slicepos_ )
	slicepos_->setTrcKeyZSampling( tkzs_ );

    setUpView();
}


void uiStoredViewer2DMainWin::posSlcChgCB( CallBacker* )
{
    if ( slicepos_ )
	tkzs_ = slicepos_->getTrcKeyZSampling();

    if ( posdlg_ )
	posdlg_->setTrcKeyZSampling( tkzs_ );

    setUpNewSlicePositions();
    setUpNewIDs();
}



ConstRefMan<PreStack::Gather> uiStoredViewer2DMainWin::getAngleData(
						const PreStack::Gather* gd )
{
    if ( !hasangledata_ || !angleparams_ || !gd )
	return nullptr;

    ConstRefMan<PreStack::Gather> gather = gd;
    RefMan<PreStack::VelocityBasedAngleComputer> velangcomp =
				new PreStack::VelocityBasedAngleComputer();
    velangcomp->setMultiID( angleparams_->velvolmid_ );
    velangcomp->setRayTracerPars( angleparams_->raypar_ );
    velangcomp->setSmoothingPars( angleparams_->smoothingpar_ );
    const FlatPosData& fp = gather->posData();
    velangcomp->setOutputSampling( fp, gather->offsetType(),
				   gather->zDomain() );
    velangcomp->setGatherIsNMOCorrected( gather->isCorrected() );
    velangcomp->setTrcKey( TrcKey(gather->getBinID()) );
    RefMan<PreStack::Gather> angledata = velangcomp->computeAngles();
    if ( !angledata )
	return nullptr;

    velangcomp = nullptr;

    BufferString angledpnm( gather->name(), " Incidence Angle" );
    angledata->setName( angledpnm );
    convAngleDataToDegrees( *angledata );
    if ( doanglegather_ )
    {
	RefMan<PreStack::Gather> anglegather =
	     getAngleGather( *gather, *angledata, angleparams_->anglerange_ );
	return anglegather;
    }

    return angledata;
}


class uiAngleCompParDlg : public uiDialog
{ mODTextTranslationClass(uiAngleCompParDlg)
public:

uiAngleCompParDlg( uiParent* p, PreStack::AngleCompParams& acp,
		   OD::GeomSystem gs, bool isag )
    : uiDialog(p,uiDialog::Setup(uiString::emptyString(),
				 uiString::emptyString(),
			      mODHelpKey(mViewer2DPSMainWindisplayAngleHelpID)))
{
    uiString windowtitle = isag ? tr("Angle Gather Display")
				: tr("Angle Data Display");

    setTitleText( windowtitle );
    uiString windowcaption = uiStrings::phrSpecify(tr("Parameters for %1")
							     .arg(windowtitle));
    setCaption( windowcaption );
    anglegrp_ = new PreStack::uiAngleCompGrp( this, acp, gs, false, false );
}

protected:

bool acceptOK( CallBacker* ) override
{
    return anglegrp_->acceptOK();
}

PreStack::uiAngleCompGrp*	anglegrp_;

};


bool uiStoredViewer2DMainWin::getAngleParams()
{
    if ( !angleparams_ )
    {
	angleparams_ = new PreStack::AngleCompParams;
	if ( !doanglegather_ )
	    angleparams_->anglerange_ = Interval<int>( 0 , 60 );
    }

    const OD::GeomSystem gs = is2D() ? OD::Geom2D : OD::Geom3D;
    uiAngleCompParDlg agcompdlg( this, *angleparams_, gs, doanglegather_ );
    return agcompdlg.go();
}


void uiStoredViewer2DMainWin::angleGatherCB( CallBacker* cb )
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


void uiStoredViewer2DMainWin::angleDataCB( CallBacker* cb )
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


void uiStoredViewer2DMainWin::displayAngle()
{
    for ( int dataidx=0; dataidx<appearances_.size(); dataidx++ )
    {
	if ( doanglegather_ ) continue;
	PSViewAppearance& psapp = appearances_[dataidx];
	ColTab::MapperSetup& vdmapper = psapp.ddpars_.vd_.mappersetup_;
	if ( hasangledata_ )
	{
	    psapp.ddpars_.vd_.show_ = true;
	    psapp.ddpars_.wva_.show_ = true;
	    vdmapper.autosym0_ = false;
	    vdmapper.symmidval_ = mUdf(float);
	    vdmapper.type_ = ColTab::MapperSetup::Fixed;
	    Interval<float> anglerg(
		    mCast(float,angleparams_->anglerange_.start),
		    mCast(float,angleparams_->anglerange_.stop) );
	    vdmapper.range_ = anglerg;
	    psapp.ddpars_.vd_.ctab_ = ColTab::Sequence::sKeyRainbow();
	}
	else
	{
	    psapp.ddpars_.vd_.show_ = true;
	    psapp.ddpars_.wva_.show_ = false;
	    psapp.ddpars_.vd_.ctab_ = "Seismics";
	    vdmapper.cliprate_ = Interval<float>(0.025,0.025);
	    vdmapper.type_ = ColTab::MapperSetup::Auto;
	    vdmapper.autosym0_ = true;
	    vdmapper.symmidval_ = 0.0f;
	}
    }

    setUpView();
}


void uiStoredViewer2DMainWin::setGather( const GatherInfo& gatherinfo )
{
    if ( !gatherinfo.isselected_ )
	return;

    Interval<float> zrg( mUdf(float), 0 );
    auto* gd = new uiGatherDisplay( nullptr );
    RefMan<PreStack::Gather> gather = new PreStack::Gather;
    DPM(DataPackMgr::FlatID()).add( gather );
    const MultiID& mid = gatherinfo.mid_;
    const TrcKey& tk = gatherinfo.tk_;
    if ( gather->readFrom(mid,tk) )
    {
	ConstRefMan<PreStack::Gather> ppgather;
	if ( preprocmgr_ && preprocmgr_->nrProcessors() )
	    ppgather = getPreProcessed( gatherinfo );

	ppgather = ppgather ? ppgather : ConstRefMan<PreStack::Gather>(gather);
	ConstRefMan<PreStack::Gather> anglegather = getAngleData( ppgather );
	gd->setVDGather( hasangledata_ ? anglegather : ppgather );
	gd->setWVAGather( hasangledata_ ? ppgather : nullptr );
	if ( mIsUdf( zrg.start ) )
	   zrg = gd->getZDataRange();

	zrg.include( gd->getZDataRange() );
    }
    else
	gd->setVDGather( nullptr );

    auto* gdi = new uiGatherDisplayInfoHeader( nullptr );
    setGatherInfo( gdi, gatherinfo );
    gdi->setOffsetRange( gd->getOffsetRange() );
    setGatherView( gd, gdi );

    gd_ += gd;
    gdi_ += gdi;
}


// uiSyntheticViewer2DMainWin

uiSyntheticViewer2DMainWin::uiSyntheticViewer2DMainWin( uiParent* p,
							const char* title )
    : uiViewer2DMainWin(p,title,true)
{
    tkzs_ = TrcKeyZSampling::getSynth();
}


uiSyntheticViewer2DMainWin::~uiSyntheticViewer2DMainWin()
{
}


void uiSyntheticViewer2DMainWin::setGatherNames( const BufferStringSet& nms)
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
    if ( posdlg_ )
	posdlg_->setSelGatherInfos( gatherinfos_ );

    setUpView();
}


void uiSyntheticViewer2DMainWin::getGatherNames( BufferStringSet& nms) const
{
    nms.erase();
    for ( int idx=0; idx<gatherinfos_.size(); idx++ )
	nms.addIfNew( gatherinfos_[idx].gathernm_ );
}


void uiSyntheticViewer2DMainWin::posDlgChgCB( CallBacker* )
{
    if ( posdlg_ )
    {
	TypeSet<GatherInfo> gatherinfos;
	posdlg_->getTrcKeyZSampling( tkzs_ );
	posdlg_->getSelGatherInfos( gatherinfos );
	for ( auto& dpinfo : gatherinfos_ )
	    dpinfo.isselected_ = false;

	for ( const auto& ginfo : gatherinfos )
	{
	    for ( auto& dpinfo : gatherinfos_ )
	    {
		if ( dpinfo.gathernm_==ginfo.gathernm_ &&
		     dpinfo.tk_==ginfo.tk_ )
		    dpinfo.isselected_ = ginfo.isselected_;
	    }
	}
    }

    setUpView();
}


uiSyntheticViewer2DMainWin&
uiSyntheticViewer2DMainWin::setGathers( const TypeSet<GatherInfo>& dps,
					const TrcKeyZSampling& tkzs )
{
    TypeSet<PSViewAppearance> oldapps = appearances_;
    appearances_.setEmpty();
    BufferStringSet oldgathernms;
    for ( const auto& ginfo : gatherinfos_ )
	oldgathernms.addIfNew( ginfo.gathernm_ );

    gatherinfos_ = dps;
    BufferStringSet newgathernms;
    for ( const auto& ginfo : gatherinfos_ )
    {
	PreStackView::GatherInfo newginfo( ginfo );
	PSViewAppearance dummypsapp;
	dummypsapp.datanm_ = newginfo.gathernm_;
	newgathernms.addIfNew( newginfo.gathernm_ );
	if ( oldapps.isPresent(dummypsapp) &&
	     !appearances_.isPresent(dummypsapp) )
	    oldgathernms.addIfNew( newginfo.gathernm_ );
    }

    prepareNewAppearances( oldgathernms, newgathernms );
    tkzs_ = tkzs;
    setUpView();
    reSizeSld( nullptr );

    return *this;
}


uiSyntheticViewer2DMainWin&
uiSyntheticViewer2DMainWin::setModels( const ElasticModelSet& models )
{
    models_ = &models;
    return *this;
}


void uiSyntheticViewer2DMainWin::setGather( const GatherInfo& ginfo )
{
    if ( !ginfo.isselected_ )
	return;

    auto* gd = new uiGatherDisplay( nullptr );
    auto vdgather = ginfo.vddp_;
    auto wvagather = ginfo.wvadp_;

    if ( !vdgather && !wvagather  )
    {
	gd->setVDGather( nullptr );
	gd->setWVAGather( nullptr );
	return;
    }

    if ( !posdlg_ )
	tkzs_.zsamp_.include( wvagather ? wvagather->zRange()
				        : vdgather->zRange(), false );
    ConstRefMan<PreStack::Gather> ppgather;
    if ( preprocmgr_ && preprocmgr_->nrProcessors() )
	ppgather = getPreProcessed( ginfo );

    gd->setVDGather( !vdgather ? ppgather ? ppgather : wvagather : vdgather );
    gd->setWVAGather( vdgather ? ppgather ? ppgather : wvagather : nullptr );
    auto* gdi = new uiGatherDisplayInfoHeader( nullptr );
    setGatherInfo( gdi, ginfo );
    gdi->setOffsetRange( gd->getOffsetRange() );
    setGatherView( gd, gdi );

    gd_ += gd;
    gdi_ += gdi;
}


uiSyntheticViewer2DMainWin& uiSyntheticViewer2DMainWin::removeGathers()
{
    gatherinfos_.setEmpty();
    return *this;
}


uiSyntheticViewer2DMainWin& uiSyntheticViewer2DMainWin::removeModels()
{
    models_ = nullptr;
    return *this;
}


void uiSyntheticViewer2DMainWin::setGatherInfo( uiGatherDisplayInfoHeader* info,
						const GatherInfo& ginfo )
{
    info->setData( ginfo.tk_.trcNr(), ginfo.gathernm_ );
}


const ElasticModel* uiSyntheticViewer2DMainWin::getModel(
							const TrcKey& tk ) const
{
    if ( !models_ )
	return nullptr;

    const int trcidx = tkzs_.hsamp_.trcIdx( tk.trcNr() );
    if ( !models_->validIdx(trcidx) )
	return nullptr;

    return models_->get( trcidx );
}


// uiViewer2DControl

#define mDefBut(fnm,cbnm,tt) \
    tb_->addButton( fnm, tt, mCB(this,uiViewer2DControl,cbnm) );

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
    , isstored_(isstored)
{
    removeViewer( vwr );
    clearToolBar();

    objectitemctrl_ = new uiObjectItemViewControl( mw );
    tb_ = objectitemctrl_->toolBar();

    mDefBut("orientation64",gatherPosCB,tr("Set positions"));
    mDefBut("gatherdisplaysettings64",gatherDataCB,
	    tr("Set gather data"));
    mDefBut("2ddisppars",propertiesDlgCB,
	    tr("Set seismic display properties"));
    ctabsel_ = new uiColorTableSel( tb_, "Select Color Table" );
    mAttachCB( ctabsel_->selectionChanged, uiViewer2DControl::coltabChg );
    mAttachCB( vwr_.dispParsChanged, uiViewer2DControl::updateColTabCB );
    ctabsel_->setCurrent( dispPars().vd_.ctab_ );
    tb_->addObject( ctabsel_ );
    tb_->addSeparator();
}


uiViewer2DControl::~uiViewer2DControl()
{
    detachAllNotifiers();
    delete pspropdlg_;
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
	vwr.handleChange( sCast(od_uint32,FlatView::Viewer::DisplayPars) );
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
    if ( !pspropdlg_ )
	return;

    TypeSet<int> vwridxs = pspropdlg_->activeViewerIdx();
    const int actvwridx = vwridxs[0];
    if ( !vwrs_.validIdx(actvwridx) )
	return;

    app_ = vwrs_[ actvwridx ]->appearance();
    propChanged.trigger();
    ctabsel_->setCurrent( app_.ddpars_.vd_.ctab_.buf() );

    ConstRefMan<FlatDataPack> vddatapack =
			      vwrs_[actvwridx]->getPack(false).get();
    ConstRefMan<FlatDataPack> wvadatapack =
			      vwrs_[actvwridx]->getPack(true).get();
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

	bool bitmapchanged = false;
	const bool doupdate = vwr.enableChange( false );
	RefMan<FlatDataPack> wvadp = vwr.getPack( true ).get();
	RefMan<FlatDataPack> vddp = vwr.getPack( false ).get();
	bool wva = wvadatapack && wvadatapack->name() == wvadp->name() &&
			     app_.ddpars_.wva_.show_ ;
	bool vd = vddatapack && vddatapack->name() == wvadp->name() &&
			    app_.ddpars_.vd_.show_;
	FlatView::Viewer::VwrDest dest = FlatView::Viewer::getDest( wva, vd );
	if ( dest != FlatView::Viewer::None )
	{
	    bitmapchanged = true;
	    vwr.setPack( dest, wvadp, false );
	}

	wva = wvadatapack && wvadatapack->name() == vddp->name() &&
			     app_.ddpars_.wva_.show_ ;
	vd = vddatapack && vddatapack->name() == vddp->name() &&
			    app_.ddpars_.vd_.show_;
	dest = FlatView::Viewer::getDest( wva, vd );
	if ( dest != FlatView::Viewer::None )
	{
	    bitmapchanged = true;
	    vwr.setPack( dest, vddp, false );
	}

	vwr.enableChange( doupdate );
	od_uint32 ctyp = sCast(od_uint32,FlatView::Viewer::DisplayPars) |
			 sCast(od_uint32,FlatView::Viewer::Annot);
	if ( bitmapchanged )
	    ctyp |= sCast(od_uint32,FlatView::Viewer::BitmapData);

	vwr.handleChange( ctyp );
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
    ctabsel_->setSensitive( showcoltab );
    uiString tooltipstr;
    if ( showcoltab )
	tooltipstr = uiString::empty();
    else
	tooltipstr = tr( "Mutiple datasets selected. "
			 "Use 'Set seismic display properties' button" );

    ctabsel_->setToolTip( tooltipstr );
}


DataPackID uiViewer2DMainWin::getPreProcessedID( const GatherInfo& ginfo )
{
     ConstRefMan<PreStack::Gather> gather = getPreProcessed( ginfo );
     return gather ? gather->id() : DataPack::cNoID();
}


DataPackID uiStoredViewer2DMainWin::getAngleData( DataPackID gatherid )
{
    if ( !hasangledata_ || !angleparams_ )
	return DataPack::cNoID();

    auto gather = DPM(DataPackMgr::FlatID()).get<PreStack::Gather>( gatherid );
    auto angle_gather = getAngleData( gather );
    if ( !angle_gather )
	return DataPack::cNoID();

    return angle_gather->id();
}

} // namespace PreStackView
