
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uituthortools.cc,v 1.11 2009-04-09 11:49:08 cvsranojay Exp $";

#include "uituthortools.h"
#include "tuthortools.h"

#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfacetr.h"
#include "ioobj.h"
#include "transl.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uitaskrunner.h"

uiTutHorTools::uiTutHorTools( uiParent* p )
	: uiDialog( p, Setup( "Tut Horizon tools",
			      "Specify process parameters",
			      "tut:105.0.2") )
    	, inctio_(*mMkCtxtIOObj(EMHorizon3D))
	, inctio2_(*mMkCtxtIOObj(EMHorizon3D))
    	, outctio_(*mMkCtxtIOObj(EMHorizon3D))
	, thickcalc_(0)
	, smoother_(0)
{
    taskfld_= new uiGenInput( this, "Select Task",
	    		BoolInpSpec(true, "Smooth a Horizon", 
			    	    "Find Thickness between two Horizons") );
    taskfld_->valuechanged.notify( mCB(this,uiTutHorTools,choiceSel) );
    
    inpfld_ = new uiIOObjSel( this, inctio_, "  Input Horizon  " );
    inpfld_->attach( alignedBelow, taskfld_ );

    outctio_.ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, outctio_, "Output Horizon" );
    outfld_->attach( alignedBelow, inpfld_ );

    strengthfld_ = new uiGenInput( this, "Filter Strength",
	    		BoolInpSpec(true, "Low", "High") );
    strengthfld_->attach( alignedBelow, outfld_ );

    inpfld2_ = new uiIOObjSel( this, inctio2_, "Input Bottom Horizon" );
    inpfld2_->attach( alignedBelow, inpfld_ );

    selfld_= new uiGenInput( this, "Add Result as an Attribute to ",
	                BoolInpSpec(true, "Top Horizon", "Bottom Horizon") ); 
    selfld_->attach( alignedBelow, inpfld2_ );

    attribnamefld_ = new uiGenInput( this, "Attribute name",
	    		StringInpSpec( "Thickness" ) );
    attribnamefld_->attach( alignedBelow, selfld_ );

    finaliseDone.notify( mCB(this,uiTutHorTools,choiceSel) );
}


uiTutHorTools::~uiTutHorTools()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
    if( thickcalc_ ) delete thickcalc_;
    if( smoother_ ) delete smoother_;
}


void uiTutHorTools::choiceSel( CallBacker* )
{
    const bool mono = taskfld_->getBoolValue();
    inpfld_->setLabelText( mono ? "  Input Horizon  " : "Input Top Horizon" );
    inpfld_->clear();
    selfld_->display( !mono );
    inpfld2_->display( !mono );
    attribnamefld_->display( !mono );
    outfld_->display( mono );
    strengthfld_->display( mono );
}


bool uiTutHorTools::initThicknessFinder()
{
    thickcalc_ = new Tut::ThicknessFinder;
    const bool top = selfld_->getBoolValue();
    EM::Horizon3D* hor1 = loadHor( top ? inctio_.ioobj : inctio2_.ioobj );
    if ( !hor1 )
	return false;

    EM::Horizon3D* hor2 = loadHor( top ? inctio2_.ioobj : inctio_.ioobj );
    if ( !hor2 )
	return false;

    thickcalc_->setHorizons( hor1, hor2 );
    thickcalc_->init( attribnamefld_->text() );
    return true;
}


bool uiTutHorTools::initHorSmoother()
{
    smoother_ = new Tut::HorSmoother;
    EM::Horizon3D* hor = loadHor(inctio_.ioobj);
    if ( !hor )
	return false;
    smoother_->setHorizons( hor );
    smoother_->setWeak( strengthfld_->getBoolValue() ); 
    return true;
}


EM::Horizon3D* uiTutHorTools::loadHor( const IOObj* ioobj )
{
    if ( !ioobj )
	return 0;

    EM::EMManager& em = EM::EMM();
    PtrMan<Executor> exec = em.objectLoader( ioobj->key() );
    if ( !exec )
    {
	BufferString errmsg = "Cannot Find Object  ";
        errmsg += ioobj->name();
        uiMSG().error( errmsg );
	return 0;
    }
    uiTaskRunner taskrunner( this );
    taskrunner.execute( *exec );
    EM::EMObject* emobj = em.getObject( em.getObjectID(ioobj->key()) );
    emobj->ref();
    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    return horizon;
}


void uiTutHorTools::saveData(bool geom)
{
    PtrMan<Executor> exec = geom ? smoother_->dataSaver(outctio_.ioobj->key())
				 : thickcalc_->dataSaver();

    if ( !exec )
    { uiMSG().error( "Cannot save Horizon" ); return; }

    uiTaskRunner taskrunner( this );
    taskrunner.execute( *exec );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiTutHorTools::acceptOK( CallBacker* )
{
    const bool smooth = taskfld_->getBoolValue();
    bool retval = 0;
    if( smooth )
    {
	if ( !inpfld_->commitInput() )
	    mErrRet("Missing Input\nPlease select the input horizon")
	if ( !outfld_->commitInput() )
	    mErrRet("Missing Output\nPlease select the output horizon")
	else if ( outctio_.ioobj->implExists(false)
	 	&& !uiMSG().askGoOn("Output horizon exists. Overwrite?") )
        return false;

	if ( !initHorSmoother() )
	    return false; 
	uiTaskRunner taskrunner( this );
	retval = taskrunner.execute( *smoother_ );
    }
    else
    {
	if( !inpfld_->commitInput() )
	    mErrRet("Missing Input\nPlease select the Top Horizon")
	if( !inpfld2_->commitInput() )
	    mErrRet("Missing Input\nPlease select the Bottom Horizon")
	if( inctio_.ioobj->key()==inctio2_.ioobj->key()
		&& !uiMSG().askGoOn("Input horizon same as Output. Continue?") )
	return false; 

	if ( !initThicknessFinder() )
	    return false;
	uiTaskRunner taskrunner( this );
	retval = taskrunner.execute( *thickcalc_ );
    }

    if ( retval )
	saveData( smooth );
    return retval;
}
