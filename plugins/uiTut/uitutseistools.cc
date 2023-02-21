/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitutseistools.h"

#include "ctxtioobj.h"
#include "ioobj.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "seistype.h"
#include "seisselection.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "tutseistools.h"

#include "uigeninput.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uitaskrunner.h"

static const char* actions[] = { "Scale", "Square", "Smooth",
				 "Replace sampling", nullptr };
// Exactly the order of the Tut::SeisTools::Action enum

uiTutSeisTools::uiTutSeisTools( uiParent* p, Seis::GeomType gt )
    : uiDialog( p, Setup( tr("Tut seismic tools"),
			    tr("Specify process parameters"),
			    HelpKey("tut","seis") ) )
    , geom_(gt)
    , tst_(*new Tut::SeisTools)
{
    // The input seismic object
    inpfld_ = new uiSeisSel( this, uiSeisSel::ioContext(geom_,true),
				uiSeisSel::Setup(geom_) );
    mAttachCB( inpfld_->selectionDone, uiTutSeisTools::inpSel );

    subselfld_ = uiSeisSubSel::get( this, Seis::SelSetup(geom_) );

    subselfld_->attachObj()->attach( alignedBelow, inpfld_ );
    // What seismic tool is required?
    actionfld_ = new uiGenInput( this, uiStrings::sAction(),
				 StringListInpSpec(actions) );
    mAttachCB( actionfld_->valueChanged, uiTutSeisTools::choiceSel );
    actionfld_->attach( centeredBelow, subselfld_ );

    // Parameters for scaling
    scalegrp_ = new uiGroup( this, "Scale group" );
    scalegrp_->attach( alignedBelow, actionfld_ );
    factorfld_ = new uiGenInput( scalegrp_, tr("Factor"),
				FloatInpSpec(tst_.factor()) );
    shiftfld_ = new uiGenInput( scalegrp_, uiStrings::sShift(),
				FloatInpSpec(tst_.shift()) );
    shiftfld_->attach( alignedBelow, factorfld_ );
    scalegrp_->setHAlignObj( factorfld_ );

    // Parameters for smoothing
    smoothszfld_ = new uiGenInput( this, tr("Filter strength"),
			           BoolInpSpec(tst_.weakSmoothing(),tr("Low"),
                                   tr("High")) );
    smoothszfld_->attach( alignedBelow, actionfld_ );

    // Parameters for change sample rate

    newsdfld_ = new uiGenInput( this, tr("New sampling ")
				    .arg(SI().getZUnitString()), FloatInpSpec(),
				FloatInpSpec() );
    newsdfld_->attach( alignedBelow, actionfld_ );

    // The output seismic object
    outfld_ = new uiSeisSel( this, uiSeisSel::ioContext(geom_,false),
				uiSeisSel::Setup(geom_) );
    outfld_->attach( alignedBelow, scalegrp_ );

    // Make sure only relevant stuff is displayed on startup
    mAttachCB( postFinalize(), uiTutSeisTools::choiceSel );
}


uiTutSeisTools::~uiTutSeisTools()
{
    detachAllNotifiers();
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
    const uiString outputtype = uiStrings::phrOutput(uiStrings::sVolume());
    // Get cubes and check
    const IOObj* inioobj = inpfld_->ioobj();
    if ( !inioobj )
	return false; // Error messages already handled

    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
	return false;
    else if ( outioobj->implExists(false)
	     && !uiMSG().askGoOn(
		    uiStrings::phrExistsContinue( outputtype, true) ) )
	return false;

    tst_.setInput( *inioobj );
    tst_.setOutput( *outioobj );

    TrcKeyZSampling  cs;
    subselfld_->getSampling( cs.hsamp_ );
    subselfld_->getZRange( cs.zsamp_ );
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
	float usrfactor = factorfld_->getFValue();
	if ( mIsUdf(usrfactor) )
	    usrfactor = 1;

	float usrshift = shiftfld_->getFValue();
	if ( mIsUdf(usrshift) )
	    usrshift = 0;

	tst_.setScale( usrfactor, usrshift );
    }
    break;
    case Tut::SeisTools::ChgSD:
    {
	SamplingData<float> sd( newsdfld_->getFValue(0),
				newsdfld_->getFValue(1) );
	const float fac = 1.f / SI().zDomain().userFactor();
	sd.start *= fac;
	sd.step *= fac;
	tst_.setSampling( sd );
    }
    break;
    default: // No parameters to set
    break;
    }

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(tst_) )
    {
	uiMSG().error( tr("Output cannot be created") );
	return false;
    }

    const bool ret = uiMSG().askGoOn(
	    tr("Process finished successfully. Do you want to continue?") );
    return !ret;
}


void uiTutSeisTools::inpSel( CallBacker* )
{
    const IOObj* inioobj = inpfld_->ioobj( true );
			// 'true' prevents error message if nothing selected
    if ( inioobj )
	subselfld_->setInput( *inioobj );
}
