/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
-*/

static const char* rcsID mUsedVar = "$Id$";
#include "cubesampling.h"
#include "uitutseistools.h"
#include "tutseistools.h"
#include "uiseissel.h"
#include "uigeninput.h"
#include "uiseissubsel.h"
#include "uitaskrunner.h"
#include "uimsg.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "seistype.h"
#include "seisselection.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "survinfo.h"

static const char* actions[] = { "Scale", "Square", "Smooth",
    				 "Replace sampling", 0 };
// Exactly the order of the Tut::SeisTools::Action enum

uiTutSeisTools::uiTutSeisTools( uiParent* p, Seis::GeomType gt )
	: uiDialog( p, Setup( "Tut seismic tools",
			      "Specify process parameters",
			      "tut:105.0.1") )
    	, inctio_(*mMkCtxtIOObj(SeisTrc))
    	, outctio_(*mMkCtxtIOObj(SeisTrc))
    	, geom_(gt)
    	, tst_(*new Tut::SeisTools)
{
    const CallBack choicecb( mCB(this,uiTutSeisTools,choiceSel) );
    const CallBack inpcb( mCB(this,uiTutSeisTools,inpSel) );

    // The input seismic object
    inpfld_ = new uiSeisSel( this, inctio_, uiSeisSel::Setup(geom_) );
    inpfld_->selectionDone.notify( inpcb );
    
    subselfld_ = uiSeisSubSel::get( this, Seis::SelSetup(geom_) );
    
    subselfld_->attachObj()->attach( alignedBelow, inpfld_ );
    // What seismic tool is required?
    actionfld_ = new uiGenInput( this, "Action",
	    			 StringListInpSpec(actions) );
    actionfld_->valuechanged.notify( choicecb );
    actionfld_->attach(centeredBelow, subselfld_ );

    // Parameters for scaling
    scalegrp_ = new uiGroup( this, "Scale group" );
    scalegrp_->attach( alignedBelow, actionfld_ );
    factorfld_ = new uiGenInput( scalegrp_, "Factor",
				FloatInpSpec(tst_.factor()) );
    shiftfld_ = new uiGenInput( scalegrp_, "Shift",
	    			FloatInpSpec(tst_.shift()) );
    shiftfld_->attach( alignedBelow, factorfld_ );
    scalegrp_->setHAlignObj( factorfld_ );

    // Parameters for smoothing
    smoothszfld_ = new uiGenInput( this, "Filter strength",
			       BoolInpSpec(tst_.weakSmoothing(),"Low","High") );
    smoothszfld_->attach( alignedBelow, actionfld_ );

    // Parameters for change sample rate

    newsdfld_ = new uiGenInput( this, BufferString("New sampling ",
				SI().getZUnitString()), FloatInpSpec(),
	    			FloatInpSpec() );
    newsdfld_->attach( alignedBelow, actionfld_ );

    // The output seismic object
    outctio_.ctxt.forread = false;
    outfld_ = new uiSeisSel( this, outctio_, uiSeisSel::Setup(geom_) );
    outfld_->attach( alignedBelow, scalegrp_ );
    
    // Make sure only relevant stuff is displayed on startup
    postFinalise().notify( choicecb );
}


uiTutSeisTools::~uiTutSeisTools()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
    delete &tst_;
}


void uiTutSeisTools::choiceSel( CallBacker* )
{
    const Tut::SeisTools::Action act
			= (Tut::SeisTools::Action)actionfld_->getIntValue();

    scalegrp_->display( act == Tut::SeisTools::Scale );
    smoothszfld_->display( act == Tut::SeisTools::Smooth );
    newsdfld_->display( act == Tut::SeisTools::ChgSD );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiTutSeisTools::acceptOK( CallBacker* )
{
    // Get cubes and check
    if ( !inpfld_->commitInput() )
	mErrRet("Missing Input\nPlease select the input seismics")
    if ( !outfld_->commitInput() )
	mErrRet("Missing Output\nPlease enter a name for the output seismics")
    else if ( outctio_.ioobj->implExists(false)
	   && !uiMSG().askGoOn("Output cube exists. Overwrite?") )
	return false;

    tst_.clear();
    tst_.setInput( *inctio_.ioobj );
    tst_.setOutput( *outctio_.ioobj );

    CubeSampling  cs;
    subselfld_->getSampling( cs.hrg );
    subselfld_->getZRange( cs.zrg );
    tst_.setRange( cs );

    // Set action-specific parameters
    tst_.setAction( (Tut::SeisTools::Action)actionfld_->getIntValue() );
    switch ( tst_.action() )
    {
    case Tut::SeisTools::Smooth:
	tst_.setWeakSmoothing( smoothszfld_->getBoolValue() );
    break;
    case Tut::SeisTools::Scale:
    {
	float usrfactor = factorfld_->getfValue();
	if ( mIsUdf(usrfactor) ) usrfactor = 1;
	float usrshift = shiftfld_->getfValue();
	if ( mIsUdf(usrshift) ) usrshift = 0;
	tst_.setScale( usrfactor, usrshift );
    }
    break;
    case Tut::SeisTools::ChgSD:
    {
	SamplingData<float> sd( newsdfld_->getfValue(0),
				newsdfld_->getfValue(1) );
	const float fac = 1. / SI().zDomain().userFactor();
	sd.start *= fac; sd.step *= fac;
	tst_.setSampling( sd );
    }
    break;
    default: // No parameters to set
    break;
    }

    uiTaskRunner taskrunner( this );
    return taskrunner.execute( tst_ );
}


void uiTutSeisTools::inpSel( CallBacker* )
{
    if ( !inpfld_->commitInput() || !inctio_.ioobj ) return;

    subselfld_->setInput( *inctio_.ioobj );
}
