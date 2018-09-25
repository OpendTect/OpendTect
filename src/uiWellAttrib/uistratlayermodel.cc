/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uistratlayermodel.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "elasticpropsel.h"
#include "envvars.h"
#include "executor.h"
#include "ioobj.h"
#include "dbman.h"
#include "objdisposer.h"
#include "od_helpids.h"
#include "od_iostream.h"
#include "seistrc.h"
#include "separstr.h"
#include "settings.h"
#include "stratlayseqgendesc.h"
#include "stratlayermodel.h"
#include "stratlaygen.h"
#include "strattransl.h"
#include "stratlaymodgen.h"
#include "stratreftree.h"
#include "stratsynthdatamgr.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "waveletmanager.h"

#include "uichecklist.h"
#include "uielasticpropsel.h"
#include "uifilesel.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewwin.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uiioobjseldlg.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimultiflatviewcontrol.h"
#include "uimsg.h"
#include "uisaveimagedlg.h"
#include "uiselsimple.h"
#include "uispinbox.h"
#include "uisplitter.h"
#include "uistatusbar.h"
#include "uistratbasiclayseqgendesc.h"
#include "uistratlaymodtools.h"
#include "uistratsimplelaymoddisp.h"
#include "uistratsynthcrossplot.h"
#include "uistratsynthdisp.h"
#include "uistrattreewin.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"


static int cMaxNrLayToBeDisplayed = 100000;

mDefineInstanceCreatedNotifierAccess(uiStratLayerModel)

const char* uiStratLayerModel::sKeyModeler2Use()
{ return "dTect.Stratigraphic Modeler to use"; }



uiStratLayerModel::uiStratLayerModel( uiParent* p, const char* edtyp, int opt )
    : uiMainWin(p,uiString::empty(),1,true)
    , desc_(*new Strat::LayerSequenceGenDesc(Strat::RT()))
    , descctio_(*mMkCtxtIOObj(StratLayerSequenceGenDesc))
    , lms_(*new Strat::LayerModelSuite)
    , newModel(this)
    , beforeSave(this)
    , afterRetrieve(this)
{
    setDeleteOnClose( true );

    if ( !edtyp || !*edtyp )
	edtyp = uiBasicLayerSequenceGenDesc::typeStr();
    descctio_.ctxt_.toselect_.require_.set( sKey::Type(), edtyp );

    uiGroup* gengrp = new uiGroup( this, "Gen group" );
    descdisp_ = uiLayerSequenceGenDesc::factory().create( edtyp, gengrp, desc_);
    if ( !descdisp_ )
	descdisp_ = new uiBasicLayerSequenceGenDesc( gengrp, desc_ );

    uiGroup* topgrp; uiGroup* botgrp; uiGroup* rightgrp=0;
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

    synthdatamgr_ = new SynthDataMgr( lms_ );
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
	modtools_->addLithFld();
    }

    synthdisp_ = new uiStratSynthDisp( topgrp, *synthdatamgr_, *modtools_,
				       moddisp_->initialSize() );

    analtb_ = new uiToolBar( this, tr("Analysis toolbar"), uiToolBar::Right );
    uiToolButtonSetup tbsu( "xplot", tr("Attributes vs model properties"),
			    mCB(this,uiStratLayerModel,xPlotReq) );
    synthdisp_->control()->getToolBar(0)->addButton(
			    "snapshot", uiStrings::sTakeSnapshot(),
			    mCB(this,uiStratLayerModel,snapshotCB));
    analtb_->addButton( tbsu );
    mDynamicCastGet( uiFlatViewer*, moddispvwr, moddisp_->getViewer() );
    if ( moddispvwr )
    {
	synthdisp_->addViewerToControl( *moddispvwr );
	auto* parsbut = synthdisp_->control()->parsButton( moddispvwr );
	parsbut->setToolTip( tr("Layermodel display properties") );
    }

    modtools_->attach( ensureBelow, moddisp_ );
    gentools_->attach( ensureBelow, descdisp_->outerObj() );

    uiToolBar* helptb =
	new uiToolBar( this, tr("Help toolbar"), uiToolBar::Right );
    uiToolButtonSetup htbsu( "contexthelp", uiStrings::sHelp(),
			     mCB(this,uiStratLayerModel,helpCB) );
    helptb->addButton( htbsu );

    uiParent* hsplitparent;
    if ( !descdisp_->separateDisplay() )
    {
	modtools_->attach( rightOf, gentools_ );
	hsplitparent = this;
    }
    else
    {
	modtools_->attach( rightBorder );
	uiSplitter* vspl = new uiSplitter( this, "Desc-LayModDisp Split" );
	vspl->addGroup( gengrp ); vspl->addGroup( rightgrp );
	hsplitparent = rightgrp;
    }

    auto* horspl = new uiSplitter( hsplitparent, "Synth-LayModDisp Splitter",
				   OD::Horizontal );
    horspl->addGroup( topgrp ); horspl->addGroup( botgrp );

    setWinTitle();
    StratTreeWin().changeLayerModelNumber( true );

    postFinalise().notify( mCB(this,uiStratLayerModel,initWin) );
    mTriggerInstanceCreatedNotifier();
}


uiStratLayerModel::~uiStratLayerModel()
{
    delete &desc_;
    delete &lms_;
    delete elpropsel_;
    descctio_.setObj(0); delete &descctio_;
    StratTreeWin().changeLayerModelNumber( false );
    UnitOfMeasure::saveCurrentDefaults();
    delete synthdatamgr_;
}


void uiStratLayerModel::initWin( CallBacker* cb )
{
    if ( !moddisp_ )
    {
	modtools_->display( false );
	gentools_->display( false );
	OBJDISP()->go( this );
	return;
    }

    const CallBack genmodscb( mCB(this,uiStratLayerModel,genModelsCB) );

    gentools_->openReq.notify( mCB(this,uiStratLayerModel,openGenDescCB) );
    gentools_->saveReq.notify( mCB(this,uiStratLayerModel,saveGenDescCB) );
    gentools_->propEdReq.notify( mCB(this,uiStratLayerModel,manPropsCB) );
    gentools_->genReq.notify( genmodscb );

    synthdisp_->elasticPropsSelReq.notify(
			    mCB(this,uiStratLayerModel,selectElasticPropsCB) );
    synthdisp_->control()->infoChanged.notify(
		    mCB(this,uiStratLayerModel,synthInfoChangedCB) );

    moddisp_->genNewModelNeeded.notify( genmodscb );
    moddisp_->infoChanged.notify(
			    mCB(this,uiStratLayerModel,modInfoChangedCB));
    moddisp_->sequenceSelected.notify( mCB(this,uiStratLayerModel,seqSelCB) );
    moddisp_->modelEdited.notify( mCB(this,uiStratLayerModel,modEdCB) );
    moddisp_->modelsAdded.notify( mCB(this,uiStratLayerModel,modelsAddedCB) );
}


void uiStratLayerModel::setWinTitle()
{
    uiString descnm( descctio_.ioobj_ ? toUiString(descctio_.ioobj_->name())
				      : uiStrings::sNew() );
    uiString txt( tr("Layer modeling [%1]").arg(descnm) );
    setCaption( txt );
}


Strat::LayerModel& uiStratLayerModel::layerModel()
{ return lms_.getCurrent(); }
const Strat::LayerModel& uiStratLayerModel::layerModel() const
{ return lms_.getCurrent(); }


void uiStratLayerModel::snapshotCB( CallBacker* )
{
    uiSaveWinImageDlg snapshotdlg( this );
    snapshotdlg.go();
}


void uiStratLayerModel::helpCB( CallBacker* )
{ HelpProvider::provideHelp(
    HelpKey(mODHelpKey(mSingleLayerGeneratorEdHelpID) ) ); }


void uiStratLayerModel::synthInfoChangedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( IOPar, iop, cb );
    uiString todisp;
    synthdisp_->makeInfoMsg( todisp, iop );
    statusBar()->message( todisp );
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
    uiStratSynthCrossplot dlg( this, *synthdatamgr_ );
    dlg.setRefLevel( curLevelID() );
    dlg.go();
}


void uiStratLayerModel::manPropsCB( CallBacker* )
{
    descdisp_->selProps();
}


void uiStratLayerModel::selectElasticPropsCB( CallBacker* )
{
    if ( !elpropsel_ )
	elpropsel_ = new ElasticPropSelection;
    if ( selectElasticProps( *elpropsel_ ) )
	doGenModels();
}


const TimeDepthModelSet& uiStratLayerModel::d2TModels() const
{
    return synthdatamgr_->d2TModels();
}


bool uiStratLayerModel::selectElasticProps( ElasticPropSelection& elsel )
{
    uiElasticPropSelDlg dlg( this, desc_.propSelection(), elsel );
    if ( dlg.go() )
    {
	if ( dlg.propSaved() )
	{
	    desc_.setElasticPropSel( dlg.storedKey() );
	    descdisp_->setNeedSave( true );
	    descdisp_->setEditDesc();
	}

	elsel.fillPar( desc_.getWorkBenchParams() );
	return true;
    }
    return false;
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
	    uiMSG().error( tr("Sorry, you cannot cancel right now."
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

    const BufferString fnm( descctio_.ioobj_->mainFileName() );
    bool rv = false;

    uiUserShowWait usw( this, uiStrings::sSavingData() );

    fillWorkBenchPars( desc_.getWorkBenchParams() );

    od_ostream strm( fnm );
    if ( !strm.isOK() )
	uiMSG().error( uiStrings::phrCannotOpenOutpFile() );
    else if ( !desc_.putTo(strm) )
	uiMSG().error(desc_.errMsg());
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


bool uiStratLayerModel::loadGenDesc( const DBKey& dbky )
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
	{ uiMSG().error( uiStrings::phrCannotOpenForRead(fnm) ); return false; }

    deleteAndZeroPtr( elpropsel_ );
    deepErase( desc_ );
    uiUserShowWait usw( this, uiStrings::sReadingData() );
    const bool rv = desc_.getFrom( strm );
    if ( !rv )
	uiMSG().error( desc_.errMsg() );
    strm.close();

    usw.setMessage( uiStrings::sUpdatingDisplay() );
    gentools_->usePar( desc_.getWorkBenchParams() );
    descdisp_->setNeedSave( false );
    descdisp_->setDescID( genDescID() );
    descdisp_->setEditDesc();
    descdisp_->descHasChanged();

    if ( doGenModels() )
    {
	mDynamicCastGet(uiMultiFlatViewControl*,mfvc,synthdisp_->control());
	if ( mfvc )
	    mfvc->reInitZooms();
    }

    synthdatamgrdc_ = synthdatamgr_->dirtyCount();

    setWinTitle();
    raise();
    return true;
}


Strat::Level::ID uiStratLayerModel::curLevelID() const
{
    return modtools_->selLevelID();
}


DBKey uiStratLayerModel::genDescID() const
{
    return descctio_.ioobj_? descctio_.ioobj_->key() : DBKey();
}


void uiStratLayerModel::seqSelCB( CallBacker* )
{
    synthdisp_->setSelectedSequence( moddisp_->selectedSequence() );
}


void uiStratLayerModel::modEdCB( CallBacker* )
{
    handleNewModel();
}


void uiStratLayerModel::modelsAddedCB( CallBacker* )
{
    gentools_->setGenWarning(
	    tr("You have added models from file.\nThis will overwrite all") );
    handleNewModel();
}


void uiStratLayerModel::updateDispEach( const LayerModel& mdl )
{
    const int nrlayers = mdl.nrLayers();
    const int mindispeach = (nrlayers / cMaxNrLayToBeDisplayed) + 1;
    const int curdispeach = modtools_->dispEach();
    if ( curdispeach < mindispeach )
	modtools_->setDispEach( mindispeach, false );
}


void uiStratLayerModel::genModelsCB( CallBacker* cb )
{
    doGenModels();
}


bool uiStratLayerModel::doGenModels()
{
    const int nrmods = gentools_->nrModels();
    if ( nrmods < 1 )
	{ uiMSG().error(tr("Enter a valid number of models")); return false; }

    uiUserShowWait usw( this, tr("Generating Models") );

    descdisp_->prepareDesc();
    descdisp_->setFromEditDesc();
    Strat::LayerModel* newmodl = new Strat::LayerModel;
    newmodl->propertyRefs() = desc_.propSelection();
    newmodl->setElasticPropSel( lms_.getCurrent().elasticPropSel() );

    Strat::LayerModelGenerator ex( desc_, *newmodl, nrmods );
    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(ex) || !newmodl->isValid() )
	{ delete newmodl; return false; }

    // transaction succeeded, we move to the new model - period.
    handleNewModel( newmodl );
    return true;
}


void uiStratLayerModel::handleNewModel( LayerModel* newmodl )
{
    if ( !newmodl )
	lms_.clearEditedData();
    else
    {
	newmodl->prepareUse();
	updateDispEach( *newmodl );
	lms_.setBaseModel( newmodl );
    }

    //First the parameters
    setModelProps();
    setElasticProps();
    auto& wbpars = desc_.getWorkBenchParams();
    synthdatamgr_->usePar( wbpars );
    synthdatamgrdc_ = synthdatamgr_->dirtyCount();
    modtools_->usePar( wbpars );
    afterRetrieve.trigger( &wbpars );

    //Then the model display and synthetics
    moddisp_->modelChanged();
    synthdisp_->modelChanged();

    mDynamicCastGet(uiMultiFlatViewControl*,mfvc,synthdisp_->control());
    if ( mfvc )
	mfvc->reInitZooms();

    newModel.trigger();
}


void uiStratLayerModel::setModelProps()
{
    BufferStringSet nms;
    const Strat::LayerModel& lm = lms_.getCurrent();
    for ( int idx=1; idx<lm.propertyRefs().size(); idx++ )
	nms.add( lm.propertyRefs()[idx]->name() ); // idx==0 is thickness
    modtools_->setProps( nms );
    nms.erase();
    const Strat::ContentSet& conts = lm.refTree().contents();
    for ( int idx=0; idx<conts.size(); idx++ )
	nms.add( conts[idx]->name() );
    modtools_->setContentNames( nms );
}


void uiStratLayerModel::setElasticProps()
{
    if ( !elpropsel_ )
    {
	elpropsel_ = new ElasticPropSelection;
	if ( !elpropsel_->usePar(desc_.getWorkBenchParams()) )
	{
	    delete elpropsel_;
	    elpropsel_ = 0;
	}
    }

    if ( !elpropsel_ )
    {
	elpropsel_ = ElasticPropSelection::getByDBKey( desc_.elasticPropSel() );
	if ( !elpropsel_ )
	{
	    elpropsel_ = new ElasticPropSelection;
	    ElasticPropGuess( desc_.propSelection(), *elpropsel_ );
	}
    }

    uiString errmsg;
    if ( !elpropsel_->isValidInput(&errmsg) )
    {
	if ( !errmsg.isEmpty() )
	{
	    errmsg = tr("%1\nPlease define a new value.").arg(errmsg);
	    uiMSG().error( errmsg );
	}
	if ( !selectElasticProps( *elpropsel_ ) )
	    return;
    }

    lms_.get( 0 ).setElasticPropSel( *elpropsel_ );
    if ( lms_.size() > 1 )
	lms_.get( 1 ).setElasticPropSel( *elpropsel_ );
}


bool uiStratLayerModel::checkUnscaledWavelets()
{
    //const auto helpky = mStratLayerModelcheckUnscaledWaveletHelpID;

    DBKeySet unscaledids;
    for ( const auto& gp : synthdatamgr_->genParams() )
	if ( !gp.isStratProp() && !WaveletMGR().isScaled(gp.wvltid_) )
	    unscaledids.add( gp.wvltid_ );
    if ( !unscaledids.isEmpty() )
    {
	uiString msg;
	if ( unscaledids.size() < 2 )
	    msg.set( tr("Wavelet '%1' is not scaled. Continue?").arg(
			    nameOf(unscaledids.first()) ) );
	else
	    msg.set( tr("Not all the wavelets seem to be scaled. Continue?") );
	msg.append( toUiString("\n\nTODO pop up the scaling dialog instead") );
	if ( !uiMSG().askGoOn(msg) )
	    return false;

    }
    return true;
}


void uiStratLayerModel::fillWorkBenchPars( IOPar& par ) const
{
    par.setEmpty();
    const_cast<uiStratLayerModel*>(this)->beforeSave.trigger( &par );
    synthdatamgr_->fillPar( par );
    gentools_->fillPar( par );
    if ( elpropsel_ )
	elpropsel_->fillPar( par );
    modtools_->fillPar( par );
}


bool uiStratLayerModel::closeOK()
{
    return saveGenDescIfNecessary();
}
