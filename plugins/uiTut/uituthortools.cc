
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uituthortools.cc,v 1.6 2007-12-10 12:59:52 cvsbert Exp $";

#include "uituthortools.h"
#include "ctxtioobj.h"
#include "transl.h"
#include "ioobj.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "emsurfacetr.h"
#include "uiexecutor.h"

uiTutHorTools::uiTutHorTools( uiParent* p )
	: uiDialog( p, Setup( "Tut Horizon tools",
			      "Specify process parameters",
			      "tut:105.0.2") )
    	, inctio_(*mGetCtxtIOObj(EMHorizon3D,Surf))
	, inctio2_(*mMkCtxtIOObj(EMHorizon3D))
    	, outctio_(*mMkCtxtIOObj(EMHorizon3D))
	, thickcalc_(0)
	, smoothnr_(0)
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

    smoothfld_ = new uiGenInput( this, "Filter Strength",
	    		BoolInpSpec(true, "Low", "High") );
    smoothfld_->attach( alignedBelow, outfld_ );

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
    if( smoothnr_ ) delete smoothnr_;
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
    smoothfld_->display( mono );
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


bool uiTutHorTools::initHorSmoothener()
{
    smoothnr_ = new Tut::HorSmoothener;
    EM::Horizon3D* hor = loadHor(inctio_.ioobj);
    if ( !hor )
	return false;
    smoothnr_->setHorizons( hor );
    smoothnr_->setWeaksmooth( smoothfld_->getBoolValue() ); 
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
    uiExecutor dlg( this, *exec );
    dlg.go();
    EM::EMObject* emobj = em.getObject( em.getObjectID(ioobj->key()) );
    emobj->ref();
    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    return horizon;
}


void uiTutHorTools::saveData(bool geom)
{
    PtrMan<Executor> exec;
    if ( geom )
	exec = smoothnr_->dataSaver( outctio_.ioobj->key() );
    else
	exec = thickcalc_->dataSaver();

    uiExecutor dlg( this, *exec );
    dlg.go();
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiTutHorTools::acceptOK( CallBacker* )
{
    const bool smooth = taskfld_->getBoolValue();
    bool retval = 0;
    if( smooth )
    {
	if ( !inpfld_->commitInput(false) )
	    mErrRet("Missing Input\nPlease select the input horizon")
	if ( !outfld_->commitInput(true) )
	    mErrRet("Missing Output\nPlease select the output horizon")
	else if ( outctio_.ioobj->implExists(false)
	 	&& !uiMSG().askGoOn("Output horizon exists. Overwrite?") )
        return false;

	if ( !initHorSmoothener() )
	    return false; 
	uiExecutor dlg( this, *smoothnr_ );
	retval = dlg.go();
    }
    else
    {
	if( !inpfld_->commitInput(false) )
	    mErrRet("Missing Input\nPlease select the Top Horizon")
	if( !inpfld2_->commitInput(false) )
	    mErrRet("Missing Input\nPlease select the Bottom Horizon")
	if( inctio_.ioobj->key()==inctio2_.ioobj->key()
		&& !uiMSG().askGoOn("Input horizon same as Output. Continue?") )
	return false; 

	if ( !initThicknessFinder() )
	    return false;
	uiExecutor dlg( this, *thickcalc_ );
	retval = dlg.go();
    }

    if ( retval )
	saveData( smooth );
    return retval;
}

										
