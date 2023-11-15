/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratlayermodel.h"

#include "ctxtioobj.h"
#include "elasticpropsel.h"
#include "hiddenparam.h"
#include "ioobj.h"
#include "keystrs.h"
#include "objdisposer.h"
#include "od_helpids.h"
#include "od_istream.h"
#include "od_ostream.h"
#include "stratlayermodel.h"
#include "stratlaymodgen.h"
#include "stratlayseqgendesc.h"
#include "stratreftree.h"
#include "stratsynth.h"
#include "strattransl.h"
#include "unitofmeasure.h"

#include "uiflatviewer.h"
#include "uigroup.h"
#include "uiioobjseldlg.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uimultiflatviewcontrol.h"
#include "uisaveimagedlg.h"
#include "uisplitter.h"
#include "uistatusbar.h"
#include "uistratbasiclayseqgendesc.h"
#include "uistratlayermodel.h"
#include "uistratlaymoddisp.h"
#include "uistratlaymodtools.h"
#include "uistratsynthcrossplot.h"
#include "uistratsynthdisp.h"
#include "uistrattreewin.h"
#include "uistrings.h"
#include "uisynthgendlg.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"


mDefineInstanceCreatedNotifierAccess(uiStratLayerModel)

const char* uiStratLayerModel::sKeyModeler2Use()
{
    return "dTect.Stratigraphic Modeler to use";
}


uiStratLayerModel::uiStratLayerModel( uiParent* p, const char* edtyp, int opt )
    : uiMainWin(p,uiString::empty(),1,true)
    , beforeSave(this)
    , afterRetrieve(this)
    , desc_(*new Strat::LayerSequenceGenDesc(Strat::RT()))
    , lms_(*new Strat::LayerModelSuite)
    , descctio_(*mMkCtxtIOObj(StratLayerSequenceGenDesc))
    , nrmodels_(0)
{
    setDeleteOnClose( true );

    if ( !edtyp || !*edtyp )
	edtyp = uiBasicLayerSequenceGenDesc::typeStr();

    descctio_.ctxt_.toselect_.require_.set( sKey::Type(), edtyp );

    auto* gengrp = new uiGroup( this, "Gen group" );
    descdisp_ = uiLayerSequenceGenDesc::factory().create( edtyp, gengrp, desc_);
    if ( !descdisp_ )
	descdisp_ = new uiBasicLayerSequenceGenDesc( gengrp, desc_ );

    uiGroup* topgrp; uiGroup* botgrp; uiGroup* rightgrp = nullptr;
    if ( descdisp_->separateDisplay() )
    {
	rightgrp = new uiGroup( this, "Right group" );
	topgrp = new uiGroup( rightgrp, "Top group" );
	botgrp = new uiGroup( rightgrp, "Bot group" );
    }
    else
    {
	topgrp = new uiGroup( this, "Top group" );
	botgrp = gengrp;
    }

    modtools_ = new uiStratLayModEditTools( botgrp );
    gentools_ = new uiStratGenDescTools( gengrp );

    synthdatamgr_ = new StratSynth::DataMgr( lms_ );
    synthdatamgrdc_ = synthdatamgr_->dirtyCount();
    moddisp_ = descdisp_->getLayModDisp( *modtools_, lms_, opt );
    if ( !moddisp_ )
    {
	new uiLabel( this, tr("Start cancelled") );
	close();
	return;
    }

    if ( moddisp_->isPerModelDisplay() )
    {
	modtools_->addEachFld();
	modtools_->setMaxDispEach( gentools_->nrModels() );
	modtools_->addLithFld();
    }

    synthdisp_ = new uiStratSynthDisp( topgrp, *synthdatamgr_.ptr(),
				       *modtools_, moddisp_->initialSize() );

    analtb_ = new uiToolBar( this, tr("Analysis toolbar"), uiToolBar::Right );
    uiToolButtonSetup tbsu( "xplot", tr("Attributes vs model properties"),
			    mCB(this,uiStratLayerModel,xPlotReq) );
    synthdisp_->control()->getToolBar(0)->addButton(
				"snapshot", uiStrings::sTakeSnapshot(),
				mCB(this,uiStratLayerModel,snapshotCB) );
    analtb_->addButton( tbsu );
    mDynamicCastGet( uiFlatViewer*, moddispvwr, moddisp_->getViewer() );
    if ( moddispvwr )
    {
	synthdisp_->addViewerToControl( *moddispvwr );
	synthdisp_->control()->setParsButToolTip( *moddispvwr,
				tr("Layermodel display properties") );
    }

    modtools_->attach( ensureBelow, moddisp_ );
    gentools_->attach( ensureBelow, descdisp_->outerObj() );

    auto* helptb = new uiToolBar( this, tr("Help toolbar"), uiToolBar::Right );
    uiToolButtonSetup htbsu( "contexthelp", uiStrings::sHelp(),
			     mCB(this,uiStratLayerModel,helpCB) );
    helptb->addButton( htbsu );

    uiParent* hsplitparent;
    if ( descdisp_->separateDisplay() )
    {
	modtools_->attach( rightBorder );
	auto* vspl = new uiSplitter( this, "Desc-LayModDisp Split", true );
	vspl->addGroup( gengrp ); vspl->addGroup( rightgrp );
	hsplitparent = rightgrp;
    }
    else
    {
	modtools_->attach( rightOf, gentools_ );
	hsplitparent = this;
    }

    auto* horspl = new uiSplitter( hsplitparent, "Synth-LayModDisp Splitter",
				   OD::Vertical );
    horspl->addGroup( topgrp );
    horspl->addGroup( botgrp );

    setWinTitle();
    StratTreeWin().changeLayerModelNumber( true );

    mAttachCB( postFinalize(), uiStratLayerModel::initWin );
    mTriggerInstanceCreatedNotifier();
}


uiStratLayerModel::~uiStratLayerModel()
{
    detachAllNotifiers();
    delete &desc_;
    delete &lms_;
    delete descctio_.ioobj_; delete &descctio_;
    StratTreeWin().changeLayerModelNumber( false );
    UnitOfMeasure::saveCurrentDefaults();
}


void uiStratLayerModel::initWin( CallBacker* )
{
    if ( !moddisp_ )
    {
	modtools_->display( false );
	gentools_->display( false );
	OBJDISP()->go( this );
	return;
    }

    mAttachCB( gentools_->openReq, uiStratLayerModel::openGenDescCB );
    mAttachCB( gentools_->saveReq, uiStratLayerModel::saveGenDescCB );
    mAttachCB( gentools_->propEdReq, uiStratLayerModel::manPropsCB );
    mAttachCB( gentools_->genReq, uiStratLayerModel::genModelsCB );

    // This one should be the first callback to be called when model changes.
    lms_.modelChanged.notify( mCB(this,uiStratLayerModel,modChgCB), true );

    mAttachCB( synthdatamgr_->elPropSelChanged,
	       uiStratLayerModel::elasticPropsCB );
    mAttachCB( synthdisp_->control()->infoChanged,
	       uiStratLayerModel::synthInfoChangedCB );

    mAttachCB( moddisp_->genNewModelNeeded, uiStratLayerModel::genModelsCB );
    mAttachCB( moddisp_->infoChanged, uiStratLayerModel::modInfoChangedCB );
    mAttachCB( moddisp_->sequenceSelected, uiStratLayerModel::seqSelCB );
    mAttachCB( moddisp_->sequencesAdded, uiStratLayerModel::seqsAddedCB );
}


void uiStratLayerModel::setWinTitle()
{
    uiString descnm( descctio_.ioobj_ ? descctio_.ioobj_->uiName()
				      : uiStrings::sNew() );
    uiString txt( tr("Layer modeling [%1]").arg(descnm) );
    setCaption( txt );
}


Strat::LayerModel& uiStratLayerModel::layerModel()
{
    return lms_.getCurrent();
}


const Strat::LayerModel& uiStratLayerModel::layerModel() const
{
    return lms_.getCurrent();
}


void uiStratLayerModel::snapshotCB( CallBacker* )
{
    uiSaveWinImageDlg snapshotdlg( this );
    snapshotdlg.go();
}


const HelpKey& uiStratLayerModel::helpKey() const
{
    return helpkey_;
}


void uiStratLayerModel::setHelpKey( const HelpKey& key )
{
    helpkey_ = key;
}


void uiStratLayerModel::helpCB( CallBacker* )
{
    HelpProvider::provideHelp( helpKey() );
}


void uiStratLayerModel::synthInfoChangedCB( CallBacker* cb )
{
    if ( !cb )
	return;

    mCBCapsuleUnpack( IOPar, iop, cb );
    BufferString todisp;
    synthdisp_->makeInfoMsg( todisp, iop );
    statusBar()->message( mToUiStringTodo(todisp.buf()) );
}


void uiStratLayerModel::modInfoChangedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const uiString*, dispmsg, cb );
    if ( !dispmsg )
	{ pErrMsg("Huh"); }
    else
	statusBar()->message( *dispmsg );
}


void uiStratLayerModel::xPlotReq( CallBacker* )
{
    if ( !checkUnscaledWavelets() )
	return;

    uiStratSynthCrossplot dlg( this, *synthdatamgr_.ptr() );
    dlg.setRefLevel( curLevelID() );
    dlg.go();
}


void uiStratLayerModel::manPropsCB( CallBacker* )
{
    descdisp_->selProps();
}


void uiStratLayerModel::elasticPropsCB( CallBacker* )
{
    const ElasticPropSelection& elpropsel = lms_.getCurrent().elasticPropSel();
    const MultiID& elpropkey = elpropsel.getStoredID();
    if ( !elpropkey.isUdf() )
	desc_.setElasticPropSel( elpropkey );

    IOPar& wbpars = desc_.getWorkBenchParams();
    synthdisp_->fillPar( wbpars );
    wbpars.removeWithKey( ElasticPropSelection::sKeyElasticPropSel() );

    descdisp_->setNeedSave( true );
    descdisp_->setEditDesc();
    descdisp_->descHasChanged();
}


void uiStratLayerModel::setNrToGen( int nrmodels )
{
    gentools_->setNrModels( nrmodels );
    gentools_->fillPar( desc_.getWorkBenchParams() );
}


int uiStratLayerModel::nrToGen() const
{
    int nrmodels = gentools_->getNrModelsFromPar( desc_.getWorkBenchParams() );
    if ( nrmodels<0 )
	nrmodels = gentools_->nrModels();
    return nrmodels;
}


bool uiStratLayerModel::checkUnscaledWavelets()
{
    const uiSynthParsGrp::ScaleRes res =
	uiSynthParsGrp::checkUnscaledWavelets( this, *synthdatamgr_.ptr() );
    return uiSynthParsGrp::isOK( res );
}


bool uiStratLayerModel::saveGenDescIfNecessary( bool allowcncl ) const
{
    const bool datamgrchgd = synthdatamgr_->dirtyCount() != synthdatamgrdc_;
    if ( !descdisp_->needSave() && !datamgrchgd )
	return true;

    while ( true )
    {
	const int res = uiMSG().askSave(tr("Unsaved generation parameters.\n"
					 "Save now?") );
	if ( !allowcncl && res < 0 )
	{
	    uiMSG().error( tr("Sorry, you cannot cancel right now.\n"
			   "Please save or discard your work") );
	    continue;
	}
	if ( res < 1 )
	    return res == 0;
	break;
    }

    return saveGenDesc();
}


bool uiStratLayerModel::saveGenDesc() const
{
    descctio_.ctxt_.forread_ = false;
    uiIOObjSelDlg dlg( const_cast<uiStratLayerModel*>(this), descctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;
    descctio_.setObj( dlg.ioObj()->clone() );

    const BufferString fnm( descctio_.ioobj_->fullUserExpr(false) );
    bool rv = false;

    MouseCursorChanger mcch( MouseCursor::Wait );
    const Strat::LayerSequenceGenDesc& desc = descdisp_->currentDesc();
    IOPar& wbpars =
	const_cast<Strat::LayerSequenceGenDesc&>(desc).getWorkBenchParams();
    fillWorkBenchPars( wbpars );

    od_ostream strm( fnm );
    if ( !strm.isOK() )
	uiMSG().error( uiStrings::sCantOpenOutpFile() );
    else if ( !desc.putTo(strm) )
	uiMSG().error( desc_.errMsg() );
    else
    {
	rv = true;
	descdisp_->setNeedSave( false );
	descdisp_->setDescID( genDescID() );
	synthdatamgrdc_ = synthdatamgr_->dirtyCount();
	const_cast<uiStratLayerModel*>(this)->setWinTitle();
    }

    return rv;
}


bool uiStratLayerModel::loadGenDesc( const MultiID& dbky )
{
    descctio_.setObj( dbky );
    return descctio_.ioobj_ ? doLoadGenDesc() : false;
}


bool uiStratLayerModel::openGenDesc()
{
    if ( !saveGenDescIfNecessary() )
	return false;

    descctio_.ctxt_.forread_ = true;
    uiIOObjSelDlg seldlg( this, descctio_ );
    if ( !seldlg.go() || !seldlg.ioObj() )
	return false;

    descctio_.setObj( seldlg.ioObj()->clone() );

    return doLoadGenDesc();
}


bool uiStratLayerModel::doLoadGenDesc()
{
    const BufferString fnm( descctio_.ioobj_->mainFileName() );
    od_istream strm( fnm );
    if ( !strm.isOK() )
	{ uiMSG().error( uiStrings::sCantOpenInpFile() ); return false; }

    desc_.setEmpty();
    MouseCursorChanger mcch( MouseCursor::Wait );
    const bool rv = desc_.getFrom( strm );
    if ( !rv )
	uiMSG().error( desc_.errMsg() );
    strm.close();

    //Before calculation
    const IOPar& wbpars = desc_.getWorkBenchParams();
    if ( !gentools_->usePar(wbpars) )
	return false;

    afterRetrieve.trigger( &wbpars );
    descdisp_->setNeedSave( false );
    descdisp_->setDescID( genDescID() );
    descdisp_->setEditDesc();
    descdisp_->descHasChanged();

    doGenModels( true );

    setWinTitle();
    raise();
    return true;
}


Strat::LevelID uiStratLayerModel::curLevelID() const
{
    return modtools_->selLevelID();
}


MultiID uiStratLayerModel::genDescID() const
{
    return descctio_.ioobj_? descctio_.ioobj_->key() : MultiID();
}


void uiStratLayerModel::seqSelCB( CallBacker* )
{
    synthdisp_->setSelectedSequence( moddisp_->selectedSequence() );
}


void uiStratLayerModel::modChgCB( CallBacker* )
{
    handleNewModel();
}


void uiStratLayerModel::seqsAddedCB( CallBacker* )
{
    gentools_->setGenWarning(
	    tr("You have added models from file.\nThis will overwrite all") );
    // NOT handleNewModel(), modelChanged will be triggered next
}


bool uiStratLayerModel::updateDispEach( const Strat::LayerModel& mdl )
{
    if ( !modtools_->canSetDispEach() )
	return false;

    const int oldnrseq = nrmodels_;
    if ( oldnrseq == 0 )
	return false;

    const int newnrseq = mdl.size();
    if ( newnrseq == oldnrseq || (newnrseq > oldnrseq && newnrseq < 5*oldnrseq))
	return false;

    const float updratio = float(newnrseq) / float(oldnrseq);
    int newnreach = int( mNINT32( modtools_->dispEach() * updratio ) );
    if ( newnreach < 1 || newnrseq <= 20 )
	newnreach = 1;

    modtools_->setMaxDispEach( newnrseq );
    modtools_->setDispEach( newnreach, false );
    return true;
}


void uiStratLayerModel::genModelsCB( CallBacker* )
{
    const bool wasempty = synthdatamgr_->isEmpty();
    if ( wasempty )
    {
	if ( modtools_->canSetDispEach() )
	    synthdatamgr_->setCalcEach( modtools_->dispEach() );

	synthdatamgr_->addSynthetic( SynthGenParams() );
    }

    doGenModels();
}


bool uiStratLayerModel::doGenModels( bool full )
{
    const int nrmods = gentools_->nrModels();
    if ( nrmods < 1 )
	{ uiMSG().error(tr("Enter a valid number of models")); return false; }

    uiUserShowWait uisw( this, tr("Setting-up new models") );
    descdisp_->prepareDesc();
    descdisp_->setFromEditDesc();
    auto* newmodl = new Strat::LayerModel;
    uisw.readyNow();

    Strat::LayerModelGenerator ex( desc_, *newmodl, nrmods );
    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(ex) || !newmodl->isValid() )
	{ delete newmodl; return false; }

    // transaction succeeded, we move to the new model - period.
    handleNewModel( newmodl, full );
    return true;
}


void uiStratLayerModel::handleNewModel( Strat::LayerModel* newmodel, bool full )
{
    synthdisp_->enableDispUpdate( false ); //Not until its handleNewModel
    const Strat::LayerModel& model = newmodel ? *newmodel : lms_.getCurrent();
    setModelProps( model );
    if ( !full && updateDispEach(model) )
	synthdatamgr_->setCalcEach( modtools_->dispEach() );

    nrmodels_ = model.size();
    if ( newmodel )
    {
	newmodel->prepareUse();
	NotifyStopper nsmoddisp( lms_.curChanged, moddisp_ );
	lms_.setBaseModel( newmodel, full );
    }

    //First the parameters
    const int decimation = synthdatamgr_->calcEach();
    const bool canchange = synthdisp_->viewer()->enableChange( false );
    const IOPar& wbpars = desc_.getWorkBenchParams();
    if ( full )
    {
	NotifyStopper nselprop( synthdatamgr_->elPropSelChanged );
	NotifyStopper nsadd( synthdatamgr_->entryAdded );
	modtools_->usePar( wbpars, false );
	if ( modtools_->dispEach() != decimation )
	    synthdatamgr_->setCalcEach( modtools_->dispEach() );
	else
	    synthdatamgr_->setEmpty();

	synthdisp_->usePar( getSynthPars() );
    }

    synthdatamgr_->addPropertySynthetics(); //May only add the missing ones
    synthdatamgrdc_ = synthdatamgr_->dirtyCount();

    NotifyStopper nslmchg( lms_.modelChanged, this );
    lms_.touch();
    nslmchg.enableNotification();

    synthdisp_->handleModelChange( full );

    uiMultiFlatViewControl* mfvc = synthdisp_->control();
    PtrMan<NotifyStopper> zoomns;
    if ( mfvc )
    {
	zoomns = new NotifyStopper( mfvc->zoomChanged, synthdisp_ );
	mfvc->reInitZooms();
    }

    od_uint32 ctyp = sCast(od_uint32,FlatView::Viewer::All);
    if ( full )
    {
	synthdisp_->useDispPars( wbpars, &ctyp );
	synthdisp_->setSavedViewRect();
    }
    else
	synthdisp_->viewer()->setViewToBoundingBox();

    synthdisp_->viewer()->enableChange( canchange );
    synthdisp_->viewer()->handleChange( ctyp );
}


void uiStratLayerModel::setModelProps( const Strat::LayerModel& lm )
{
    BufferStringSet nms;
    for ( const auto* pr : lm.propertyRefs() )
	if ( !pr->isThickness() )
	    nms.add( pr->name() );

    modtools_->setProps( nms );
    nms.erase();

    const Strat::ContentSet& conts = lm.refTree().contents();
    for ( int idx=0; idx<conts.size(); idx++ )
	nms.add( conts[idx]->name() );
    modtools_->setContentNames( nms );
}


IOPar uiStratLayerModel::getSynthPars() const
{
    const IOPar& wbpars = desc_.getWorkBenchParams();
    IOPar ret;

    const MultiID& descky = desc_.elasticPropSel();
    if ( !descky.isUdf() )
	ret.set( ElasticPropSelection::sKeyElasticPropSel(), descky );

    PtrMan<IOPar> elpropiop =
	wbpars.subselect( ElasticPropSelection::sKeyElasticProp() );
    if ( elpropiop && !elpropiop->isEmpty() )
	ret.mergeComp( *elpropiop.ptr(),
		       ElasticPropSelection::sKeyElasticProp() );

    PtrMan<IOPar> synthiop =
		  wbpars.subselect( StratSynth::DataMgr::sKeySynthetics() );
    if ( synthiop && !synthiop->isEmpty() )
	ret.mergeComp( *synthiop.ptr(), StratSynth::DataMgr::sKeySynthetics() );

    return ret;
}


void uiStratLayerModel::fillWorkBenchPars( IOPar& par ) const
{
    par.setEmpty();
    const_cast<uiStratLayerModel*>(this)->beforeSave.trigger( &par );
    gentools_->fillPar( par );
    modtools_->fillPar( par );
    synthdisp_->fillPar( par );
    par.removeWithKey( ElasticPropSelection::sKeyElasticPropSel() );
}


bool uiStratLayerModel::closeOK()
{
    return saveGenDescIfNecessary();
}
