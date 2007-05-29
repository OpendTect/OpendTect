

/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : May 2007
-*/

static const char* rcsID = "$ $";

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
			      "0.0.0") )
    	, inctio_(*mMkCtxtIOObj(EMHorizon3D))
	, inctio2_(*mMkCtxtIOObj(EMHorizon3D))
    	, outctio_(*mMkCtxtIOObj(EMHorizon3D))
	, tst_(0)
	, ftr_(0)
{
    taskfld_= new uiGenInput( this, "Select Task",
	    		BoolInpSpec(true, "Smooth a Horizon", 
			    	    "Find Thickness between two Horizons") );
    taskfld_->valuechanged.notify( mCB(this,uiTutHorTools,choiceSel) );
    
    inpfld_ = new uiIOObjSel( this, inctio_, "  Input Horizon  " );
    inpfld_->attach( alignedBelow, taskfld_ );

    inpfld2_ = new uiIOObjSel( this, inctio2_, "Input Bottom Horizon" );
    inpfld2_->attach( alignedBelow, inpfld_ );

    selfld_= new uiGenInput( this, "Add Result as an Attribute to ",
	                BoolInpSpec(true, "Top Horizon", "Bottom Horizon") ); 
    selfld_->attach( alignedBelow, inpfld2_ );

    outctio_.ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, outctio_, "Output Horizon" );
    outfld_->attach( alignedBelow, selfld_ );

    finaliseDone.notify( mCB(this,uiTutHorTools,choiceSel) );
}


uiTutHorTools::~uiTutHorTools()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
    if( tst_ ) delete tst_;
    if( ftr_ ) delete ftr_;
}


void uiTutHorTools::choiceSel( CallBacker* )
{
    const bool mono = taskfld_->getBoolValue();
    inpfld_->setLabelText( mono ? "  Input Horizon  " : "Input Top Horizon" );
    inpfld_->clear();
    selfld_->display( !mono );
    inpfld2_->display( !mono );
    outfld_->display( mono );
}


void uiTutHorTools::fillParThickness()
{
    tst_ = new Tut::HorTools;
    EM::Horizon3D* hor = loadHor(inctio_.ioobj);
    tst_->setTop( hor );
    hor = loadHor(inctio2_.ioobj);
    tst_->setBot( hor ); 
    tst_->setOutHor( selfld_->getBoolValue() );

    EM::Horizon3D* outhor = tst_->getOutHor();
    const int idx = outhor->auxdata.addAuxData( "Thickness" );
    tst_->setIdx( idx );
    tst_->setId( outhor->id() );
    StepInterval<int> inlrg = outhor->geometry().rowRange();
    StepInterval<int> crlrg = outhor->geometry().colRange();
    tst_->setHorSamp( inlrg, crlrg );
}


void uiTutHorTools::fillParFilter()
{
    ftr_ = new Tut::HorFilter;
    EM::Horizon3D* hor = loadHor(inctio_.ioobj);
    ftr_->setInput( hor );
    StepInterval<int> inlrg = hor->geometry().rowRange();
    StepInterval<int> crlrg = hor->geometry().colRange();
    ftr_->setHorSamp( inlrg, crlrg );
}

EM::Horizon3D* uiTutHorTools::loadHor( IOObj* ioobj )
{
    EM::EMManager& em = EM::EMM();
    PtrMan<Executor> exec = em.objectLoader( ioobj->key() );
    uiExecutor dlg( this, *exec );
    dlg.go();
    EM::EMObject* emobj = em.getObject( em.getObjectID(ioobj->key()) );
    emobj->ref();
    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    return horizon;
}


void uiTutHorTools::saveData(bool geom)
{
    const MultiID* id = geom ? new MultiID( outctio_.ioobj->key() ) : 0;
    PtrMan<Executor> saver = geom ? ftr_->getHor()->geometry().saver( 0, id ) : 
				    tst_->getOutHor()->auxdata.auxDataSaver();
    saver->execute();
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiTutHorTools::acceptOK( CallBacker* )
{
    bool smooth = taskfld_->getBoolValue();
    bool retval = 0;
    if( smooth )
    {
	if ( !inpfld_->commitInput(false) )
	    mErrRet("Missing Input\nPlease select the input horizon")
	if ( !outfld_->commitInput(true) )
	    mErrRet("Missing Output\nPlease enter a name for the output horizon")
	else if ( outctio_.ioobj->implExists(false)
	 	&& !uiMSG().askGoOn("Output horizon exists. Overwrite?") )
        return false;
	fillParFilter();
	uiExecutor dlg( this, *ftr_ );
	retval = dlg.go();
    }
    else
    {
	if( !inpfld_->commitInput(false) )
	    mErrRet("Missing Input\nPlease select the Top Horizon")
	if( !inpfld2_->commitInput(false) )
	    mErrRet("Missing Input\nPlease select the Bottom Horizon")
	fillParThickness();
	uiExecutor dlg( this, *tst_ );
	retval = dlg.go();
    }
    
    saveData( smooth );
    return retval;
}


