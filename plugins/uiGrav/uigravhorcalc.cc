/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Apr 2010
-*/

static const char* rcsID = "$Id: uigravhorcalc.cc,v 1.1 2010-04-19 15:14:27 cvsbert Exp $";

#include "uigravhorcalc.h"
#include "gravhorcalc.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiveldesc.h"
#include "ioman.h"
#include "survinfo.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"


uiGravHorCalc::uiGravHorCalc( uiParent* p, EM::ObjectID enobjid )
    : uiDialog(p,Setup("Calculate Gravity","", mTODOHelpID))
    , horid_(EM::EMM().getMultiID(enobjid))
    , velfld_(0)
{
    hornm_ = IOM().nameOf( horid_ );
    setTitleText( BufferString("Calculate gravity at '", hornm_, "'" ) );

    const IOObjContext ctxt( mIOObjContext(EMHorizon3D) );
    uiIOObjSel::Setup su( "Top Horizon" );
    su.optional_ = true;
    uiGroup* inpgrp = new uiGroup( this, "Upper group" );

    topfld_ = new uiIOObjSel( inpgrp, ctxt, su );
    topfld_->setInput( horid_ );
    topfld_->selectionDone.notify( mCB(this,uiGravHorCalc,topSel) );

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
	IOObjContext velctxt = uiVelSel::ioContext();
	velctxt.forread = true;
	uiSeisSel::Setup velsetup( Seis::Vol );
	velsetup.seltxt( "Velocity model" );
	velfld_ = new uiVelSel( inpgrp, velctxt, velsetup );
	velfld_->attach( alignedBelow, botfld_ );
    }

    cutoffangfld_ = new uiGenInput( this, "Cutoff angle (deg)",
	    			    IntInpSpec(45) );
    cutoffangfld_->attach( alignedBelow, inpgrp );

    attrnmfld_ = new uiGenInput( this, "Output attribute",
	    			 StringInpSpec("Grav") );
    attrnmfld_->attach( alignedBelow, cutoffangfld_ );
    uiLabel* lbl = new uiLabel( this, BufferString("(on '",hornm_,"')") );
    lbl->attach( rightOf, attrnmfld_ );

    finaliseDone.notify( mCB(this,uiGravHorCalc,initFlds) );
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
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGravHorCalc::acceptOK( CallBacker* )
{
    return false;
}
