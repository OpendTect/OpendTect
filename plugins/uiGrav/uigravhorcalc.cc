/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Apr 2010
-*/

static const char* rcsID mUnusedVar = "$Id: uigravhorcalc.cc,v 1.10 2012-09-13 13:08:06 cvsbert Exp $";

#include "uigravhorcalc.h"
#include "gravhorcalc.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uizaxistransform.h"
#include "ioman.h"
#include "survinfo.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "emsurfaceiodata.h"
#include "uit2dconvsel.h"


uiGravHorCalc::uiGravHorCalc( uiParent* p, EM::ObjectID enobjid )
    : uiDialog(p,Setup("Calculate Gravity","", mTODOHelpID))
    , topfld_(0)
    , t2dfld_(0)
{
    MultiID horid = EM::EMM().getMultiID( enobjid );
    horioobj_ = IOM().get( horid );
    if ( !horioobj_ )
	{ new uiLabel(this,"Internal: Cannot find horizon"); return; }
    setTitleText( BufferString("Calculate gravity at '",
		  horioobj_->name(), "'" ) );

    const IOObjContext ctxt( mIOObjContext(EMHorizon3D) );
    uiIOObjSel::Setup su( "Top Horizon" );
    su.optional_ = true;
    uiGroup* inpgrp = new uiGroup( this, "Upper group" );

    topfld_ = new uiIOObjSel( inpgrp, ctxt, su );
    topfld_->setInput( horid );
    topfld_->selectionDone.notify( mCB(this,uiGravHorCalc,topSel) );
    topfld_->setChecked( (bool)topfld_->ioobj(true) );

    denvarfld_ = new uiGenInput( inpgrp, "Density (contrast)",
	    			   BoolInpSpec(false,"Variable","Constant") );
    denvarfld_->attach( alignedBelow, topfld_ );
    denvarfld_->valuechanged.notify( mCB(this,uiGravHorCalc,denVarSel) );
    denattrfld_ = new uiGenInput( inpgrp,"Attribute containing Density (kg/m3)",
	    			  StringListInpSpec() );
    denattrfld_->attach( alignedBelow, denvarfld_ );
    denvaluefld_ = new uiGenInput( inpgrp, "Density (kg/m3)",
	    			   FloatInpSpec(1000) );
    denvaluefld_->attach( alignedBelow, denvarfld_ );

    su.seltxt_ = "Bottom Horizon";
    botfld_ = new uiIOObjSel( inpgrp, ctxt, su );
    botfld_->setInput( MultiID("") );
    botfld_->attach( alignedBelow, denattrfld_ );
    inpgrp->setHAlignObj( botfld_ );

    if ( SI().zIsTime() )
    {
	uiT2DConvSel::Setup convsu( topfld_, false );
	t2dfld_ = new uiT2DConvSel( inpgrp, convsu );
	t2dfld_->attach( alignedBelow, botfld_ );
    }

    cutoffangfld_ = new uiGenInput( this, "Cutoff angle (deg)",
	    			    IntInpSpec(45) );
    cutoffangfld_->attach( alignedBelow, inpgrp );

    attrnmfld_ = new uiGenInput( this, "Output attribute",
	    			 StringInpSpec("Grav") );
    attrnmfld_->attach( alignedBelow, cutoffangfld_ );
    uiLabel* lbl = new uiLabel( this,
	    		BufferString("(on '",horioobj_->name(),"')") );
    lbl->attach( rightOf, attrnmfld_ );

    postFinalise().notify( mCB(this,uiGravHorCalc,initFlds) );
}


uiGravHorCalc::~uiGravHorCalc()
{
    delete const_cast<IOObj*>( horioobj_ );
}


void uiGravHorCalc::initFlds( CallBacker* )
{
    denVarSel( 0 );
    topSel( 0 );
}


void uiGravHorCalc::denVarSel( CallBacker* )
{
    const bool isvar = denvarfld_->getBoolValue();
    denattrfld_->display( isvar );
    denvaluefld_->display( !isvar );
}


void uiGravHorCalc::topSel( CallBacker* )
{
    const IOObj* ioobj = topfld_->ioobj( true );
    if ( !ioobj ) ioobj = horioobj_;

    EM::IOObjInfo eminfo( ioobj->key() );
    if ( !eminfo.isOK() )
    {
	BufferString msg( "Cannot read '", ioobj->name().buf(), "'" );
	uiMSG().error(msg); return;
    }

    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    denattrfld_->newSpec( StringListInpSpec(attrnms), 0 );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGravHorCalc::acceptOK( CallBacker* )
{
    if ( !topfld_ ) return false;

    return false;
}
