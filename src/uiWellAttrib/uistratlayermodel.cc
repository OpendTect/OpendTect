/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlayermodel.cc,v 1.5 2010-12-10 14:32:33 cvsbert Exp $";

#include "uistratlayermodel.h"
#include "uistratsinglayseqgendesc.h"
#include "uistratlaymoddisp.h"
#include "uistratsynthdisp.h"
#include "uistrattreewin.h"
#include "stratlayseqgendesc.h"
#include "stratlayermodel.h"
#include "stratlaygentr.h"
#include "stratlaymodgen.h"
#include "stratreftree.h"
#include "executor.h"
#include "uitaskrunner.h"
#include "uiselsimple.h"
#include "uisplitter.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uitoolbutton.h"
#include "uilabel.h"
#include "uigroup.h"
#include "uimsg.h"
#include "strmprov.h"
#include "ctxtioobj.h"
#include "ioobj.h"


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
    if ( nms.size() > 1 )
    {
	uiSelectFromList ls( par,
		uiSelectFromList::Setup("Select modeling type",nms) );
	ls.go();
	const int sel = ls.selection();
	if ( sel < 0 ) return;
	nm = nms.get(sel).buf();
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
    , descctio_(*mMkCtxtIOObj(StratLayerSequenceGenDesc))
    , modlctio_(*mMkCtxtIOObj(StratLayerModel))
{
    uiGroup* gengrp = new uiGroup( this, "SeqGen disp" );
    uiGroup* rightgrp = new uiGroup( this, "Right group" );

    if ( !edtyp || !*edtyp )
	edtyp = uiSingleLayerSequenceGenDesc::typeStr();
    seqdisp_ = uiLayerSequenceGenDesc::factory().create( edtyp, gengrp, desc_ );
    if ( !seqdisp_ )
	seqdisp_ = new uiSingleLayerSequenceGenDesc( gengrp, desc_ );

    uiGroup* leftgengrp = new uiGroup( gengrp, "Left buttons" );
    uiToolButton* opentb = new uiToolButton( leftgengrp, "open.png",
				"Open stored generation description",
				mCB(this,uiStratLayerModel,openGenDesc) );
    leftgengrp->attach( ensureBelow, seqdisp_ );
    uiToolButton* stb = new uiToolButton( leftgengrp, "save.png",
	    			"Save generation description",
				mCB(this,uiStratLayerModel,saveGenDesc) );
    stb->attach( rightOf, opentb );

    uiGroup* rightgengrp = new uiGroup( gengrp, "Right buttons" );
    const CallBack gocb( mCB(this,uiStratLayerModel,genModels) );
    nrmodlsfld_ = new uiGenInput( rightgengrp, "", IntInpSpec(100) );
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
    delete modlctio_.ioobj; delete &modlctio_;
}


void uiStratLayerModel::dispEachChg( CallBacker* )
{
    synthdisp_->setDispEach( moddisp_->getEachDisp() );
}


void uiStratLayerModel::levelChg( CallBacker* )
{
    synthdisp_->setDispMrkrs( moddisp_->levelDepths(), moddisp_->levelColor() );
}


void uiStratLayerModel::openGenDesc( CallBacker* )
{
    descctio_.ctxt.forread = true;
    uiIOObjSelDlg dlg( this, descctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return;
    descctio_.setObj( dlg.ioObj()->clone() );

    const BufferString fnm( descctio_.ioobj->fullUserExpr(true) );
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	{ uiMSG().error( "Cannot open input file" ); return; }

    desc_.erase();
    if ( !desc_.getFrom(*sd.istrm) )
	uiMSG().error(desc_.errMsg());
    sd.close();

    seqdisp_->descHasChanged();
    modl_.setEmpty();
    moddisp_->modelChanged();
}


void uiStratLayerModel::saveGenDesc( CallBacker* )
{
    descctio_.ctxt.forread = false;
    uiIOObjSelDlg dlg( this, descctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return;
    descctio_.setObj( dlg.ioObj()->clone() );

    const BufferString fnm( descctio_.ioobj->fullUserExpr(false) );
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
	uiMSG().error( "Cannot open output file" );
    else if ( !desc_.putTo(*sd.ostrm) )
	uiMSG().error(desc_.errMsg());

    sd.close();
}


void uiStratLayerModel::genModels( CallBacker* )
{
    const int nrmods = nrmodlsfld_->getIntValue();
    if ( nrmods < 1 )
	{ uiMSG().error("Please enter a valid number of models"); return; }

    modl_.setEmpty();
    seqdisp_->getPropertyRefs( modl_.propertyRefs() );
    uiTaskRunner tr( this );
    Strat::LayerModelGenerator ex( desc_, modl_, nrmods );
    tr.execute( ex );
    moddisp_->modelChanged();
    synthdisp_->modelChanged();
}
