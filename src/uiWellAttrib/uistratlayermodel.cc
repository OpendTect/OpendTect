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
#include "ioman.h"
#include "objdisposer.h"
#include "od_helpids.h"
#include "od_iostream.h"
#include "seistrc.h"
#include "separstr.h"
#include "settings.h"
#include "stratlayseqgendesc.h"
#include "stratlaygen.h"
#include "stratlayermodel.h"
#include "stratlaymodgen.h"
#include "strattransl.h"
#include "stratreftree.h"
#include "stratsynth.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "wavelet.h"

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


mDefineInstanceCreatedNotifierAccess(uiStratLayerModel)

const char* uiStratLayerModel::sKeyModeler2Use()
{
    return "dTect.Stratigraphic Modeler to use";
}


class uiStratLayerModelManager : public CallBacker
{ mODTextTranslationClass(uiStratLayerModelManager)
public:

uiStratLayerModelManager()
{
    mAttachCB( IOM().surveyToBeChanged, uiStratLayerModelManager::survChg );
    mAttachCB( IOM().applicationClosing, uiStratLayerModelManager::survChg );
}


~uiStratLayerModelManager()
{
    detachAllNotifiers();
}


void survChg( CallBacker* )
{
    if ( dlg_ )
	dlg_->saveGenDescIfNecessary( false );
    deleteAndZeroPtr( dlg_ );
}

void winClose( CallBacker* )
{
    uiStratTreeWin::makeEditable( true );
    dlg_ = nullptr;
}

void startCB( CallBacker* cb )
{
    if ( haveExistingDlg() )
	return;
    const uiStringSet& usrnms =
			uiLayerSequenceGenDesc::factory().getUserNames();
    const BufferStringSet& nms = uiLayerSequenceGenDesc::factory().getNames();
    mDynamicCastGet(uiToolButton*,tb,cb)
    if ( Strat::RT().isEmpty() || nms.isEmpty() )
	{ pErrMsg("Pre-condition not met"); return; }

    uiParent* par = tb ? tb->parent() : &StratTreeWin();
    const char* settres = Settings::common().find(
				uiStratLayerModel::sKeyModeler2Use());
    BufferString modnm( settres );
    int defmodnr = -1;
    bool givechoice = nms.size() > 1;
    if ( modnm.isEmpty() )
	modnm = *nms.last();
    else
    {
	FileMultiString fms( modnm );
	modnm = fms[0];
	defmodnr = nms.indexOf( modnm.buf() );
	if ( defmodnr < 0 )
	    modnm.setEmpty();
	else
	{
	    const bool alwayswant = fms.size() > 1 && *fms[1] == 'A';
	    givechoice = givechoice && !alwayswant;
	}
    }

    if ( givechoice )
    {
	uiSelectFromList::Setup sflsu( tr("Select modeling type"), usrnms );
	sflsu.current( defmodnr < 0 ? nms.size()-1 : defmodnr );
	uiSelectFromList dlg( par, sflsu );
	uiCheckList* defpol = new uiCheckList( &dlg, uiCheckList::Chain1st,
						OD::Horizontal );
	defpol->addItem( tr("Set as default") )
	       .addItem( tr("Always use this type") );
	defpol->setChecked( 0, defmodnr >= 0 );
	defpol->attach( centeredBelow, dlg.selFld() );
	if ( !dlg.go() ) return;

	const int sel = dlg.selection();
	if ( sel < 0 ) return;
	const BufferString newmodnm = nms.get( sel );
	int indic = defpol->isChecked(0) ? (defpol->isChecked(1) ? 2 : 1) : 0;
	bool needwrite = true;
	if ( indic == 0 )
	{
	    if ( defmodnr < 0 )
		needwrite = false;
	    else
		Settings::common().removeWithKey(
				    uiStratLayerModel::sKeyModeler2Use() );
	}
	else
	{
	    if ( indic == 2 || defmodnr < 0 || modnm != newmodnm )
	    {
		Settings::common().set( uiStratLayerModel::sKeyModeler2Use(),
			BufferString(newmodnm, indic == 2 ? "`Always" : "") );
	    }
	    else if ( defmodnr >= 0 )
		needwrite = false;
	}

	if ( needwrite )
	    Settings::common().write( false );
	modnm = newmodnm;
    }
    doLayerModel( par, modnm, 0 );
}

uiStratLayerModel* getDlg()
{ return dlg_; }


bool haveExistingDlg()
{
    if ( dlg_ )
    {
	uiMSG().error(tr("Please exit your other layer modeling window first"));
	dlg_->raise();
	return true;
    }
    return false;
}

void doLayerModel( uiParent* p, const char* modnm, int opt )
{
    if ( haveExistingDlg() || Strat::RT().isEmpty() )
	return;

    dlg_ = new uiStratLayerModel( p, modnm, opt );
    if ( !dlg_->moddisp_ )
	deleteAndZeroPtr( dlg_ );
    else
    {
	uiStratTreeWin::makeEditable( false );
	mAttachCB( dlg_->windowClosed, uiStratLayerModelManager::winClose );
	dlg_->go();
    }
}

void addToTreeWin()
{
    auto* su = new uiToolButtonSetup( "stratlayermodeling",
			    tr("Start layer/synthetics modeling"),
			    mCB(this,uiStratLayerModelManager,startCB) );
    uiStratTreeWin::addTool( su );
}

    uiStratLayerModel*	dlg_ = nullptr;

};

static uiStratLayerModelManager& uislm_manager()
{
    mDefineStaticLocalObject( uiStratLayerModelManager, theinst, );
    return theinst;
}

void uiStratLayerModel::initClass()
{
    uislm_manager().addToTreeWin();
}


void uiStratLayerModel::doBasicLayerModel()
{
    doBasicLayerModel( &StratTreeWin() );
}


void uiStratLayerModel::doBasicLayerModel( uiParent* p )
{
    doLayerModel( p, uiBasicLayerSequenceGenDesc::typeStr() );
}


void uiStratLayerModel::doLayerModel( const char* modnm, int opt )
{
    doLayerModel( &StratTreeWin(), modnm, opt );
}


void uiStratLayerModel::doLayerModel( uiParent* p, const char* modnm, int opt )
{
    if ( Strat::RT().isEmpty() )
	Strat::loadDefaultTree();

    if ( Strat::RT().isEmpty() )
	StratTreeWin().popUp();
    else
	uislm_manager().doLayerModel( p, modnm, opt );
}



uiStratLayerModel* uiStratLayerModel::getUILayerModel()
{
    return uislm_manager().getDlg();
}


//uiStratLayerModel
uiStratLayerModel::uiStratLayerModel( uiParent* p, const char* edtyp, int opt )
    : uiMainWin(p,uiString::emptyString(),1,true)
    , desc_(*new Strat::LayerSequenceGenDesc(Strat::RT()))
    , descctio_(*mMkCtxtIOObj(StratLayerSequenceGenDesc))
    , lmp_(*new Strat::LayerModelProviderImpl)
    , newModels(this)
    , saveRequired(this)
    , retrieveRequired(this)
{
    setDeleteOnClose( true );

    if ( !edtyp || !*edtyp )
	edtyp = uiBasicLayerSequenceGenDesc::typeStr();

    descctio_.ctxt_.toselect_.require_.set( sKey::Type(), edtyp );

    auto* gengrp = new uiGroup( this, "Gen group" );
    seqdisp_ = uiLayerSequenceGenDesc::factory().create( edtyp, gengrp, desc_ );
    if ( !seqdisp_ )
	seqdisp_ = new uiBasicLayerSequenceGenDesc( gengrp, desc_ );

    uiGroup* topgrp; uiGroup* botgrp; uiGroup* rightgrp = nullptr;
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
    mAttachCB( modtools_->selPropChg, uiStratLayerModel::selPropChgCB );
    gentools_ = new uiStratGenDescTools( gengrp );

    synthdisp_ = new uiStratSynthDisp( topgrp, lmp_ );
    synthdisp_->set( *this );
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
				mCB(this,uiStratLayerModel,snapshotCB) );
    mAttachCB( synthdisp_->synthsChanged,
	       uiStratLayerModel::syntheticsChangedCB );
    analtb_->addButton( tbsu );
    mDynamicCastGet( uiFlatViewer*,vwr,moddisp_->getViewer());
    if ( vwr )
    {
	synthdisp_->addViewerToControl( *vwr );
	mAttachCB( vwr->viewChanged, uiStratLayerModel::lmViewChangedCB );
	synthdisp_->control()->setParsButToolTip( *vwr,
				    tr("Layermodel display properties") );
    }

    modtools_->attach( ensureBelow, moddisp_ );
    gentools_->attach( ensureBelow, seqdisp_->outerObj() );

    auto* helptb = new uiToolBar( this, tr("Help toolbar"), uiToolBar::Right );
    uiToolButtonSetup htbsu( "contexthelp", uiStrings::sHelp(),
			     mCB(this,uiStratLayerModel,helpCB) );
    helptb->addButton( htbsu );

    uiSplitter* hspl;
    if ( !seqdisp_->separateDisplay() )
    {
	modtools_->attach( rightOf, gentools_ );
	hspl = new uiSplitter( this, "Hor splitter", false );
    }
    else
    {
	modtools_->attach( rightBorder );
	auto* vspl = new uiSplitter( this, "Vert splitter", true );
	vspl->addGroup( gengrp ); vspl->addGroup( rightgrp );
	hspl = new uiSplitter( rightgrp, "Hor splitter", false );
    }

    hspl->addGroup( topgrp );
    hspl->addGroup( botgrp );

    mAttachCB( modtools_->dispEachChg, uiStratLayerModel::dispEachChg );
    mAttachCB( modtools_->selLevelChg, uiStratLayerModel::levelChg );
    mAttachCB( modtools_->flattenChg, uiStratLayerModel::flattenChg );
    mAttachCB( modtools_->mkSynthChg, uiStratLayerModel::mkSynthChg );
    mAttachCB( gentools_->openReq, uiStratLayerModel::openGenDescCB );
    mAttachCB( gentools_->saveReq, uiStratLayerModel::saveGenDescCB );
    mAttachCB( gentools_->propEdReq, uiStratLayerModel::manPropsCB );
    mAttachCB( gentools_->genReq, uiStratLayerModel::genModels );
    mAttachCB( gentools_->nrModelsChanged,uiStratLayerModel::nrModelsChangedCB);
    mAttachCB( synthdisp_->viewChanged, uiStratLayerModel::viewChgedCB );
    mAttachCB( synthdisp_->modSelChanged, uiStratLayerModel::modSelChg );
    mAttachCB( synthdisp_->dispParsChanged,
	       uiStratLayerModel::synthDispParsChangedCB );
    mAttachCB( synthdisp_->layerPropSelNeeded,
	       uiStratLayerModel::selElasticPropsCB );
    mAttachCB( synthdisp_->control()->infoChanged,
	       uiStratLayerModel::infoChanged );
    mAttachCB( moddisp_->genNewModelNeeded, uiStratLayerModel::genModels );
    mAttachCB(  moddisp_->rangeChanged, uiStratLayerModel::modDispRangeChanged);
    mAttachCB( moddisp_->infoChanged, uiStratLayerModel::infoChanged );
    mAttachCB( moddisp_->sequenceSelected, uiStratLayerModel::seqSel );
    mAttachCB( moddisp_->modelEdited, uiStratLayerModel::modEd );
    mAttachCB( moddisp_->dispPropChanged,
	       uiStratLayerModel::lmDispParsChangedCB );

    setWinTitle();
    StratTreeWin().changeLayerModelNumber( true );
    mAttachCB( postFinalize(), uiStratLayerModel::initWin );
}


uiStratLayerModel::~uiStratLayerModel()
{
    detachAllNotifiers();
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
    uiString descnm( descctio_.ioobj_ ? descctio_.ioobj_->uiName()
				      : uiStrings::sNew() );
    uiString txt( tr("Layer modeling [%1]").arg(descnm) );
    setCaption( txt );
}


const char* uiStratLayerModel::levelName() const
{
    const char* nm = modtools_->selLevel();
    return !nm || !*nm || (*nm == '-' && *(nm+1) == '-') ? nullptr : nm;
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


void uiStratLayerModel::initWin( CallBacker* )
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
    TypeSet<float> zlvls = moddisp_->levelDepths();
    for ( int idx=0; idx<zlvls.size(); idx++ )
	if ( !mIsUdf(zlvls[idx]) ) return true;
    return false;
}


void uiStratLayerModel::mkSynthChg( CallBacker* )
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


void uiStratLayerModel::flattenChg( CallBacker* )
{
    moddisp_->setFlattened( modtools_->showFlattened() );
    synthdisp_->setFlattened( modtools_->showFlattened() );
}


void uiStratLayerModel::levelChg( CallBacker* )
{
    synthdisp_->setDispMrkrs( modtools_->selLevel(), moddisp_->levelDepths(),
		    modtools_->selLevelColor() );
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
    BufferStringSet wvltnms;
    for ( int idx=0; idx<currentStratSynth().nrSynthetics(); idx++ )
    {
	const SyntheticData* sd = currentStratSynth().getSyntheticByIdx( idx );
	if ( !sd )
	    continue;

	const SynthGenParams& sgp = sd->getGenParams();
	if ( !sgp.isRawOutput() )
	    continue;

	wvltnms.addIfNew( sgp.getWaveletNm() );
    }

    TypeSet<MultiID> wvltids;
    for ( int idx=wvltnms.size()-1; idx>=0; idx-- )
    {
	PtrMan<IOObj> ioobj = Wavelet::getIOObj( wvltnms.get(idx).buf() );
	if ( !ioobj )
	    return false;

	PtrMan<Wavelet> wvlt = Wavelet::get( ioobj );
	if ( !wvlt )
	    return false;

	const MultiID wvltid = ioobj->key();
	if ( Wavelet::isScaled(wvltid) )
	    wvltids += wvltid;
    }

    for ( const auto& wvltid : wvltids )
    {
	BufferStringSet opts;
	opts.add( "[Start tool]: Start the wavelet scaling dialog" );
	opts.add( "[Mark scaled]: The wavelet amplitude is already compatible "
		  "with the seismic data" );
	opts.add( "[Ignore]: I will not use scaling-sensitive operations" );
	uiGetChoice dlg( this, opts,
		tr("The wavelet seems to be unscaled.\n"
		"For most purposes, you will need a scaled wavelet.\n"), true );
	dlg.setHelpKey( mODHelpKey(mStratLayerModelcheckUnscaledWaveletHelpID));
	dlg.go();
	const int choice = dlg.choice();
	if ( choice < 0 )
	    return false;
	else if ( choice == 1 )
	    Wavelet::markScaled( wvltid );
	else if ( choice == 2 )
	    continue;
    }

    return synthdisp_->haveUserScaleWavelet();
}


void uiStratLayerModel::xPlotReq( CallBacker* )
{
    if ( !checkUnscaledWavelet() )
	return;

    if ( !synthdisp_->getSynthetics().size() )
	return;

    uiStratSynthCrossplot xplotdlg( this, layerModel(),
				    synthdisp_->getSynthetics() );
    if ( !xplotdlg.errMsg().isEmpty() )
	{ uiMSG().error( xplotdlg.errMsg() ); return; }

    const char* lvlnm = modtools_->selLevel();
    if ( lvlnm && *lvlnm )
	xplotdlg.setRefLevel( lvlnm );

    xplotdlg.show();
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
    const Strat::LayerSequenceGenDesc& desc = seqdisp_->currentDesc();
    IOPar& wbpars =
	const_cast<Strat::LayerSequenceGenDesc&>(desc).getWorkBenchParams();
    fillWorkBenchPars( wbpars );

    od_ostream strm( fnm );
    if ( !strm.isOK() )
	uiMSG().error( uiStrings::sCantOpenOutpFile() );
    else if ( !desc.putTo(strm) )
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
    uiIOObjSelDlg dlg( this, descctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;
    descctio_.setObj( dlg.ioObj()->clone() );

    const BufferString fnm( descctio_.ioobj_->fullUserExpr(true) );
    od_istream strm( fnm );
    if ( !strm.isOK() )
	{ uiMSG().error( uiStrings::sCantOpenInpFile() ); return false; }

    deleteAndZeroPtr( elpropsel_ );
    deepErase( desc_ );
    MouseCursorChanger mcch( MouseCursor::Wait );
    const bool rv = desc_.getFrom( strm );
    if ( !rv )
	uiMSG().error(desc_.errMsg());
    strm.close();

    //Before calculation
    if ( !gentools_->usePar( desc_.getWorkBenchParams() ) )
	return false;

    if ( !rv )
	return false;

    moddisp_->clearDispPars();
    moddisp_->retrievePars();
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
    raise();
    return true;
}


MultiID uiStratLayerModel::genDescID() const
{
    MultiID ret;
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
    SynthFVSpecificDispPars* sddisp = synthdisp_->dispPars( propnm );
    if ( !sddisp )
	return;

    sddisp->vdmapper_ = lmpropdp.mapper_;
    sddisp->ctab_ = lmpropdp.ctab_;
    sddisp->overlap_ = lmpropdp.overlap_;
    ConstRefMan<SyntheticData> vdsd =
			synthdisp_->getCurrentSyntheticData( false );
    ConstRefMan<SyntheticData> wvasd =
			synthdisp_->getCurrentSyntheticData( true );
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
    unsigned int idx = 0;
    while ( cleansdnm[idx] != ']' && idx<sdnm.size() )
	idx++;

    cleansdnm[idx] = '\0';
    sdnm = cleansdnm;
    return true;
}


void uiStratLayerModel::synthDispParsChangedCB( CallBacker* )
{
    ConstRefMan<SyntheticData> vdsd =
				synthdisp_->getCurrentSyntheticData( false );
    if ( !vdsd )
	return;

    BufferString sdnm( vdsd->name() );
    if ( !getCleanSyntheticName(sdnm) )
	return;

    if ( isEditUsed() )
	sdnm.remove( StratSynth::sKeyFRNameSuffix() );

    LMPropSpecificDispPars vddisppars( sdnm );
    vddisppars.mapper_ = vdsd->dispPars().vdmapper_;
    vddisppars.ctab_ = vdsd->dispPars().ctab_;
    vddisppars.overlap_ = vdsd->dispPars().overlap_;
    if ( !moddisp_->setPropDispPars(vddisppars) )
	return;

    BufferString selpropnm = modtools_->selProp();
    if ( !selpropnm.isEmpty() && selpropnm == sdnm )
	moddisp_->modelChanged();
}


void uiStratLayerModel::modEd( CallBacker* )
{
    synthdisp_->setForceUpdate( nrModels()!=lmp_.getCurrent().size() );
    handleNewModel();
}


void uiStratLayerModel::calcAndSetDisplayEach( bool overridedispeach )
{
    int decimation = modtools_->dispEach();
    if ( !overridedispeach )
	return;

    const int nrmods = nrModels();
    decimation = sCast(int,Math::Ceil(nrmods/100.f));
    modtools_->setDispEach( decimation );
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
    doGenModels( isgo, isgo && seqdisp_->separateDisplay() );
}


void uiStratLayerModel::doGenModels( bool forceupdsynth, bool overridedispeach )
{
    const int nrmods = nrModels();
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
    synthdisp_->setDispMrkrs( modtools_->selLevel(), moddisp_->levelDepths(),
					modtools_->selLevelColor() );
    const bool canshowflattened = canShowFlattened();
    synthdisp_->setSnapLevelSensitive( canshowflattened );
    modtools_->setFlatTBSensitive( canshowflattened );

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
    nms.erase(); const Strat::LevelSet& lvls = Strat::LVLS();
    for ( int idx=0; idx<lvls.size(); idx++ )
	nms.add( lvls.levels()[idx]->name() );
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
	const MultiID descky = desc_.elasticPropSel();
	if ( IOM().isUsable(descky ) && IOM().implExists(descky) )
	    elpropsel_ = ElasticPropSelection::getByDBKey( descky );

	bool haserrors = !elpropsel_;
	if ( !elpropsel_ )
	{
	    elpropsel_ = new ElasticPropSelection( true, desc_.propSelection());
	    haserrors = !elpropsel_->usePar( desc_.getWorkBenchParams() );
	}

	haserrors = haserrors || (elpropsel_ && !elpropsel_->isOK());
	if ( haserrors )
	{
	    if ( elpropsel_ )
		uiMSG().warning( elpropsel_->errMsg() );
	    deleteAndZeroPtr( elpropsel_ );
	    return;
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
	if ( !selElasticProps(*elpropsel_) )
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
    synthdisp_->setDispMrkrs( modtools_->selLevel(), moddisp_->levelDepths(),
			      modtools_->selLevelColor() );
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
    ePROPS().ensureHasElasticProps( true );
    if ( !edlm.propertyRefs().getByMnemonic(Mnemonic::defSVEL()) )
	edlm.propertyRefs().add( PROPS().getByMnemonic( Mnemonic::defSVEL() ) );
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
    seqdisp_->setNeedSave( true );
}


void uiStratLayerModel::infoChanged( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(IOPar,pars,caller,cb);
    mDynamicCastGet(uiStratLayerModelDisp*,moddisp,caller);
    if ( !moddisp )
    {
	BufferString msg;
	synthdisp_->makeInfoMsg( msg, pars );
	statusBar()->message( mToUiStringTodo(msg.buf()) );
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
