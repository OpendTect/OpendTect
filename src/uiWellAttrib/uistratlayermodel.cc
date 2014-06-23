/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratlayermodel.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "elasticpropsel.h"
#include "envvars.h"
#include "executor.h"
#include "ioobj.h"
#include "ioman.h"
#include "pixmap.h"
#include "od_iostream.h"
#include "settings.h"
#include "separstr.h"
#include "stratlayseqgendesc.h"
#include "stratlayermodel.h"
#include "strattransl.h"
#include "stratlaymodgen.h"
#include "stratreftree.h"
#include "stratsynth.h"
#include "survinfo.h"
#include "wavelet.h"
#include "unitofmeasure.h"

#include "uielasticpropsel.h"
#include "uiobjdisposer.h"
#include "uiioobjsel.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uitoolbar.h"
#include "uimsg.h"
#include "uiselsimple.h"
#include "uisplitter.h"
#include "uiflatviewer.h"
#include "uiflatviewwin.h"
#include "uiflatviewstdcontrol.h"
#include "uimultiflatviewcontrol.h"
#include "uisaveimagedlg.h"
#include "uispinbox.h"
#include "uistratbasiclayseqgendesc.h"
#include "uistratsimplelaymoddisp.h"
#include "uistratsynthdisp.h"
#include "uistratsynthcrossplot.h"
#include "uistratlaymodtools.h"
#include "uistrattreewin.h"
#include "uistatusbar.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"
#include "uichecklist.h"

#include "seistrc.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiStratLayerModel)

const char* uiStratLayerModel::sKeyModeler2Use()
{
    return "dTect.Stratigraphic Modeler to use";
}


class uiStratLayerModelManager : public CallBacker
{
public:

uiStratLayerModelManager()
    : dlg_(0)
{
    IOM().surveyToBeChanged.notify(mCB(this,uiStratLayerModelManager,survChg));
    IOM().applicationClosing.notify(mCB(this,uiStratLayerModelManager,survChg));
}


void survChg( CallBacker* )
{
    if ( dlg_ )
	dlg_->saveGenDescIfNecessary( false );
    delete dlg_; dlg_ = 0;
}

void winClose( CallBacker* )
{
    dlg_ = 0;
}

void startCB( CallBacker* cb )
{
    if ( haveExistingDlg() )
	return;
    const TypeSet<uiString>& usrnms =
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
	uiSelectFromList::Setup sflsu( "Select modeling type", usrnms );
	sflsu.current( defmodnr < 0 ? nms.size()-1 : defmodnr );
	uiSelectFromList dlg( par, sflsu );
	uiCheckList* defpol = new uiCheckList( &dlg, uiCheckList::Chain1st,
						OD::Horizontal );
	defpol->addItem( "Set as default" ).addItem( "Always use this type" );
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

bool haveExistingDlg()
{
    if ( dlg_ )
    {
	uiMSG().error( "Please exit your other layer modeling window first" );
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
    dlg_->windowClosed.notify(mCB(this,uiStratLayerModelManager,winClose));
    dlg_->go();
}

void addToTreeWin()
{
    uiToolButtonSetup* su = new uiToolButtonSetup( "stratlayermodeling",
			    "Start layer/synthetics modeling",
			    mCB(this,uiStratLayerModelManager,startCB) );
    uiStratTreeWin::addTool( su );
}

    uiStratLayerModel*	dlg_;

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
    doLayerModel( uiBasicLayerSequenceGenDesc::typeStr() );
}


void uiStratLayerModel::doLayerModel( const char* modnm, int opt )
{
    if ( Strat::RT().isEmpty() )
	StratTreeWin().popUp();
    else
	uislm_manager().doLayerModel( &StratTreeWin(), modnm, opt );
}



class uiStratLayerModelLMProvider : public Strat::LayerModelProvider
{
public:

uiStratLayerModelLMProvider()
{
    modl_ = new Strat::LayerModel;
    setEmpty();
}

~uiStratLayerModelLMProvider()
{
    delete modl_;
}

Strat::LayerModel& getCurrent()
{
    return *curmodl_;
}

Strat::LayerModel& getEdited( bool yn )
{
    return yn ? modled_ : *modl_;
}

void setUseEdited( bool yn )
{
    curmodl_ = yn ? &modled_ : modl_;
}

void setEmpty()
{
    modl_->setEmpty();
    modled_.setEmpty();
    curmodl_ = modl_;
}

void setBaseModel( Strat::LayerModel* newmdl )
{
    delete modl_;
    modl_ = newmdl;
}

void resetEditing()
{
    modled_ = *modl_;
    curmodl_ = modl_;
}

void initEditing()
{
    modled_ = *modl_;
    curmodl_ = &modled_;
}

    Strat::LayerModel*		curmodl_;
    Strat::LayerModel*		modl_;
    Strat::LayerModel		modled_;

};


uiStratLayerModel::uiStratLayerModel( uiParent* p, const char* edtyp, int opt )
    : uiMainWin(p,uiStrings::sEmptyString(),1,true)
    , desc_(*new Strat::LayerSequenceGenDesc(Strat::RT()))
    , elpropsel_(0)
    , descctio_(*mMkCtxtIOObj(StratLayerSequenceGenDesc))
    , analtb_(0)
    , lmp_(*new uiStratLayerModelLMProvider)
    , needtoretrievefrpars_(false)
    , automksynth_(true)
    , nrmodels_(0)
    , newModels(this)
    , levelChanged(this)
    , waveletChanged(this)
    , saveRequired(this)
    , retrieveRequired(this)
{
    setDeleteOnClose( true );

    if ( !edtyp || !*edtyp )
	edtyp = uiBasicLayerSequenceGenDesc::typeStr();
    descctio_.ctxt.toselect.require_.set( sKey::Type(), edtyp );

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
    gentools_ = new uiStratGenDescTools( gengrp );
    moddisp_ = seqdisp_->getLayModDisp( *modtools_, lmp_, opt );
    if ( !moddisp_ )
	return;

    synthdisp_ = new uiStratSynthDisp( topgrp, lmp_ );
    analtb_ = new uiToolBar( this, "Analysis toolbar", uiToolBar::Right );
    uiToolButtonSetup tbsu( "xplot", "Attributes vs model properties",
			    mCB(this,uiStratLayerModel,xPlotReq) );
    synthdisp_->control()->getToolBar(0)->addButton(
	    "snapshot", "Get snapshot", mCB(this,uiStratLayerModel,snapshotCB));
    synthdisp_->synthsChanged.notify(
		mCB(this,uiStratLayerModel,syntheticsChangedCB) );
    analtb_->addButton( tbsu );
    mDynamicCastGet( uiFlatViewer*,vwr,moddisp_->getViewer());
    if ( vwr ) synthdisp_->addViewerToControl( *vwr );

    modtools_->attach( ensureBelow, moddisp_ );
    gentools_->attach( ensureBelow, seqdisp_->outerObj() );

    uiToolBar* helptb = new uiToolBar( this, "Help toolbar", uiToolBar::Right );
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
	uiSplitter* vspl = new uiSplitter( this, "Vert splitter", true );
	vspl->addGroup( gengrp ); vspl->addGroup( rightgrp );
	hspl = new uiSplitter( rightgrp, "Hor splitter", false );
    }
    hspl->addGroup( topgrp ); hspl->addGroup( botgrp );

    modtools_->dispEachChg.notify( mCB(this,uiStratLayerModel,dispEachChg) );
    modtools_->selLevelChg.notify( mCB(this,uiStratLayerModel,levelChg) );
    modtools_->flattenChg.notify( mCB(this,uiStratLayerModel,levelChg) );
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
    delete descctio_.ioobj; delete &descctio_;
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
    BufferString txt( "Layer modeling" );
    if ( descctio_.ioobj )
	txt.add( " [" ).add( descctio_.ioobj->name() ).add( "]" );
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
	uiOBJDISP()->go( this );
    }
    mTriggerInstanceCreatedNotifier();
}


void uiStratLayerModel::dispEachChg( CallBacker* )
{
    synthdisp_->setDispEach( modtools_->dispEach() );
    levelChg( 0 );
}


bool uiStratLayerModel::canShowFlattened() const
{
    TypeSet<float> zlvls = moddisp_->levelDepths();
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


void uiStratLayerModel::levelChg( CallBacker* cb )
{
    moddisp_->setFlattened( modtools_->showFlattened() );

    const bool canshowflattened = canShowFlattened();
    synthdisp_->setDispMrkrs( modtools_->selLevel(), moddisp_->levelDepths(),
		    modtools_->selLevelColor(), modtools_->showFlattened() );
    synthdisp_->setSnapLevelSensitive( canshowflattened );
    if ( cb )
	levelChanged.trigger();
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
    if ( Wavelet::isScaled(synthdisp_->waveletID()) )
	return true;

    BufferStringSet opts;
    opts.add( "[&Start tool]: Start the wavelet scaling dialog" );
    opts.add( "[&Mark scaled]: The wavelet amplitude is already compatible "
	      "with the seismic data" );
    opts.add( "[&Ignore]: I will not use scaling-sensitive operations" );
    uiGetChoice dlg( this, opts,
	    "The wavelet seems to be unscaled.\n"
	    "For most purposes, you will need a scaled wavelet.\n", true );
    dlg.setHelpKey( mODHelpKey(mStratLayerModelcheckUnscaledWaveletHelpID) );
    dlg.go(); const int choice = dlg.choice();
    if ( choice < 0 )
	return false;
    else if ( choice == 2 )
	return true;
    else if ( choice == 1 )
    {
	Wavelet::markScaled( synthdisp_->waveletID() );
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
    if ( dlg.errMsg() )
	{ uiMSG().error( dlg.errMsg() ); return; }
    const char* lvlnm = modtools_->selLevel();
    if ( lvlnm && *lvlnm ) dlg.setRefLevel( lvlnm );
    dlg.go();
}


void uiStratLayerModel::wvltChg( CallBacker* cb )
{
    viewChgedCB( cb );
    levelChg( 0 );
    waveletChanged.trigger();
}


void uiStratLayerModel::modDispRangeChanged( CallBacker* )
{
    mDynamicCastGet( uiFlatViewer*,vwr,moddisp_->getViewer());
    if ( vwr )
	synthdisp_->setZDataRange( vwr->getSelDataRange( false ) , true );
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
	genModels(0);
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
	const int res = uiMSG().askSave( "Generation description not saved.\n"
					 "Save now?" );
	if ( !allowcncl && res < 0 )
	{
	    uiMSG().error( "Sorry, you cannot cancel right now."
			   "Please save or discard your work" );
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
    descctio_.ctxt.forread = false;
    uiIOObjSelDlg dlg( const_cast<uiStratLayerModel*>(this), descctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;
    descctio_.setObj( dlg.ioObj()->clone() );

    const BufferString fnm( descctio_.ioobj->fullUserExpr(false) );
    bool rv = false;

    MouseCursorChanger mcch( MouseCursor::Wait );

    fillWorkBenchPars( desc_.getWorkBenchParams() );

    od_ostream strm( fnm );
    if ( !strm.isOK() )
	uiMSG().error( "Cannot open output file" );
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

    descctio_.ctxt.forread = true;
    uiIOObjSelDlg dlg( this, descctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;
    descctio_.setObj( dlg.ioObj()->clone() );

    const BufferString fnm( descctio_.ioobj->fullUserExpr(true) );
    od_istream strm( fnm );
    if ( !strm.isOK() )
	{ uiMSG().error( "Cannot open input file" ); return false; }

    delete elpropsel_; elpropsel_ = 0;
    desc_.erase();
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

    seqdisp_->setNeedSave( false );
    lmp_.setEmpty();
    seqdisp_->descHasChanged();

    delete elpropsel_; elpropsel_ = 0;

    BufferString edtyp;
    descctio_.ctxt.toselect.require_.get( sKey::Type(), edtyp );
    BufferString profilestr( "Profile" );
    synthdisp_->resetRelativeViewRect();
    synthdisp_->setForceUpdate( true );
    if ( !profilestr.isStartOf(edtyp) )
    {
	needtoretrievefrpars_ = true;
	gentools_->genReq.trigger();
	//Set when everything is in place.
    }
    else
    {
	CBCapsule<IOPar*> caps( &desc_.getWorkBenchParams(),
				const_cast<uiStratLayerModel*>(this) );
	const_cast<uiStratLayerModel*>(this)->retrieveRequired.trigger( &caps );
    }


    if ( !useDisplayPars( desc_.getWorkBenchParams() ))
	return false;

    //Before calculation
    gentools_->usePar( desc_.getWorkBenchParams() );
    setWinTitle();
    return true;
}


MultiID uiStratLayerModel::genDescID() const
{
    MultiID ret;
    if ( descctio_.ioobj )
	ret = descctio_.ioobj->key();
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
    BufferString propnm( "[", lmpropdp.propnm_.buf(), "]" );
    SyntheticData* sd = synthdisp_->getSyntheticData( propnm );
    if ( !sd ) return;
    sd->dispPars().vdmapper_ = lmpropdp.mapper_;
    sd->dispPars().ctab_ = lmpropdp.ctab_;
    sd->dispPars().overlap_ = lmpropdp.overlap_;
    SyntheticData* vdsd = synthdisp_->getCurrentSyntheticData( false );
    SyntheticData* wvasd = synthdisp_->getCurrentSyntheticData( true );
    if ( (vdsd && propnm == vdsd->name()) ||
	 (wvasd && propnm == wvasd->name()) )
	synthdisp_->modelChanged();
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
    SyntheticData* vdsd = synthdisp_->getCurrentSyntheticData( false );
    if ( !vdsd ) return;

    BufferString sdnm( vdsd->name() );
    if ( !getCleanSyntheticName(sdnm) )
	return;

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
    synthdisp_->setForceUpdate( nrmodels_!=lmp_.getCurrent().size() );
    handleNewModel();
}


void uiStratLayerModel::genModels( CallBacker* cb )
{
    const int nrmods = gentools_->nrModels();
    if ( nrmods < 1 )
	{ uiMSG().error("Please enter a valid number of models"); return; }

    MouseCursorChanger mcs( MouseCursor::Wait );

    Strat::LayerModel* newmodl = new Strat::LayerModel;
    newmodl->propertyRefs() = seqdisp_->desc().propSelection();
    newmodl->setElasticPropSel( lmp_.getCurrent().elasticPropSel() );

    seqdisp_->prepareDesc();
    mcs.restore();

    Strat::LayerModelGenerator ex( desc_, *newmodl, nrmods );
    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(ex) || !newmodl->isValid() )
	{ delete newmodl; return; }

    // transaction succeeded, we move to the new model - period.

    const bool wasforced = synthdisp_->doForceUpdate();
    const bool wasgo = cb == gentools_;
    if ( wasgo  ) // i.e. 'Go' is used
	synthdisp_->setForceUpdate( true );

    lmp_.setBaseModel( newmodl );
    handleNewModel();

    if ( wasgo && !wasforced )
	modtools_->setMkSynthetics( false );
}



void uiStratLayerModel::handleNewModel()
{
    lmp_.resetEditing();
    synthdisp_->setUseEdited( false );
    moddisp_->setFluidReplOn( false );

    setModelProps();
    setElasticProps();
    useSyntheticsPars( desc_.getWorkBenchParams() );
    synthdisp_->setDisplayZSkip( moddisp_->getDisplayZSkip(), true );
    moddisp_->modelChanged();

    if ( needtoretrievefrpars_ )
    {
	CBCapsule<IOPar*> caps( &desc_.getWorkBenchParams(),
				const_cast<uiStratLayerModel*>(this) );
	const_cast<uiStratLayerModel*>(this)->retrieveRequired.trigger( &caps );
	needtoretrievefrpars_ = false;
    }
    else
	levelChg( 0 );

    nrmodels_ = layerModel().size();
    newModels.trigger();

    uiWorldRect prevrelzoomwr = synthdisp_->getRelativeViewRect();
    mDynamicCastGet(uiMultiFlatViewControl*,mfvc,synthdisp_->control());
    if ( mfvc ) mfvc->reInitZooms();
    synthdisp_->setZoomView( prevrelzoomwr );
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
	elpropsel_ = new ElasticPropSelection;
	if ( !elpropsel_->usePar(desc_.getWorkBenchParams()) )
	{
	    delete elpropsel_;
	    elpropsel_ = 0;
	}
    }

    if ( !elpropsel_ )
    {
	elpropsel_ = ElasticPropSelection::get( desc_.elasticPropSel() );
	if ( !elpropsel_ )
	{
	    elpropsel_ = new ElasticPropSelection;
	    ElasticPropGuess( desc_.propSelection(), *elpropsel_ );
	}
    }

    BufferString errmsg;
    if ( !elpropsel_->isValidInput( &errmsg) )
    {
	if ( !errmsg.isEmpty() )
	{
	    errmsg += "\nPlease define a new value. ";
	    uiMSG().message( errmsg.buf() );
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

    uiWorldRect prevrelzoomwr = synthdisp_->getRelativeViewRect();
    synthdisp_->setUseEdited( usefr );
    if ( parschanged )
    {
	synthdisp_->setForceUpdate( true );
	useSyntheticsPars( desc_.getWorkBenchParams() );
    }
    synthdisp_->showFRResults();
    synthdisp_->setDispMrkrs( modtools_->selLevel(), moddisp_->levelDepths(),
		    modtools_->selLevelColor(), modtools_->showFlattened() );
    moddisp_->setBrineFilled( fwd );
    moddisp_->setFluidReplOn( usefr );
    moddisp_->modelChanged();
    if ( parschanged )
    {
	mDynamicCastGet(uiMultiFlatViewControl*,mfvc,synthdisp_->control());
	if ( mfvc ) mfvc->reInitZooms();
	synthdisp_->setZoomView( prevrelzoomwr );
    }
    synthdisp_->setForceUpdate( false );
}


void uiStratLayerModel::prepareFluidRepl()
{
    lmp_.initEditing();
    Strat::LayerModel& edlm = lmp_.getEdited( true );
    if ( edlm.propertyRefs().find(Strat::LayerModel::defSVelStr()) == -1 )
	edlm.propertyRefs() += new PropertyRef( Strat::LayerModel::defSVelStr(),
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
    if ( !modtools_->usePar( par ) )
	return false;

    return true;
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
    synthdisp_->fillPar( par );
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
    fillSyntheticsPars( desc_.getWorkBenchParams() );
}


void uiStratLayerModel::infoChanged( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(IOPar,pars,caller,cb);
    mDynamicCastGet(uiStratLayerModelDisp*,moddisp,caller);
    if ( !moddisp )
    {
	BufferString mesg;
	uiFlatViewWin::makeInfoMsg( mesg, pars );
	statusBar()->message( mesg.buf() );
    }
    else
    {
	BufferString msg;
	for ( int idx=0; idx<pars.size(); idx++ )
	{
	    msg += pars.getKey( idx );
	    msg +=": ";
	    msg += pars.getValue( idx );
	    msg += "\t";
	}
	statusBar()->message( msg.buf() );
    }
}
