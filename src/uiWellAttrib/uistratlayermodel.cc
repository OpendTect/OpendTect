/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlayermodel.cc,v 1.32 2011-08-11 13:47:30 cvsbruno Exp $";

#include "uistratlayermodel.h"

#include "ctxtioobj.h"
#include "elasticpropsel.h"
#include "executor.h"
#include "ioobj.h"
#include "ioman.h"
#include "strmprov.h"
#include "settings.h"
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
#include "uistrattreewin.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"


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
    const char* nm = nms.get(0).buf();
    const char* settres = Settings::common().find(
	    			uiStratLayerModel::sKeyModeler2Use());
    if ( settres && nms.isPresent(settres) )
	nm = settres;
    else if ( nms.size() > 1 )
    {
	uiSelectFromList dlg( par,
		uiSelectFromList::Setup("Select modeling type",nms) );
	uiCheckBox* alwusebut = new uiCheckBox( &dlg,
					"Always use this modeling type");
	alwusebut->attach( centeredBelow, dlg.selFld() );
	if ( !dlg.go() ) return;
	const int sel = dlg.selection();
	if ( sel < 0 ) return;
	nm = nms.get(sel).buf();
	if ( alwusebut->isChecked() )
	{
	    Settings::common().set( uiStratLayerModel::sKeyModeler2Use(), nm );
	    Settings::common().write();
	}
    }

    uiStratLayerModel dlg( par, nm );
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
    : uiMainWin(p,"Layer modeling",0,false,true)
    , desc_(*new Strat::LayerSequenceGenDesc(Strat::RT()))
    , modl_(*new Strat::LayerModel)
    , elpropsel_(0)				   
    , descctio_(*mMkCtxtIOObj(StratLayerSequenceGenDesc))
{
    uiGroup* gengrp = new uiGroup( this, "SeqGen disp" );
    uiGroup* rightgrp = new uiGroup( this, "Right group" );

    if ( !edtyp || !*edtyp )
	edtyp = uiBasicLayerSequenceGenDesc::typeStr();
    seqdisp_ = uiLayerSequenceGenDesc::factory().create( edtyp, gengrp, desc_ );
    if ( !seqdisp_ )
	seqdisp_ = new uiBasicLayerSequenceGenDesc( gengrp, desc_ );

    uiGroup* leftgengrp = new uiGroup( gengrp, "Left buttons" );
    uiToolButton* opentb = new uiToolButton( leftgengrp, "open.png",
				"Open stored generation description",
				mCB(this,uiStratLayerModel,openGenDescCB) );
    leftgengrp->attach( ensureBelow, seqdisp_ );
    uiToolButton* stb = new uiToolButton( leftgengrp, "save.png",
	    			"Save generation description",
				mCB(this,uiStratLayerModel,saveGenDescCB) );
    stb->attach( rightOf, opentb );
    uiToolButton* proptb = new uiToolButton( leftgengrp, "defprops.png",
	    			"Manage layer properties",
				mCB(this,uiStratLayerModel,manPropsCB) );
    proptb->attach( rightOf, stb );

    uiGroup* rightgengrp = new uiGroup( gengrp, "Right buttons" );
    const CallBack gocb( mCB(this,uiStratLayerModel,genModels) );
    nrmodlsfld_ = new uiGenInput( rightgengrp, "", IntInpSpec(25) );
    nrmodlsfld_->setElemSzPol( uiObject::Small );
    nrmodlsfld_->setStretch( 0, 0 );
    nrmodlsfld_->setToolTip( "Number of models to generate", 0 );
    nrmodlsfld_->updateRequested.notify( gocb );
    uiToolButton* gotb = new uiToolButton( rightgengrp, "go.png",
	    			"Generate this amount of models", gocb );
    nrmodlsfld_->attach( leftOf, gotb );
    rightgengrp->attach( rightBorder );
    rightgengrp->attach( ensureBelow, seqdisp_ );
    rightgengrp->attach( ensureRightOf, leftgengrp );
    rightgengrp->setFrame( true );

    synthdisp_ = new uiStratSynthDisp( rightgrp, modl_ );
    synthdisp_->wvltChanged.notify( mCB(this,uiStratLayerModel,wvltChg) );
    synthdisp_->zoomChanged.notify( mCB(this,uiStratLayerModel,zoomChg) );
    synthdisp_->layerPropSelNeeded.notify(
				mCB(this,uiStratLayerModel,selElasticPropsCB) );
    uiToolButtonSetup tbsu( "xplot.png", "Attributes vs model properties",
	   		    mCB(this,uiStratLayerModel,xPlotReq) );
    synthdisp_->addTool( tbsu );
    moddisp_ = new uiStratLayerModelDisp( rightgrp, modl_ );
    moddisp_->dispEachChg.notify( mCB(this,uiStratLayerModel,dispEachChg) );
    moddisp_->levelChg.notify( mCB(this,uiStratLayerModel,levelChg) );

    uiSplitter* spl = new uiSplitter( this, "Vert splitter", true );
    spl->addGroup( gengrp ); spl->addGroup( rightgrp );
    spl = new uiSplitter( rightgrp );
    spl = new uiSplitter( rightgrp, "Hor splitter", false );
    spl->addGroup( synthdisp_ ); spl->addGroup( moddisp_ );
}


uiStratLayerModel::~uiStratLayerModel()
{
    delete &desc_;
    delete &modl_;
    delete descctio_.ioobj; delete &descctio_;
}


void uiStratLayerModel::dispEachChg( CallBacker* cb )
{
    levelChg( cb );
}


void uiStratLayerModel::levelChg( CallBacker* )
{
    synthdisp_->setDispMrkrs( moddisp_->selectedLevel(),
	    		      moddisp_->levelDepths(), moddisp_->levelColor() );
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
    const char* lvlnm = moddisp_->selectedLevel();
    if ( lvlnm && *lvlnm ) dlg.setRefLevel( lvlnm );
    dlg.go();
}


void uiStratLayerModel::wvltChg( CallBacker* cb )
{
    zoomChg( cb );
}


void uiStratLayerModel::manPropsCB( CallBacker* )
{
    seqdisp_->selProps();
}


void uiStratLayerModel::selElasticPropsCB( CallBacker* )
{
    selElasticProps();
    genModels(0);
}


void uiStratLayerModel::selElasticProps()
{
    uiElasticPropSelDlg dlg(this,desc_.propSelection(),desc_.elasticPropSel());
    if ( dlg.go() )
    {
	desc_.setElasticPropSel( dlg.storedKey() );
	delete elpropsel_; 
	elpropsel_ = new ElasticPropSelection( dlg.elasticSel() );
    }
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
    return true;
}


void uiStratLayerModel::genModels( CallBacker* cb )
{
    const int nrmods = nrmodlsfld_->getIntValue();
    if ( nrmods < 1 )
	{ uiMSG().error("Please enter a valid number of models"); return; }

    modl_.setEmpty();
    modl_.propertyRefs() = seqdisp_->desc().propSelection();
    uiTaskRunner tr( this );
    Strat::LayerModelGenerator ex( desc_, modl_, nrmods );
    tr.execute( ex );

    setElasticProps();

    moddisp_->modelChanged();
    synthdisp_->modelChanged();
    levelChg( cb );
}


void uiStratLayerModel::setElasticProps()
{
    const bool hadsel = elpropsel_;

    if ( !elpropsel_ )
	elpropsel_ = ElasticPropSelection::get( desc_.elasticPropSel() );

    if ( !elpropsel_ || !elpropsel_->isValidInput() )
	selElasticProps();

    if ( !elpropsel_ ) 
	return;

    if ( !hadsel )
	seqdisp_->setNeedSave( true );

    modl_.addElasticPropSel( *elpropsel_ );
}


bool uiStratLayerModel::closeOK()
{
    return saveGenDescIfNecessary();
}
