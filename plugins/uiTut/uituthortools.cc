
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh / Karthika
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uituthortools.cc,v 1.13 2009-07-22 16:01:28 cvsbert Exp $";

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
    taskfld_= new uiGenInput( this, "Task",
	    		BoolInpSpec(true,"Thickness between two horizons",
			    		 "Smooth a horizon") );
    taskfld_->valuechanged.notify( mCB(this,uiTutHorTools,choiceSel) );
    
     
    inpfld_ = new uiIOObjSel( this, inctio_, "Input Horizon" );
    inpfld_->attach( alignedBelow, taskfld_ );

    outctio_.ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, outctio_, "Output Horizon" );
    outfld_->attach( alignedBelow, inpfld_ );

    strengthfld_ = new uiGenInput( this, "Filter Strength",
	    		BoolInpSpec(true, "Low", "High") );
    strengthfld_->attach( alignedBelow, outfld_ );

    inpfld2_ = new uiIOObjSel( this, inctio2_, "Bottom Horizon" );
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


#define mIsThick (taskfld_->getBoolValue())


void uiTutHorTools::choiceSel( CallBacker* )
{
    const bool isthick = mIsThick;

    inpfld_->setLabelText( isthick ? "Top Horizon" : "Input Horizon" );
    inpfld2_->display( isthick );
    selfld_->display( isthick );
    attribnamefld_->display( isthick );
    outfld_->display( !isthick );
    strengthfld_->display( !isthick );
}


#define mGetHor(varnm,ioobj) \
    EM::EMObject* varnm##_emobj = \
	EM::EMM().loadIfNotFullyLoaded( (ioobj)->key(), tr ); \
    mDynamicCastGet(EM::Horizon3D*,varnm,varnm##_emobj) \
    if ( !varnm ) \
	return false; \
    varnm->ref()

bool uiTutHorTools::initThicknessCalculator( uiTaskRunner* tr )
{
    thickcalc_ = new Tut::ThicknessCalculator;
    const bool top = selfld_->getBoolValue();
    mGetHor(hor1,top ? inctio_.ioobj : inctio2_.ioobj);
    mGetHor(hor2,top ? inctio2_.ioobj : inctio_.ioobj);

    thickcalc_->setHorizons( hor1, hor2 );
    thickcalc_->init( attribnamefld_->text() );
    return true;
}


bool uiTutHorTools::initHorSmoother( uiTaskRunner* tr )
{
    smoother_ = new Tut::HorSmoother;
    mGetHor(hor,inctio_.ioobj);
    smoother_->setHorizons( hor );
    smoother_->setWeak( strengthfld_->getBoolValue() ); 
    return true;
}


EM::Horizon3D* uiTutHorTools::loadHor( const IOObj& ioobj )
{
    EM::EMManager& em = EM::EMM();
    PtrMan<Executor> exec = em.objectLoader( ioobj.key() );
    if ( !exec )
    {
	BufferString errmsg = "Cannot Find Object  ";
        errmsg += ioobj.name();
        uiMSG().error( errmsg );
	return 0;
    }

    uiTaskRunner taskrunner( this );
    taskrunner.execute( *exec );
    EM::EMObject* emobj = em.getObject( em.getObjectID(ioobj.key()) );
    emobj->ref();

    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    return horizon;
}


void uiTutHorTools::saveData( bool isthick )
{
    PtrMan<Executor> exec = isthick ? thickcalc_->dataSaver()
			    : smoother_->dataSaver(outctio_.ioobj->key());

    if ( !exec )
	{ uiMSG().error( "Cannot save Horizon" ); return; }

    uiTaskRunner taskrunner( this );
    taskrunner.execute( *exec );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiTutHorTools::acceptOK( CallBacker* )
{
    const bool isthick = mIsThick;

    const bool inpok = inpfld_->commitInput()
		    && (!isthick || inpfld2_->commitInput());
    if ( !inpok )
	mErrRet("\nPlease select all input horizons")

    uiTaskRunner taskrunner( this );
    if ( isthick )
    {
	if( !inpfld2_->commitInput() )
	    mErrRet("")
	if( inctio_.ioobj->key()==inctio2_.ioobj->key()
		&& !uiMSG().askGoOn("Input horizon same as Output. Continue?") )
	    return false;
	if ( !initThicknessCalculator( &taskrunner ) )
	    return false;
    }
    else
    {
	if ( !outfld_->commitInput() )
	    mErrRet("Missing Output\nPlease select the output horizon")
	else if ( outctio_.ioobj->implExists(false)
	 	&& !uiMSG().askGoOn("Output horizon exists. Overwrite?") )
	    return false;
	if ( !initHorSmoother( &taskrunner ) )
	    return false; 
    }

    Executor* toexec = isthick ? (Executor*)thickcalc_ : (Executor*)smoother_;
    bool retval = taskrunner.execute( *toexec );
    if ( retval )
	saveData( isthick );

    return retval;
}
