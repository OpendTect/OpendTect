/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlayermodel.cc,v 1.70 2012/09/14 14:05:09 cvshelene Exp $";

#include "uistratlayermodel.h"

#include "ctxtioobj.h"
#include "elasticpropsel.h"
#include "executor.h"
#include "ioobj.h"
#include "ioman.h"
#include "strmprov.h"
#include "settings.h"
#include "separstr.h"
#include "stratlayseqgendesc.h"
#include "stratlayermodel.h"
#include "strattransl.h"
#include "stratlaymodgen.h"
#include "stratreftree.h"
#include "stratsynth.h"
#include "wavelet.h"

#include "uielasticpropsel.h"
#include "uiobjdisposer.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uitoolbar.h"
#include "uimsg.h"
#include "uiselsimple.h"
#include "uisplitter.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uistratbasiclayseqgendesc.h"
#include "uistratsimplelaymoddisp.h"
#include "uistratsynthdisp.h"
#include "uistratsynthcrossplot.h"
#include "uistratlaymodtools.h"
#include "uistrattreewin.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"
#include "uichecklist.h"

mDefineInstanceCreatedNotifierAccess(uiStratLayerModel)


const char* uiStratLayerModel::sKeyModeler2Use()
{
    return "dTect.Stratigraphic Modeler to use";
}


class uiStratLayerModelLauncher : public CallBacker
{
public:

void theCB( CallBacker* cb )
{
    if ( Strat::RT().isEmpty() )
	return;

    const BufferStringSet& nms =
			uiLayerSequenceGenDesc::factory().getNames( true );
    if ( nms.isEmpty() ) return;

    mDynamicCastGet(uiToolButton*,tb,cb)
    if ( !tb ) return;

    uiParent* par = tb->parent();
    const char* settres = Settings::common().find(
	    			uiStratLayerModel::sKeyModeler2Use());
    BufferString modnm( settres );
    int defmodnr = -1;
    bool givechoice = nms.size() > 1;
    if ( modnm.isEmpty() )
	modnm = nms.get( nms.size() - 1 );
    else
    {
	FileMultiString fms( modnm );
	modnm = fms[0];
	defmodnr = nms.indexOf( modnm.buf() );
	if ( defmodnr < 0 )
	    modnm.setEmpty();
	else
	    givechoice = *fms[1] != 'A';
    }

    if ( givechoice )
    {
	uiSelectFromList::Setup sflsu( "Select modeling type", nms );
	sflsu.current( defmodnr < 0 ? nms.size()-1 : defmodnr );
	uiSelectFromList dlg( par, sflsu );
	uiCheckList* defpol = new uiCheckList( &dlg, "Set as default",
				"Always use this type", uiCheckList::Chain1st );
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
			BufferString(newmodnm, indic == 2 ? "`A" : "") );
	    }
	    else if ( defmodnr >= 0 )
		needwrite = false;
	}

	if ( needwrite )
	    Settings::common().write( false );
	modnm = newmodnm;
    }

    doLayerModel( par, modnm );
}


void doBasicLayerModel( uiParent* p )
{
    doLayerModel( p, uiBasicLayerSequenceGenDesc::typeStr() );
}


void doLayerModel( uiParent* p, const char* modnm )
{
    if ( Strat::RT().isEmpty() )
	return;

    uiStratLayerModel* dlg = new uiStratLayerModel( p, modnm );
    dlg->go();
}


void addToTreeWin()
{
    uiToolButtonSetup* su = new uiToolButtonSetup( "stratlayermodeling.png",
			    "Start layer/synthetics modeling",
			    mCB(this,uiStratLayerModelLauncher,theCB) );
    uiStratTreeWin::addTool( su );
}

};

void uiStratLayerModel::initClass()
{
    static uiStratLayerModelLauncher launcher;
    launcher.addToTreeWin();
}


void uiStratLayerModel::doBasicLayerModel()
{
    doLayerModel( uiBasicLayerSequenceGenDesc::typeStr() );
}


void uiStratLayerModel::doLayerModel( const char* modnm )
{
    uiStratLayerModelLauncher launcher;
    launcher.doLayerModel( &StratTreeWin(), modnm );
}


uiStratLayerModel::uiStratLayerModel( uiParent* p, const char* edtyp )
    : uiMainWin(p,"",0,false)
    , desc_(*new Strat::LayerSequenceGenDesc(Strat::RT()))
    , modl_(*new Strat::LayerModel)
    , modlpostfr_(*new Strat::LayerModel)
    , elpropsel_(0)				   
    , descctio_(*mMkCtxtIOObj(StratLayerSequenceGenDesc))
    , analtb_(0)
    , usepostfrmodl_(false)      
    , newModels(this)				   
    , levelChanged(this)				   
    , waveletChanged(this)				   
{
    setDeleteOnClose( true );

    if ( !edtyp || !*edtyp )
	edtyp = uiBasicLayerSequenceGenDesc::typeStr();
    descctio_.ctxt.toselect.require_.set( sKey::Type, edtyp );

    uiGroup* gengrp = new uiGroup( this, "Gen group" );
    seqdisp_ = uiLayerSequenceGenDesc::factory().create( edtyp, gengrp, desc_ );
    if ( !seqdisp_ )
	seqdisp_ = new uiBasicLayerSequenceGenDesc( gengrp, desc_ );

    uiGroup* topgrp; uiGroup* botgrp; uiGroup* rightgrp;
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
    moddisp_ = seqdisp_->getLayModDisp( *modtools_, modl_ );
    if ( !moddisp_ )
	return;

    synthdisp_ = new uiStratSynthDisp( topgrp, modl_ );
    analtb_ = new uiToolBar( this, "Analysis toolbar", uiToolBar::Right );
    uiToolButtonSetup tbsu( "xplot.png", "Attributes vs model properties",
	   		    mCB(this,uiStratLayerModel,xPlotReq) );
    analtb_->addButton( tbsu );
    mDynamicCastGet( uiFlatViewer*,vwr,moddisp_->getViewer());
    if ( vwr ) synthdisp_->addViewerToControl( *vwr );

    modtools_->attach( ensureBelow, moddisp_ );
    gentools_->attach( ensureBelow, seqdisp_->outerObj() );

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
    gentools_->openReq.notify( mCB(this,uiStratLayerModel,openGenDescCB) );
    gentools_->saveReq.notify( mCB(this,uiStratLayerModel,saveGenDescCB) );
    gentools_->propEdReq.notify( mCB(this,uiStratLayerModel,manPropsCB) );
    gentools_->genReq.notify( mCB(this,uiStratLayerModel,genModels) );
    synthdisp_->wvltChanged.notify( mCB(this,uiStratLayerModel,wvltChg) );
    synthdisp_->zoomChanged.notify( mCB(this,uiStratLayerModel,zoomChg) );
    synthdisp_->modSelChanged.notify( mCB(this,uiStratLayerModel,modSelChg) );
    synthdisp_->layerPropSelNeeded.notify(
			    mCB(this,uiStratLayerModel,selElasticPropsCB) );
    moddisp_->genNewModelNeeded.notify( mCB(this,uiStratLayerModel,genModels) );
    moddisp_->rangeChanged.notify( 
			    mCB(this,uiStratLayerModel,modDispRangeChanged));
    moddisp_->sequenceSelected.notify( mCB(this,uiStratLayerModel,seqSel) );
    moddisp_->modelEdited.notify( mCB(this,uiStratLayerModel,modEd) );

    setWinTitle();
    StratTreeWin().changeLayerModelNumber( true );
    postFinalise().notify( mCB(this,uiStratLayerModel,initWin) );
}


uiStratLayerModel::~uiStratLayerModel()
{
    delete &desc_;
    delete &modl_;
    delete &modlpostfr_;
    delete descctio_.ioobj; delete &descctio_;
    StratTreeWin().changeLayerModelNumber( false );
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


const SeisTrcBuf& uiStratLayerModel::postStackTraces() const
{
    return synthdisp_->postStackTraces();
}


const SeisTrcBuf& uiStratLayerModel::modelTraces( const PropertyRef& pr ) const
{
    return synthdisp_->postStackTraces( &pr );
}


const PropertyRefSelection& uiStratLayerModel::modelProperties() const
{
    return synthdisp_->modelPropertyRefs();
}


const ObjectSet<const TimeDepthModel>& uiStratLayerModel::d2TModels() const
{
    static ObjectSet<const TimeDepthModel> empty;
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


void uiStratLayerModel::levelChg( CallBacker* cb )
{
    synthdisp_->setDispMrkrs( modtools_->selLevel(), moddisp_->levelDepths(),
		    modtools_->selLevelColor(), modtools_->showFlattened() );
    moddisp_->setFlattened( modtools_->showFlattened() );
    if ( cb )
	levelChanged.trigger();
}


void uiStratLayerModel::modSelChg( CallBacker* cb )
{
    mCBCapsuleUnpack(int,modidx,cb);
    moddisp_->selectSequence( modidx -1 );
}


void uiStratLayerModel::zoomChg( CallBacker* )
{
    moddisp_->setZoomBox( synthdisp_->curView(true) );
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
    dlg.setHelpID( "110.2.2" );
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

    uiStratSynthCrossplot dlg( this, usepostfrmodl_ ? modlpostfr_ : modl_,
	    		       synthdisp_->getSynthetics() );
    if ( dlg.errMsg() )
	{ uiMSG().error( dlg.errMsg() ); return; } 
    const char* lvlnm = modtools_->selLevel();
    if ( lvlnm && *lvlnm ) dlg.setRefLevel( lvlnm );
    dlg.go();
}


void uiStratLayerModel::wvltChg( CallBacker* cb )
{
    zoomChg( cb );
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
	return true;
    }
    return false;
}


bool uiStratLayerModel::saveGenDescIfNecessary() const
{
    if ( !seqdisp_->needSave() )
	return true;

    const int res = uiMSG().askSave( "Generation description not saved.\n"
	    			     "Save now?" );
    if ( res < 1 ) return res == 0;
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
    StreamData sd( StreamProvider(fnm).makeOStream() );
    bool rv = false;
    if ( !sd.usable() )
	uiMSG().error( "Cannot open output file" );
    else if ( !desc_.putTo(*sd.ostrm) )
	uiMSG().error(desc_.errMsg());
    else
    {
	rv = true;
	seqdisp_->setNeedSave( false );
	const_cast<uiStratLayerModel*>(this)->setWinTitle();
    }

    sd.close();
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
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	{ uiMSG().error( "Cannot open input file" ); return false; }

    desc_.erase();
    bool rv = desc_.getFrom( *sd.istrm );
    if ( !rv )
	uiMSG().error(desc_.errMsg());
    sd.close();
    if ( !rv )
	return false;

    seqdisp_->setNeedSave( false );
    seqdisp_->descHasChanged();
    modl_.setEmpty();
    modlpostfr_.setEmpty();
    moddisp_->modelChanged();
    synthdisp_->modelChanged();
    delete elpropsel_; elpropsel_ = 0;
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


void uiStratLayerModel::modEd( CallBacker* )
{
    synthdisp_->modelChanged();
}


void uiStratLayerModel::genModels( CallBacker* )
{
    const int nrmods = gentools_->nrModels();
    if ( nrmods < 1 )
	{ uiMSG().error("Please enter a valid number of models"); return; }

    modl_.setEmpty();
    modl_.propertyRefs() = seqdisp_->desc().propSelection();
    modlpostfr_.setEmpty();
    uiTaskRunner tr( this );
    Strat::LayerModelGenerator ex( desc_, modl_, nrmods );
    tr.execute( ex );

    setModelProps();
    setElasticProps();

    moddisp_->modelChanged();
    synthdisp_->modelChanged();
    levelChg( 0 );
    newModels.trigger();
}


void uiStratLayerModel::setModelProps()
{
    BufferStringSet nms;
    for ( int idx=1; idx<modl_.propertyRefs().size(); idx++ )
	nms.add( modl_.propertyRefs()[idx]->name() );
    modtools_->setProps( nms );
    nms.erase(); const Strat::LevelSet& lvls = Strat::LVLS();
    for ( int idx=0; idx<lvls.size(); idx++ )
	nms.add( lvls.levels()[idx]->name() );
    modtools_->setLevelNames( nms );
    nms.erase(); const Strat::ContentSet& conts = modl_.refTree().contents();
    for ( int idx=0; idx<conts.size(); idx++ )
	nms.add( conts[idx]->name() );
    modtools_->setContentNames( nms );
}


void uiStratLayerModel::setElasticProps()
{
    if ( !elpropsel_ )
	elpropsel_ = ElasticPropSelection::get( desc_.elasticPropSel() );

    if ( !elpropsel_ )
    {
	elpropsel_ = new ElasticPropSelection;
	ElasticPropGuess( desc_.propSelection(), *elpropsel_ );
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

    modl_.addElasticPropSel( *elpropsel_ );
    modlpostfr_.addElasticPropSel( *elpropsel_ );
}


bool uiStratLayerModel::closeOK()
{
    return saveGenDescIfNecessary();
}


void uiStratLayerModel::displayFRResult( SyntheticData* synthdata )
{
    usepostfrmodl_ = synthdata;
    synthdisp_->displaySynthetic( synthdata ? synthdata
	    			: synthdisp_->getCurrentSyntheticData() );
    moddisp_->modelChanged();
}


SyntheticData* uiStratLayerModel::getCurrentSyntheticData() const
{                                                                                 
    return synthdisp_->getCurrentSyntheticData();
}


void uiStratLayerModel::prepareFluidRepl()
{
    modlpostfr_ = modl_;
}
