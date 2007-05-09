
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
-*/

static const char* rcsID = "$Id: uitutseistools.cc,v 1.5 2007-05-09 15:58:49 cvsbert Exp $";

#include "uitutseistools.h"
#include "tutseistools.h"
#include "uiseissel.h"
#include "uigeninput.h"
#include "uiexecutor.h"
#include "uimsg.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "seissingtrcproc.h"
#include "ctxtioobj.h"
#include "ioobj.h"


uiTutSeisTools::uiTutSeisTools( uiParent* p )
	: uiDialog( p, Setup( "Tut seismic tools",
			      "Specify process parameters",
			      "0.0.0") )
    	, inctio_(*mMkCtxtIOObj(SeisTrc))
    	, outctio_(*mMkCtxtIOObj(SeisTrc))
    	, tst_(*new Tut::SeisTools)
    	, stp_(0)
{
    const CallBack choicecb( mCB(this,uiTutSeisTools,choiceSel) );

    // The input seismic object
    inpfld_ = new uiSeisSel( this, inctio_, SeisSelSetup() );

    // What seismic tool is required?
    static const char* choices = { "Scale", "Square", "Smooth", 0 };
    choicefld_ = new uiGenInput( this, "Action",
	    			 StringListInpSpec(choices) );
    choicefld_->valuechanged.notify( choicecb );

    // Parameters for scaling
    scalegrp_ = new uiGroup( this, "Scale group" );
    scalegrp_->attach( alignedBelow, inpfld_ );
    factorfld_ = new uiGenInput( scalegrp_, "Factor",
				FloatInpSpec(tst_.factor()) );
    shiftfld_ = new uiGenInput( scalegrp_, "Shift",
	    			FloatInpSpec(tst_.shift()) );
    shiftfld_->attach( alignedBelow, factorfld_ );
    scalegrp_->setHAlignObj( factorfld_ );

    // Parameters for smoothing
    smoothszfld_ = new uiGenInput( this, "Filter Size",
	    			   BoolInpSpec(true,"3","5") );
    smoothszfld_->attach( alignedBelow, choicefld_ );

    // The output seismic object
    outctio_.ctxt.forread = false;
    outfld_ = new uiSeisSel( this, outctio_, SeisSelSetup() );
    outfld_->attach( leftAlignedBelow, factorfld_ );

    // Make sure only relevant stuff is displayed on startup
    finaliseDone.notify( choicecb );
}


uiTutSeisTools::~uiTutSeisTools()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
    delete stp_;
    delete &tst_;
}


void uiTutSeisTools::choiceSel( CallBacker* )
{
    const int selection = choicefld_->getIntValue();
    scalegrp_->display( selection == 0 );
    smoothszfld_->display( selection == 2 );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiTutSeisTools::acceptOK( CallBacker* )
{
    if ( !inpfld_->commitInput(false) )
	mErrRet("Missing Input\nPlease select the input seismics")
    if ( !outfld_->commitInput(true) )
	mErrRet("Missing Output\nPlease enter a name for the output seismics")

    const int selection = choicefld_->getIntValue();
    tst_.setTool( selection );
    if ( selection == 0 )
    {
	float usrfactor = factorfld_->getfValue();
	if ( mIsUdf(usrfactor) ) usrfactor = 1;
	tst_.setFactor( usrfactor );

	float usrshift = shiftfld_->getfValue();
	if ( mIsUdf(usrshift) ) usrshift = 0;
	tst_.setShift( usrshift );
    }
    else if ( selection == 2 )
	tst_.setSmallFilt( smoothszfld_->getBoolValue() );

    stp_ = new SeisSingleTraceProc( inctio_.ioobj, outctio_.ioobj );
    stp_->setProcessingCB( mCB(this,uiTutSeisTools,doProc) );

    uiExecutor dlg( this, *stp_ );
    return dlg.go();
}


void uiTutSeisTools::doProc( CallBacker* )
{
    tst_.apply( stp_->getTrace() );
}
