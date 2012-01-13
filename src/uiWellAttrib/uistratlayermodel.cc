/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlayermodel.cc,v 1.43 2012-01-13 10:26:22 cvsbert Exp $";

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

#include "uielasticpropsel.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiselsimple.h"
#include "uisplitter.h"
#include "uistratbasiclayseqgendesc.h"
#include "uistratlaymoddisp.h"
#include "uistratsynthdisp.h"
#include "uistratsynthcrossplot.h"
#include "uistratlaymodtools.h"
#include "uistrattreewin.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"
#include "uichecklist.h"


const char* uiStratLayerModel::sKeyModeler2Use()
{
    return "dTect.Stratigraphic Modeler to use";
}


class uiStratLayerModelLauncher : public CallBacker
{
public:

void theCB( CallBacker* cb )
{
    if ( !Strat::RT().hasChildren() )
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

    uiStratLayerModel dlg( par, modnm );
    dlg.go();
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


uiStratLayerModel::uiStratLayerModel( uiParent* p, const char* edtyp )
    : uiMainWin(p,"",0,false,true)
    , desc_(*new Strat::LayerSequenceGenDesc(Strat::RT()))
    , modl_(*new Strat::LayerModel)
    , elpropsel_(0)				   
    , descctio_(*mMkCtxtIOObj(StratLayerSequenceGenDesc))
{
    uiGroup* gengrp = new uiGroup( this, "SeqGen disp" );
    uiGroup* rightgrp = new uiGroup( this, "Right group" );
    uiGroup* botgrp = new uiGroup( rightgrp, "Bottom group" );

    if ( !edtyp || !*edtyp )
	edtyp = uiBasicLayerSequenceGenDesc::typeStr();
    seqdisp_ = uiLayerSequenceGenDesc::factory().create( edtyp, gengrp, desc_ );
    if ( !seqdisp_ )
	seqdisp_ = new uiBasicLayerSequenceGenDesc( gengrp, desc_ );
    descctio_.ctxt.toselect.require_.set( sKey::Type, edtyp );

    gentools_ = new uiStratGenDescTools( gengrp );
    gentools_->attach( ensureBelow, seqdisp_ );
    gentools_->openReq.notify( mCB(this,uiStratLayerModel,openGenDescCB) );
    gentools_->saveReq.notify( mCB(this,uiStratLayerModel,saveGenDescCB) );
    gentools_->propEdReq.notify( mCB(this,uiStratLayerModel,manPropsCB) );
    gentools_->genReq.notify( mCB(this,uiStratLayerModel,genModels) );

    synthdisp_ = new uiStratSynthDisp( rightgrp, modl_ );
    synthdisp_->wvltChanged.notify( mCB(this,uiStratLayerModel,wvltChg) );
    synthdisp_->zoomChanged.notify( mCB(this,uiStratLayerModel,zoomChg) );
    synthdisp_->modSelChanged.notify( mCB(this,uiStratLayerModel,modSelChg) );
    synthdisp_->layerPropSelNeeded.notify(
				mCB(this,uiStratLayerModel,selElasticPropsCB) );
    uiToolButtonSetup tbsu( "xplot.png", "Attributes vs model properties",
	   		    mCB(this,uiStratLayerModel,xPlotReq) );
    synthdisp_->addTool( tbsu );

    modtools_ = new uiStratLayModEditTools( botgrp );
    moddisp_ = new uiStratLayerModelDisp( *modtools_, modl_ );
    modtools_->attach( ensureBelow, moddisp_ );
    modtools_->attach( rightBorder );
    modtools_->dispEachChg.notify( mCB(this,uiStratLayerModel,dispEachChg) );
    modtools_->selLevelChg.notify( mCB(this,uiStratLayerModel,levelChg) );

    uiSplitter* spl = new uiSplitter( this, "Vert splitter", true );
    spl->addGroup( gengrp ); spl->addGroup( rightgrp );
    spl = new uiSplitter( rightgrp, "Hor splitter", false );
    spl->addGroup( synthdisp_ ); spl->addGroup( botgrp );

    setWinTitle();
}


uiStratLayerModel::~uiStratLayerModel()
{
    delete &desc_;
    delete &modl_;
    delete descctio_.ioobj; delete &descctio_;
}


void uiStratLayerModel::setWinTitle()
{
    BufferString txt( "Layer modeling" );
    if ( descctio_.ioobj )
	txt.add( " [" ).add( descctio_.ioobj->name() ).add( "]" );
    setCaption( txt );
}


void uiStratLayerModel::dispEachChg( CallBacker* cb )
{
    levelChg( cb );
}


void uiStratLayerModel::levelChg( CallBacker* )
{
    synthdisp_->setDispMrkrs( modtools_->selLevel(), moddisp_->levelDepths(),
	    			moddisp_->levelColor() );
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


void uiStratLayerModel::xPlotReq( CallBacker* )
{
    uiStratSynthCrossplot dlg( this, modl_, synthdisp_->getSynthetics() );
    if ( dlg.errMsg() )
	{ uiMSG().error( dlg.errMsg() ); return; } 
    const char* lvlnm = modtools_->selLevel();
    if ( lvlnm && *lvlnm ) dlg.setRefLevel( lvlnm );
    dlg.go();
}


void uiStratLayerModel::wvltChg( CallBacker* cb )
{
    zoomChg( cb );
    levelChg( cb );
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
    moddisp_->modelChanged();
    synthdisp_->modelChanged();
    delete elpropsel_; elpropsel_ = 0;
    setWinTitle();
    return true;
}


void uiStratLayerModel::genModels( CallBacker* cb )
{
    const int nrmods = gentools_->nrModels();
    if ( nrmods < 1 )
	{ uiMSG().error("Please enter a valid number of models"); return; }

    modl_.setEmpty();
    modl_.propertyRefs() = seqdisp_->desc().propSelection();
    uiTaskRunner tr( this );
    Strat::LayerModelGenerator ex( desc_, modl_, nrmods );
    tr.execute( ex );

    setModelProps();
    setElasticProps();

    moddisp_->modelChanged();
    synthdisp_->modelChanged();
    levelChg( cb );
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
}


bool uiStratLayerModel::closeOK()
{
    return saveGenDescIfNecessary();
}
