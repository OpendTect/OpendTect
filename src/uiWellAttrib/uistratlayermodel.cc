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


static float sMaxNrLayToBeDisplayed = 500.0f;
static const char* sKeyDecimation() { return "Decimation"; }

mDefineInstanceCreatedNotifierAccess(uiStratLayerModel)

const char* uiStratLayerModel::sKeyModeler2Use()
{
    return "dTect.Stratigraphic Modeler to use";
}


class uiStratLayerModelLMProvider : public Strat::LayerModelProvider
{ mODTextTranslationClass(uiStratLayerModelLMProvider);
public:

uiStratLayerModelLMProvider()
    : modled_(0)
{
    modl_ = new Strat::LayerModel;
    setEmpty();
}

~uiStratLayerModelLMProvider()
{
    delete modl_;
    delete modled_;
}

Strat::LayerModel& getCurrent()
{
    return *curmodl_;
}

Strat::LayerModel& getEdited( bool yn )
{
    return yn ? *modled_ : *modl_;
}

void setUseEdited( bool yn )
{
    curmodl_ = yn ? modled_ : modl_;
}

void setEmpty()
{
    modl_->setEmpty();
    if ( modled_ ) modled_->setEmpty();
    curmodl_ = modl_;
}

void setBaseModel( Strat::LayerModel* newmdl )
{
    delete modl_;
    modl_ = newmdl;
}

void resetEditing()
{
    delete modled_;
    modled_ = new Strat::LayerModel;
    curmodl_ = modl_;
}

void initEditing()
{
    if ( !modled_ )
	modled_ = new Strat::LayerModel;
    *modled_ = *modl_;
    curmodl_ = modled_;
}

    Strat::LayerModel*		curmodl_;
    Strat::LayerModel*		modl_;
    Strat::LayerModel*		modled_;

};


uiStratLayerModel::uiStratLayerModel( uiParent* p, const char* edtyp, int opt )
    : uiMainWin(p,uiString::empty(),1,true)
    , desc_(*new Strat::LayerSequenceGenDesc(Strat::RT()))
    , elpropsel_(0)
    , descctio_(*mMkCtxtIOObj(StratLayerSequenceGenDesc))
    , analtb_(0)
    , lmp_(*new uiStratLayerModelLMProvider)
    , needtoretrievefrpars_(false)
    , automksynth_(true)
    , nrmodels_(0)
    , moddisp_(0)
    , newModels(this)
    , waveletChanged(this)
    , saveRequired(this)
    , retrieveRequired(this)
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
    modtools_->selPropChg.notify( mCB(this,uiStratLayerModel,selPropChgCB) );
    gentools_ = new uiStratGenDescTools( gengrp );

    synthdisp_ = new uiStratSynthDisp( topgrp, *this, lmp_ );
    moddisp_ = descdisp_->getLayModDisp( *modtools_, lmp_, opt );
    if ( !moddisp_ )
    {
	new uiLabel( this, tr("Start cancelled") );
	close();
	return;
    }

    analtb_ = new uiToolBar( this, tr("Analysis toolbar"), uiToolBar::Right );
    uiToolButtonSetup tbsu( "xplot", tr("Attributes vs model properties"),
			    mCB(this,uiStratLayerModel,xPlotReq) );
    synthdisp_->control()->getToolBar(0)->addButton(
	    "snapshot", uiStrings::sTakeSnapshot(),
	    mCB(this,uiStratLayerModel,snapshotCB));
    synthdisp_->synthsChanged.notify(
		mCB(this,uiStratLayerModel,syntheticsChangedCB) );
    analtb_->addButton( tbsu );
    mDynamicCastGet( uiFlatViewer*,vwr,moddisp_->getViewer());
    if ( vwr )
    {
	synthdisp_->addViewerToControl( *vwr );
	vwr->viewChanged.notify( mCB(this,uiStratLayerModel,lmViewChangedCB) );
	uiToolButton* parsbut = synthdisp_->control()->parsButton( vwr );
	parsbut->setToolTip( tr("Layermodel display properties") );
    }

    modtools_->attach( ensureBelow, moddisp_ );
    gentools_->attach( ensureBelow, descdisp_->outerObj() );

    uiToolBar* helptb =
	new uiToolBar( this, tr("Help toolbar"), uiToolBar::Right );
    uiToolButtonSetup htbsu( "contexthelp", uiStrings::sHelp(),
			     mCB(this,uiStratLayerModel,helpCB) );
    helptb->addButton( htbsu );

    uiParent* horsplitattachhrp = 0;
    if ( !descdisp_->separateDisplay() )
    {
	modtools_->attach( rightOf, gentools_ );
	horsplitattachhrp = this;
    }
    else
    {
	modtools_->attach( rightBorder );
	uiSplitter* vspl = new uiSplitter( this, "Desc-LayModDisp Split" );
	vspl->addGroup( gengrp ); vspl->addGroup( rightgrp );
	horsplitattachhrp = rightgrp;
    }

    uiSplitter* horspl =
	new uiSplitter( horsplitattachhrp, "Synth-LayModDisp Splitter",
			OD::Horizontal);
    horspl->addGroup( topgrp ); horspl->addGroup( botgrp );

    modtools_->dispEachChg.notify( mCB(this,uiStratLayerModel,dispEachChg) );
    modtools_->selLevelChg.notify( mCB(this,uiStratLayerModel,levelChg) );
    modtools_->flattenChg.notify( mCB(this,uiStratLayerModel,flattenChg) );
    modtools_->mkSynthChg.notify( mCB(this,uiStratLayerModel,mkSynthChg) );
    gentools_->openReq.notify( mCB(this,uiStratLayerModel,openGenDescCB) );
    gentools_->saveReq.notify( mCB(this,uiStratLayerModel,saveGenDescCB) );
    gentools_->propEdReq.notify( mCB(this,uiStratLayerModel,manPropsCB) );
    gentools_->genReq.notify( mCB(this,uiStratLayerModel,genModels) );
    gentools_->nrModelsChanged.notify(
	    mCB(this,uiStratLayerModel,nrModelsChangedCB) );
    synthdisp_->wvltChanged.notify( mCB(this,uiStratLayerModel,wvltChg) );
    synthdisp_->viewChanged.notify( mCB(this,uiStratLayerModel,viewChgedCB) );
    synthdisp_->modSelChanged.notify( mCB(this,uiStratLayerModel,modSelChg) );
    synthdisp_->dispParsChanged.notify(
	    mCB(this,uiStratLayerModel,synthDispParsChangedCB) );
    synthdisp_->layerPropSelNeeded.notify(
			    mCB(this,uiStratLayerModel,selElasticPropsCB) );
    synthdisp_->control()->infoChanged.notify(
		    mCB(this,uiStratLayerModel,synthInfoChangedCB) );
    moddisp_->genNewModelNeeded.notify( mCB(this,uiStratLayerModel,genModels) );
    moddisp_->rangeChanged.notify(
			    mCB(this,uiStratLayerModel,modDispRangeChanged));
    moddisp_->infoChanged.notify(
			    mCB(this,uiStratLayerModel,modInfoChangedCB));
    moddisp_->sequenceSelected.notify( mCB(this,uiStratLayerModel,seqSel) );
    moddisp_->modelEdited.notify( mCB(this,uiStratLayerModel,modEd) );
    moddisp_->dispPropChanged.notify(
	    mCB(this,uiStratLayerModel,lmDispParsChangedCB) );

    setWinTitle();
    StratTreeWin().changeLayerModelNumber( true );
    postFinalise().notify( mCB(this,uiStratLayerModel,initWin) );
}


uiStratLayerModel::~uiStratLayerModel()
{
    delete &desc_;
    delete &lmp_;
    delete descctio_.ioobj_; delete &descctio_;
    delete elpropsel_;
    StratTreeWin().changeLayerModelNumber( false );
    UnitOfMeasure::saveCurrentDefaults();
}


void uiStratLayerModel::snapshotCB( CallBacker* )
{
    uiSaveWinImageDlg snapshotdlg( this );
    snapshotdlg.go();
}


void uiStratLayerModel::setWinTitle()
{
    uiString descnm( descctio_.ioobj_ ? toUiString(descctio_.ioobj_->name())
				      : uiStrings::sNew() );
    uiString txt( tr("Layer modeling [%1]").arg(descnm) );
    setCaption( txt );
}


BufferString uiStratLayerModel::levelName() const
{
    return modtools_->selLevelName();
}


StratSynth::DataMgr& uiStratLayerModel::currentStratSynthDM()
{
    return synthdisp_->curDM();
}


const StratSynth::DataMgr& uiStratLayerModel::currentStratSynthDM() const
{
    return const_cast<const uiStratSynthDisp*>(synthdisp_)->curDM();
}


const StratSynth::DataMgr& uiStratLayerModel::normalStratSynthDM() const
{
    return synthdisp_->normalDM();
}


const StratSynth::DataMgr& uiStratLayerModel::editStratSynthDM() const
{
    return synthdisp_->editDM();
}


bool uiStratLayerModel::isEditUsed() const
{
    return synthdisp_->isEditUsed();
}


const PropertyRefSelection& uiStratLayerModel::modelProperties() const
{
    return synthdisp_->modelPropertyRefs();
}


const ObjectSet<const TimeDepthModel>& uiStratLayerModel::d2TModels() const
{
    mDefineStaticLocalObject( ObjectSet<const TimeDepthModel>, empty, );
    const ObjectSet<const TimeDepthModel>* ret = synthdisp_->d2TModels();
    return ret ? *ret : empty;
}


const Wavelet* uiStratLayerModel::wavelet() const
{
    return synthdisp_->getWavelet();
}


void uiStratLayerModel::initWin( CallBacker* cb )
{
    if ( !moddisp_ )
    {
	modtools_->display( false );
	gentools_->display( false );
	OBJDISP()->go( this );
    }
    mTriggerInstanceCreatedNotifier();
}


void uiStratLayerModel::dispEachChg( CallBacker* )
{
    synthdisp_->setDispEach( modtools_->dispEach() );
}


void uiStratLayerModel::mkSynthChg( CallBacker* cb )
{
    automksynth_ = modtools_->mkSynthetics();
    synthdisp_->setAutoUpdate( automksynth_ );
    if ( automksynth_ )
	handleNewModel();
}


void uiStratLayerModel::lmViewChangedCB( CallBacker* )
{
    synthdisp_->updateRelativeViewRect();
}


void uiStratLayerModel::flattenChg( CallBacker* cb )
{
    moddisp_->setFlattened( modtools_->showFlattened() );
    synthdisp_->setSelectedLevel( modtools_->selLevelID() );
    synthdisp_->setFlattened( modtools_->showFlattened() );
}


void uiStratLayerModel::levelChg( CallBacker* cb )
{
    synthdisp_->setSelectedLevel( modtools_->selLevelID() );
    synthdisp_->updateMarkers();
    modtools_->setFlatTBSensitive( moddisp_->canBeFlattened() );
}


void uiStratLayerModel::modSelChg( CallBacker* cb )
{
    mCBCapsuleUnpack(int,modidx,cb);
    moddisp_->selectSequence( modidx -1 );
}


void uiStratLayerModel::viewChgedCB( CallBacker* )
{
    uiWorldRect wr( mUdf(double), 0, 0, 0 );
    synthdisp_->setDisplayZSkip( moddisp_->getDisplayZSkip(), false );
    if ( synthdisp_->getSynthetics().size() )
	wr = synthdisp_->curView( true );
    moddisp_->setZoomBox( wr );
}


bool uiStratLayerModel::checkUnscaledWavelet()
{
    const Wavelet* wvlt = synthdisp_->getWavelet();
    if ( !wvlt )
	return false;
    if ( WaveletMGR().isScaled(synthdisp_->waveletID()) )
	return true;

    uiStringSet opts;
    opts.add( tr("[Start tool]: Start the wavelet scaling dialog") );
    opts.add( tr("[Mark scaled]: The wavelet amplitude is already compatible "
	      "with the seismic data") );
    opts.add( tr("[Ignore]: I will not use scaling-sensitive operations") );
    uiGetChoice dlg( this, opts,
	    tr("The wavelet seems to be unscaled.\n"
	    "For most purposes, you will need a scaled wavelet.\n"), true );
    dlg.setHelpKey( mODHelpKey(mStratLayerModelcheckUnscaledWaveletHelpID) );
    dlg.go(); const int choice = dlg.choice();
    if ( choice < 0 )
	return false;
    else if ( choice == 2 )
	return true;
    else if ( choice == 1 )
    {
	WaveletMGR().setScalingInfo( synthdisp_->waveletID(), 0 );
	return true;
    }

    return synthdisp_->haveUserScaleWavelet();
}


void uiStratLayerModel::xPlotReq( CallBacker* )
{
    if ( !checkUnscaledWavelet() )
	return;

    if ( !synthdisp_->getSynthetics().size() ) return;

    uiStratSynthCrossplot dlg( this, layerModel(),synthdisp_->getSynthetics());
    if ( !dlg.errMsg().isEmpty() )
	{ uiMSG().error( dlg.errMsg() ); return; }
    BufferString lvlnm = modtools_->selLevelName();
    if ( !lvlnm.isEmpty() )
	dlg.setRefLevel( lvlnm );
    dlg.go();
}


void uiStratLayerModel::wvltChg( CallBacker* cb )
{
    viewChgedCB( cb );
    waveletChanged.trigger();
}


void uiStratLayerModel::modDispRangeChanged( CallBacker* )
{
    synthdisp_->setZDataRange( moddisp_->getViewer()->getSelDataRange(false) ,
			       true );
}


void uiStratLayerModel::manPropsCB( CallBacker* )
{
    descdisp_->selProps();
}


void uiStratLayerModel::selElasticPropsCB( CallBacker* )
{
    if ( !elpropsel_ )
	elpropsel_ = new ElasticPropSelection;
    if ( selElasticProps( *elpropsel_ ) )
	doGenModels( false, false );
}


bool uiStratLayerModel::selElasticProps( ElasticPropSelection& elsel )
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


bool uiStratLayerModel::saveGenDescIfNecessary( bool allowcncl ) const
{
    if ( !descdisp_->needSave() )
	return true;

    while ( true )
    {
	const int res = uiMSG().askSave(tr("Generation description not saved.\n"
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
	{ uiMSG().error( uiStrings::phrCannotOpenInpFile() ); return false; }

    deleteAndZeroPtr( elpropsel_ );
    deepErase( desc_ );
    uiUserShowWait usw( this, uiStrings::sReadingData() );
    const bool rv = desc_.getFrom( strm );
    if ( !rv )
	uiMSG().error(desc_.errMsg());
    strm.close();

    //Before calculation
    if ( !gentools_->usePar( desc_.getWorkBenchParams() ) )
	return false;

    if ( !rv )
	return false;

    usw.setMessage( uiStrings::sUpdatingDisplay() );
    moddisp_->clearDispPars();
    moddisp_->retrievePars();
    descdisp_->setNeedSave( false );
    lmp_.setEmpty();

    descdisp_->setEditDesc();
    descdisp_->descHasChanged();

    synthdisp_->resetRelativeViewRect();
    synthdisp_->setForceUpdate( true );
    BufferString edtyp;
    descctio_.ctxt_.toselect_.require_.get( sKey::Type(), edtyp );
    if ( descdisp_->separateDisplay() )
    {
	needtoretrievefrpars_ = true;
	doGenModels( true, false );
	//Set when everything is in place.
    }
    else
    {
	CBCapsule<IOPar*> caps( &desc_.getWorkBenchParams(),
				const_cast<uiStratLayerModel*>(this) );
	const_cast<uiStratLayerModel*>(this)->retrieveRequired.trigger( &caps );
    }


    mDynamicCastGet(uiMultiFlatViewControl*,mfvc,synthdisp_->control());
    if ( mfvc ) mfvc->reInitZooms();
    synthdisp_->setSavedViewRect();
    synthdisp_->setSelectedLevel( modtools_->selLevelID() );
    setWinTitle();
    raise();
    return true;
}


DBKey uiStratLayerModel::genDescID() const
{
    DBKey ret;
    if ( descctio_.ioobj_ )
	ret = descctio_.ioobj_->key();
    return ret;
}


void uiStratLayerModel::seqSel( CallBacker* )
{
    synthdisp_->setSelectedTrace( moddisp_->selectedSequence() );
}


void uiStratLayerModel::lmDispParsChangedCB( CallBacker* )
{
    LMPropSpecificDispPars lmpropdp;
    if ( !moddisp_->getCurPropDispPars(lmpropdp) )
	return;
    BufferString lmpropsdnm( lmpropdp.propnm_.buf() );
    if ( isEditUsed() )
	lmpropsdnm += StratSynth::DataMgr::sKeyFRNameSuffix();
    const BufferString propnm( "[", lmpropsdnm.buf(), "]" );
    RefMan<SyntheticData> sd = synthdisp_->getSyntheticData( propnm );
    if ( !sd ) return;
    *sd->dispPars().vdmapsetup_ = *lmpropdp.mappersetup_;
    sd->dispPars().colseqname_ = lmpropdp.colseqname_;
    sd->dispPars().overlap_ = lmpropdp.overlap_;
    RefMan<SyntheticData> vdsd = synthdisp_->getCurrentSyntheticData( false );
    RefMan<SyntheticData> wvasd = synthdisp_->getCurrentSyntheticData( true );
    if ( (vdsd && propnm == vdsd->name()) ||
	 (wvasd && propnm == wvasd->name()) )
	synthdisp_->reDisplayPostStackSynthetic( false );
}


static bool getCleanSyntheticName( BufferString& sdnm )
{
    if ( sdnm.isEmpty() ) return false;
    char* cleansdnm = sdnm.getCStr();
    if ( cleansdnm[0] != '[' ) return false; //Is not StratPropSyntheticData
    cleansdnm++;
    int idx = 0;
    while ( cleansdnm[idx] != ']' && idx<sdnm.size() )
	idx++;

    cleansdnm[idx] = '\0';
    sdnm = cleansdnm;
    return true;
}


void uiStratLayerModel::synthDispParsChangedCB( CallBacker* )
{
    RefMan<SyntheticData> vdsd = synthdisp_->getCurrentSyntheticData( false );
    if ( !vdsd ) return;

    BufferString sdnm( vdsd->name() );
    if ( !getCleanSyntheticName(sdnm) )
	return;

    if ( isEditUsed() )
	sdnm.remove( StratSynth::DataMgr::sKeyFRNameSuffix() );

    LMPropSpecificDispPars vddisppars( sdnm );
    vddisppars.mappersetup_ = vdsd->dispPars().vdmapsetup_;
    vddisppars.colseqname_ = vdsd->dispPars().colseqname_;
    vddisppars.overlap_ = vdsd->dispPars().overlap_;
    if ( !moddisp_->setPropDispPars(vddisppars) )
	return;

    BufferString selpropnm = modtools_->selProp();
    if ( !selpropnm.isEmpty() && selpropnm == sdnm )
	moddisp_->modelChanged();
}


void uiStratLayerModel::modEd( CallBacker* )
{
    synthdisp_->setForceUpdate( nrmodels_!=lmp_.getCurrent().size() );
    handleNewModel();
}


void uiStratLayerModel::calcAndSetDisplayEach( bool overridedispeach )
{
    int decimation = mUdf(int);
    if ( desc_.getWorkBenchParams().get(sKeyDecimation(),decimation) &&
	 !overridedispeach )
	return;

    const int nrmods = gentools_->nrModels();
    const int nrseq = desc_.size();
    decimation =
	mCast(int,floor(mCast(float,nrmods*nrseq)/sMaxNrLayToBeDisplayed)) + 1;
    desc_.getWorkBenchParams().set( sKeyDecimation(), decimation );
}


void uiStratLayerModel::setNrModels( int nrmodels )
{
    gentools_->setNrModels( nrmodels );
    gentools_->fillPar( desc_.getWorkBenchParams() );
}


int uiStratLayerModel::nrModels() const
{
    int nrmodels = gentools_->getNrModelsFromPar( desc_.getWorkBenchParams() );
    if ( nrmodels<0 )
	nrmodels = gentools_->nrModels();
    return nrmodels;
}


void uiStratLayerModel::nrModelsChangedCB( CallBacker* )
{
    gentools_->fillPar( desc_.getWorkBenchParams() );
}


void uiStratLayerModel::genModels( CallBacker* cb )
{
    const bool isgo = cb==gentools_;
    BufferString edtyp;
    descctio_.ctxt_.toselect_.require_.get( sKey::Type(), edtyp );
    doGenModels( isgo, isgo && descdisp_->separateDisplay() );
}

void uiStratLayerModel::doGenModels( bool forceupdsynth, bool overridedispeach )
{
    const int nrmods = gentools_->nrModels();
    if ( nrmods < 1 )
	{ uiMSG().error(tr("Enter a valid number of models")); return; }

    uiUserShowWait usw( this, tr("Generating Models") );

    descdisp_->prepareDesc();
    descdisp_->setFromEditDesc();
    Strat::LayerModel* newmodl = new Strat::LayerModel;
    newmodl->propertyRefs() = desc_.propSelection();
    newmodl->setElasticPropSel( lmp_.getCurrent().elasticPropSel() );

    Strat::LayerModelGenerator ex( desc_, *newmodl, nrmods );
    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(ex) || !newmodl->isValid() )
	{ delete newmodl; return; }

    // transaction succeeded, we move to the new model - period.

    calcAndSetDisplayEach( overridedispeach );
    if ( forceupdsynth  ) // i.e. 'Go' is used
	synthdisp_->setForceUpdate( true );

    lmp_.setBaseModel( newmodl );
    uiWorldRect prevrelzoomwr = synthdisp_->getRelativeViewRect();
    Interval<float> reldepthrg = moddisp_->relDepthZoneOfInterest();
    if ( !reldepthrg.isUdf() )
    {
	prevrelzoomwr.setTop( reldepthrg.start );
	prevrelzoomwr.setBottom( reldepthrg.stop );
	moddisp_->reSetRelDepthZoneOfInterest();
    }

    handleNewModel();
    mDynamicCastGet(uiMultiFlatViewControl*,mfvc,synthdisp_->control());
    if ( mfvc ) mfvc->reInitZooms();
    synthdisp_->setZoomView( prevrelzoomwr );
}



void uiStratLayerModel::handleNewModel()
{
    lmp_.resetEditing();
    synthdisp_->setUseEdited( false );

    //First the parameters
    setModelProps();
    setElasticProps();
    useSyntheticsPars( desc_.getWorkBenchParams() );
    useDisplayPars( desc_.getWorkBenchParams() );
    if ( needtoretrievefrpars_ )
    {
	CBCapsule<IOPar*> caps( &desc_.getWorkBenchParams(),
				const_cast<uiStratLayerModel*>(this) );
	const_cast<uiStratLayerModel*>(this)->retrieveRequired.trigger( &caps );
	needtoretrievefrpars_ = false;
    }

    //Then the model display (uiStrat)
    moddisp_->setFlattened( modtools_->showFlattened(), false );
    moddisp_->modelUpdate();

    //Finally the synthetics
    synthdisp_->setDisplayZSkip( moddisp_->getDisplayZSkip(), false );
    synthdisp_->setFlattened( modtools_->showFlattened(), false );
    synthdisp_->updateMarkers();
    nrmodels_ = layerModel().size();

    newModels.trigger();

    synthdisp_->setForceUpdate( false );
}


void uiStratLayerModel::setModelProps()
{
    BufferStringSet nms;
    const Strat::LayerModel& lm = lmp_.getCurrent();
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
	if ( !selElasticProps( *elpropsel_ ) )
	    return;
    }

    lmp_.getEdited(true).setElasticPropSel( *elpropsel_ );
    lmp_.getEdited(false).setElasticPropSel( *elpropsel_ );
}


bool uiStratLayerModel::closeOK()
{
    return saveGenDescIfNecessary();
}


void uiStratLayerModel::displayFRText( bool yn, bool isbrine )
{
    synthdisp_->displayFRText( yn, isbrine );
    moddisp_->displayFRText( yn, isbrine );
}


void uiStratLayerModel::displayFRResult( bool usefr, bool parschanged,
					 bool isbrine )
{
    lmp_.setUseEdited( usefr );
    synthdisp_->setUseEdited( usefr );
    if ( parschanged )
    {
	synthdisp_->setForceUpdate( true );
	useSyntheticsPars( desc_.getWorkBenchParams() );
    }
    synthdisp_->showFRResults();
    synthdisp_->updateMarkers();
    moddisp_->modelChanged();
    displayFRText( true, isbrine );
    synthdisp_->setForceUpdate( false );
}


void uiStratLayerModel::resetFluidRepl()
{
    lmp_.setUseEdited( false );
    synthdisp_->setUseEdited( false );
    displayFRText( false );
}


void uiStratLayerModel::prepareFluidRepl()
{
    lmp_.initEditing();
    Strat::LayerModel& edlm = lmp_.getEdited( true );
    const bool hasswave =
	edlm.propertyRefs().find(PropertyRef::standardSVelStr()) >= 0 ||
	edlm.propertyRefs().find(PropertyRef::standardSVelAliasStr()) >= 0;
    if ( !hasswave )
	edlm.propertyRefs() += new PropertyRef( PropertyRef::standardSVelStr(),
						PropertyRef::Vel );
}


const Strat::LayerModel& uiStratLayerModel::layerModelOriginal() const
{
    return lmp_.getEdited( false );
}


Strat::LayerModel& uiStratLayerModel::layerModelOriginal()
{
    return lmp_.getEdited( false );
}


const Strat::LayerModel& uiStratLayerModel::layerModelEdited() const
{
    return lmp_.getEdited( true );
}


Strat::LayerModel& uiStratLayerModel::layerModelEdited()
{
    return lmp_.getEdited( true );
}


const Strat::LayerModel& uiStratLayerModel::layerModel() const
{
    return lmp_.getCurrent();
}


Strat::LayerModel& uiStratLayerModel::layerModel()
{
    return lmp_.getCurrent();
}


bool uiStratLayerModel::useDisplayPars( const IOPar& par )
{
    return modtools_->usePar( par );
}



void uiStratLayerModel::fillWorkBenchPars( IOPar& par ) const
{
    par.setEmpty();
    CBCapsule<IOPar*> caps( &par, const_cast<uiStratLayerModel*>(this) );
    const_cast<uiStratLayerModel*>(this)->saveRequired.trigger( &caps );
    gentools_->fillPar( par );
    if ( elpropsel_ )
	elpropsel_->fillPar( par );
    moddisp_->savePars();
    fillDisplayPars( par );
    fillSyntheticsPars( par );
}


bool uiStratLayerModel::useSyntheticsPars( const IOPar& par )
{
    if ( !synthdisp_->prepareElasticModel() )
	return false;
    return synthdisp_->usePar( par );
}


void uiStratLayerModel::fillSyntheticsPars( IOPar& par ) const
{
    synthdisp_->fillPar( par, false );
}


void uiStratLayerModel::fillDisplayPars( IOPar& par ) const
{
    modtools_->fillPar( par );
}


void uiStratLayerModel::helpCB( CallBacker* )
{ HelpProvider::provideHelp(
    HelpKey(mODHelpKey(mSingleLayerGeneratorEdHelpID) ) ); }


void uiStratLayerModel::syntheticsChangedCB( CallBacker* )
{
    synthdisp_->fillPar( desc_.getWorkBenchParams() );
    descdisp_->setNeedSave( true );
}


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


void uiStratLayerModel::selPropChgCB( CallBacker* )
{
    const int listidx = modtools_->selPropIdx();
    descdisp_->setDispProp( listidx<0 ? -1 : listidx+1 );
}
