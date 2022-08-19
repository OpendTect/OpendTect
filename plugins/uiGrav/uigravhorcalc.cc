/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigravhorcalc.h"
#include "gravhorcalc.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "ioman.h"
#include "survinfo.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "emsurfaceiodata.h"
#include "uistrings.h"
#include "uit2dconvsel.h"


uiGravHorCalc::uiGravHorCalc( uiParent* p, EM::ObjectID enobjid )
    : uiDialog(p,Setup(tr("Calculate Gravity"),uiStrings::sEmptyString(), 
                       mTODOHelpKey))
    , topfld_(0)
    , t2dfld_(0)
{
    MultiID horid = EM::EMM().getMultiID( enobjid );
    horioobj_ = IOM().get( horid );
    if ( !horioobj_ )
	{ new uiLabel(this,tr("Internal: Cannot find horizon")); return; }
    setTitleText( tr("Calculate gravity at '%1'").arg( horioobj_->name() ) );

    const IOObjContext ctxt( mIOObjContext(EMHorizon3D) );
    uiIOObjSel::Setup su( uiStrings::sTopHor() );
    su.optional_ = true;
    uiGroup* inpgrp = new uiGroup( this, "Upper group" );

    topfld_ = new uiIOObjSel( inpgrp, ctxt, su );
    topfld_->setInput( horid );
    topfld_->selectionDone.notify( mCB(this,uiGravHorCalc,topSel) );
    topfld_->setChecked( (bool)topfld_->ioobj(true) );

    denvarfld_ = new uiGenInput( inpgrp, tr("Density (contrast)"),
	    			   BoolInpSpec(false,tr("Variable"),
                                   tr("Constant")) );
    denvarfld_->attach( alignedBelow, topfld_ );
    denvarfld_->valuechanged.notify( mCB(this,uiGravHorCalc,denVarSel) );
    denattrfld_ = new uiGenInput( inpgrp,
                                  tr("Attribute containing Density (kg/m3)"),
	    			  StringListInpSpec() );
    denattrfld_->attach( alignedBelow, denvarfld_ );
    denvaluefld_ = new uiGenInput( inpgrp, tr("Density (kg/m3)"),
	    			   FloatInpSpec(1000) );
    denvaluefld_->attach( alignedBelow, denvarfld_ );

    su.seltxt_ = uiStrings::sBottomHor();
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

    cutoffangfld_ = new uiGenInput( this, tr("Cutoff angle (deg)"),
	    			    IntInpSpec(45) );
    cutoffangfld_->attach( alignedBelow, inpgrp );

    attrnmfld_ = new uiGenInput( this, tr("Output attribute"),
	    			 StringInpSpec("Grav") );
    attrnmfld_->attach( alignedBelow, cutoffangfld_ );
    uiLabel* lbl = new uiLabel( this,
	    		tr("(on '%1')").arg( horioobj_->name() ) );
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
	uiString msg = tr("Cannot read '%1'").arg(ioobj->name().buf());
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
