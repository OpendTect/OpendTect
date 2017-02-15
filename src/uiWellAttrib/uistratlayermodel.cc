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
#include "stratsynth.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "waveletmanager.h"

#include "uichecklist.h"
#include "uielasticpropsel.h"
#include "uifileinput.h"
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
    : uiMainWin(p,uiString::emptyString(),1,true)
    , desc_(*new Strat::LayerSequenceGenDesc(Strat::RT()))
    , elpropsel_(0)
    , descctio_(*mMkCtxtIOObj(StratLayerSequenceGenDesc))
    , analtb_(0)
    , lmp_(*new uiStratLayerModelLMProvider)
    , needtoretrievefrpars_(false)
    , automksynth_(true)
    , nrmodels_(0)
    , moddisp_(0)
    , modeldlg_(0)
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
    seqdisp_ = uiLayerSequenceGenDesc::factory().create( edtyp, gengrp, desc_ );
    if ( !seqdisp_ )
	seqdisp_ = new uiBasicLayerSequenceGenDesc( gengrp, desc_ );

    uiGroup* topgrp; uiGroup* botgrp; uiGroup* rightgrp=0;
    if ( seqdisp_->separateDisplay() )
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

    synthdisp_ = new uiStratSynthDisp( topgrp, lmp_ );
    moddisp_ = seqdisp_->getLayModDisp( *modtools_, lmp_, opt );
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
    gentools_->attach( ensureBelow, seqdisp_->outerObj() );

    uiToolBar* helptb =
	new uiToolBar( this, tr("Help toolbar"), uiToolBar::Right );
    uiToolButtonSetup htbsu( "contexthelp", uiStrings::sHelp(),
			     mCB(this,uiStratLayerModel,helpCB) );
    helptb->addButton( htbsu );

    uiParent* horsplitattachhrp = 0;
    if ( !seqdisp_->separateDisplay() )
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
    synthdisp_->wvltChanged.notify( mCB(this,uiStratLayerModel,wvltChg) );
    synthdisp_->viewChanged.notify( mCB(this,uiStratLayerModel,viewChgedCB) );
    synthdisp_->modSelChanged.notify( mCB(this,uiStratLayerModel,modSelChg) );
    synthdisp_->dispParsChanged.notify(
	    mCB(this,uiStratLayerModel,synthDispParsChangedCB) );
    synthdisp_->layerPropSelNeeded.notify(
			    mCB(this,uiStratLayerModel,selElasticPropsCB) );
    synthdisp_->control()->infoChanged.notify(
	    mCB(this,uiStratLayerModel,infoChanged) );
    moddisp_->genNewModelNeeded.notify( mCB(this,uiStratLayerModel,genModels) );
    moddisp_->rangeChanged.notify(
			    mCB(this,uiStratLayerModel,modDispRangeChanged));
    moddisp_->infoChanged.notify(
			    mCB(this,uiStratLayerModel,infoChanged));
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
    if ( modeldlg_ )
	modeldlg_->close();
    delete modeldlg_;
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
    uiString descnm( descctio_.ioobj_ ? descctio_.ioobj_->uiName()
				      : uiStrings::sNew() );
    uiString txt( tr("Layer modeling [%1]").arg(descnm) );
    setCaption( txt );
}


const char* uiStratLayerModel::levelName() const
{
    const char* nm = modtools_->selLevel();
    return !nm || !*nm || (*nm == '-' && *(nm+1) == '-') ? 0 : nm;
}


StratSynth& uiStratLayerModel::currentStratSynth()
{
    return synthdisp_->curSS();
}


const StratSynth& uiStratLayerModel::currentStratSynth() const
{
    return const_cast<const uiStratSynthDisp*>(synthdisp_)->curSS();
}


const StratSynth& uiStratLayerModel::normalStratSynth() const
{
    return synthdisp_->normalSS();
}


const StratSynth& uiStratLayerModel::editStratSynth() const
{
    return synthdisp_->editSS();
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


bool uiStratLayerModel::canShowFlattened() const
{
    TypeSet<float> zlvls = moddisp_->flattenLevelDepths();
    for ( int idx=0; idx<zlvls.size(); idx++ )
	if ( !mIsUdf(zlvls[idx]) ) return true;
    return false;
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
    synthdisp_->setFlattened( modtools_->showFlattened() );
    synthdisp_->setFlattenLvlNm( modtools_->getFlattenLvlNm() );
}


void uiStratLayerModel::levelChg( CallBacker* cb )
{
    synthdisp_->setDispMrkrs( modtools_->getSelLvlNmSet(),
						moddisp_->getLevelDepths() );
    modtools_->setFlatTBSensitive( canShowFlattened() );
    if ( !canShowFlattened() && moddisp_->isFlattened() )
    {
	modtools_->setShowFlattened( false );
	moddisp_->setFlattened( false );
	synthdisp_->setFlattened( false, true );
    }
    else if ( modtools_->showFlattened() )
    {
	moddisp_->setFlattened( modtools_->showFlattened() );
	synthdisp_->setFlattened( modtools_->showFlattened(), true );
    }
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
    const char* lvlnm = modtools_->selLevel();
    if ( lvlnm && *lvlnm ) dlg.setRefLevel( lvlnm );
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
    seqdisp_->selProps();
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
	    seqdisp_->setNeedSave( true );
	    seqdisp_->setEditDesc();
	}

	elsel.fillPar( desc_.getWorkBenchParams() );
	return true;
    }
    return false;
}


bool uiStratLayerModel::saveGenDescIfNecessary( bool allowcncl ) const
{
    if ( !seqdisp_->needSave() )
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

    const BufferString fnm( descctio_.ioobj_->fullUserExpr(false) );
    bool rv = false;

    MouseCursorChanger mcch( MouseCursor::Wait );

    fillWorkBenchPars( desc_.getWorkBenchParams() );

    od_ostream strm( fnm );
    if ( !strm.isOK() )
	uiMSG().error( uiStrings::sCantOpenOutpFile() );
    else if ( !desc_.putTo(strm) )
	uiMSG().error(desc_.errMsg());
    else
    {
	rv = true;
	seqdisp_->setNeedSave( false );
	const_cast<uiStratLayerModel*>(this)->setWinTitle();
    }

    return rv;
}


bool uiStratLayerModel::openGenDesc()
{
    if ( !saveGenDescIfNecessary() )
	return false;

    descctio_.ctxt_.forread_ = true;
    modeldlg_ = new uiIOObjSelDlg( this, descctio_ );


    if ( !modeldlg_->go() || !modeldlg_->ioObj() )
	return false;

    descctio_.setObj( modeldlg_->ioObj()->clone() );

    const BufferString fnm( descctio_.ioobj_->fullUserExpr(true) );
    od_istream strm( fnm );
    if ( !strm.isOK() )
	{ uiMSG().error( uiStrings::sCantOpenInpFile() ); return false; }

    delete elpropsel_; elpropsel_ = 0;
    deepErase( desc_ );
    MouseCursorChanger mcch( MouseCursor::Wait );
    bool rv = desc_.getFrom( strm );
    if ( !rv )
	uiMSG().error(desc_.errMsg());
    strm.close();

    //Before calculation
    if ( !gentools_->usePar( desc_.getWorkBenchParams() ) )
	return false;

    if ( !rv )
	return false;

    moddisp_->clearDispPars();
    seqdisp_->setNeedSave( false );
    lmp_.setEmpty();

    seqdisp_->setEditDesc();
    seqdisp_->descHasChanged();

    synthdisp_->resetRelativeViewRect();
    synthdisp_->setForceUpdate( true );
    BufferString edtyp;
    descctio_.ctxt_.toselect_.require_.get( sKey::Type(), edtyp );
    if ( seqdisp_->separateDisplay() )
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
    setWinTitle();
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
	lmpropsdnm += StratSynth::sKeyFRNameSuffix();
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
	sdnm.remove( StratSynth::sKeyFRNameSuffix() );

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


void uiStratLayerModel::genModels( CallBacker* cb )
{
    const bool isgo = cb==gentools_;
    BufferString edtyp;
    descctio_.ctxt_.toselect_.require_.get( sKey::Type(), edtyp );
    doGenModels( isgo, isgo && seqdisp_->separateDisplay() );
}

void uiStratLayerModel::doGenModels( bool forceupdsynth, bool overridedispeach )
{
    const int nrmods = gentools_->nrModels();
    if ( nrmods < 1 )
	{ uiMSG().error(tr("Enter a valid number of models")); return; }

    MouseCursorChanger mcs( MouseCursor::Wait );

    seqdisp_->prepareDesc();
    seqdisp_->setFromEditDesc();
    Strat::LayerModel* newmodl = new Strat::LayerModel;
    newmodl->propertyRefs() = desc_.propSelection();
    newmodl->setElasticPropSel( lmp_.getCurrent().elasticPropSel() );
    mcs.restore();

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
    moddisp_->setFluidReplOn( false );

    setModelProps();
    setElasticProps();
    useSyntheticsPars( desc_.getWorkBenchParams() );
    useDisplayPars( desc_.getWorkBenchParams() );
    synthdisp_->setDisplayZSkip( moddisp_->getDisplayZSkip(), true );
    synthdisp_->setFlattened( modtools_->showFlattened(), true );
    moddisp_->setFlattened( modtools_->showFlattened(), true );
    synthdisp_->setDispMrkrs( modtools_->getSelLvlNmSet(),
						moddisp_->getLevelDepths() );

    if ( needtoretrievefrpars_ )
    {
	CBCapsule<IOPar*> caps( &desc_.getWorkBenchParams(),
				const_cast<uiStratLayerModel*>(this) );
	const_cast<uiStratLayerModel*>(this)->retrieveRequired.trigger( &caps );
	needtoretrievefrpars_ = false;
    }

    nrmodels_ = layerModel().size();
    newModels.trigger();

    synthdisp_->setForceUpdate( false );
}


void uiStratLayerModel::setModelProps()
{
    BufferStringSet nms;
    const Strat::LayerModel& lm = lmp_.getCurrent();
    for ( int idx=1; idx<lm.propertyRefs().size(); idx++ )
	nms.add( lm.propertyRefs()[idx]->name() );
    modtools_->setProps( nms );
    nms.erase(); Strat::LVLS().getNames( nms );
    modtools_->setLevelNames( nms );
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
	    uiMSG().message(errmsg);
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


void uiStratLayerModel::displayFRResult( bool usefr, bool parschanged,
					 bool fwd )
{
    lmp_.setUseEdited( usefr );
    mostlyfilledwithbrine_ = !fwd;
    if ( !usefr )
	mostlyfilledwithbrine_ = !mostlyfilledwithbrine_;

    synthdisp_->setUseEdited( usefr );
    if ( parschanged )
    {
	synthdisp_->setForceUpdate( true );
	useSyntheticsPars( desc_.getWorkBenchParams() );
    }
    synthdisp_->showFRResults();
    synthdisp_->setDispMrkrs( modtools_->getSelLvlNmSet(),
						moddisp_->getLevelDepths() );
    moddisp_->setBrineFilled( fwd );
    moddisp_->setFluidReplOn( usefr );
    moddisp_->modelChanged();
    synthdisp_->setForceUpdate( false );
}


void uiStratLayerModel::resetFluidRepl()
{
    lmp_.setUseEdited( false );
    synthdisp_->setUseEdited( false );
    moddisp_->setFluidReplOn( false );
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
    seqdisp_->setNeedSave( true );
}


void uiStratLayerModel::infoChanged( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(IOPar,pars,caller,cb);
    mDynamicCastGet(uiStratLayerModelDisp*,moddisp,caller);
    if ( !moddisp )
    {
	uiString msg;
	synthdisp_->makeInfoMsg( msg, pars );
	statusBar()->message( msg );
    }
    else
    {
	uiString msg;
	for ( int idx=0; idx<pars.size(); idx++ )
	{
	    msg.append( toUiString("%1 : %2 ;").arg(pars.getKey(idx))
					     .arg(pars.getValue(idx)) );
	}
	statusBar()->message( msg );
    }
}


void uiStratLayerModel::selPropChgCB( CallBacker* )
{
    seqdisp_->setDispProp( modtools_->selPropIdx() );
}
