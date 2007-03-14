
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
-*/

static const char* rcsID = "$Id: uitutseistools.cc,v 1.4 2007-03-14 09:00:47 cvsraman Exp $";

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
	: uiDialog( p, Setup( "Filter seismics Raman-style",
			      "Specify process parameters",
			      "0.0.0")
			      .oktext("Jolly &Good").canceltext("&No way") )
    	, inctio_(*mMkCtxtIOObj(SeisTrc))
    	, outctio_(*mMkCtxtIOObj(SeisTrc))
    	, tst_(*new Tut::SeisTools)
    	, stp_(0)
{
    inpfld_ = new uiSeisSel( this, inctio_, SeisSelSetup() );

    strengthfld_ = new uiGenInput( this, "Scaling Factor",
			       FloatInpSpec(Tut::SeisTools::defaultstrength_) );
    incrementfld_ = new uiGenInput( this, "Shift",
			       FloatInpSpec(Tut::SeisTools::defaultincrement_) );
    strengthfld_->attach( alignedBelow, inpfld_ );
    incrementfld_->attach( rightTo, strengthfld_ );

    outctio_.ctxt.forread = false;
    outfld_ = new uiSeisSel( this, outctio_, SeisSelSetup() );
    outfld_->attach( leftAlignedBelow, strengthfld_ );
}


uiTutSeisTools::~uiTutSeisTools()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
    delete stp_;
    delete &tst_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiTutSeisTools::acceptOK( CallBacker* )
{
    if ( !inpfld_->commitInput(false) )
	mErrRet("Missing Input\nPlease select the input seismics")
    if ( !outfld_->commitInput(true) )
	mErrRet("Missing Output\nPlease enter a name for the ouptut seismics")

    const float usrstrength = strengthfld_->getfValue();
    if ( !mIsUdf(usrstrength) && usrstrength > 0 )
    {
	tst_.setStrength( usrstrength );
	Tut::SeisTools::defaultstrength_ = usrstrength;
    }

    const float usrincrement = incrementfld_->getfValue();
    if ( !mIsUdf(usrincrement) )
    {
	tst_.setIncrement( usrincrement );
	Tut::SeisTools::defaultincrement_ = usrincrement;
    }

    stp_ = new SeisSingleTraceProc( inctio_.ioobj, outctio_.ioobj );
    stp_->setProcessingCB( mCB(this,uiTutSeisTools,doProc) );

    uiExecutor dlg( this, *stp_ );
    return dlg.go();
}


void uiTutSeisTools::doProc( CallBacker* )
{
    tst_.apply( stp_->getTrace() );
}
