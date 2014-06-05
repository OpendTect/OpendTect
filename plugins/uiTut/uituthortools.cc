
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh / Karthika
 * DATE     : May 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uituthortools.h" 
#include "tuthortools.h"

#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfacetr.h"
#include "ioobj.h"
#include "transl.h"
#include "keystrs.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "od_helpids.h"


uiTutHorTools::uiTutHorTools( uiParent* p )
	: uiDialog( p, Setup( tr("Tut Horizon tools"),
			      tr("Specify process parameters"),
			      mNoHelpKey ) )
{
    taskfld_= new uiGenInput( this, tr("Task"),
	    		BoolInpSpec(true,tr("Thickness between two horizons"),
			    		 tr("Smooth a horizon")) );
    taskfld_->valuechanged.notify( mCB(this,uiTutHorTools,choiceSel) );

    IOObjContext ctxt = mIOObjContext(EMHorizon3D);
    inpfld_ = new uiIOObjSel( this, ctxt, "Input Horizon" );
    inpfld_->attach( alignedBelow, taskfld_ );

    // For thickness calculation
    inpfld2_ = new uiIOObjSel( this, ctxt, "Bottom Horizon" );
    inpfld2_->attach( alignedBelow, inpfld_ );

    selfld_= new uiGenInput( this, tr("Add Result as an Attribute to "),
	                BoolInpSpec(true, tr("Top Horizon"), 
                                    tr("Bottom Horizon")) ); 
    selfld_->attach( alignedBelow, inpfld2_ );

    attribnamefld_ = new uiGenInput( this, tr("Attribute name"),
	    		StringInpSpec( sKey::Thickness() ) );
    attribnamefld_->attach( alignedBelow, selfld_ );

    // For smoothing
    ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctxt, "Output Horizon" );
    outfld_->attach( alignedBelow, inpfld_ );

    strengthfld_ = new uiGenInput( this, tr("Filter Strength"),
	    		BoolInpSpec(true, tr("Low"), tr("High")) );
    strengthfld_->attach( alignedBelow, outfld_ );

    postFinalise().notify( mCB(this,uiTutHorTools,choiceSel) );
}


uiTutHorTools::~uiTutHorTools()
{
}


void uiTutHorTools::choiceSel( CallBacker* )
{
    const bool isthick = taskfld_->getBoolValue();
    inpfld_->setLabelText( isthick ? tr("Top Horizon") : tr("Input Horizon") );
    inpfld2_->display( isthick );
    selfld_->display( isthick );
    attribnamefld_->display( isthick );
    outfld_->display( !isthick );
    strengthfld_->display( !isthick );
}


bool uiTutHorTools::checkAttribName() const
{
    int attridx = -1;
    BufferString attrnm = attribnamefld_->text();
    const bool top = selfld_->getBoolValue();
    const MultiID key = top ? inpfld_->key() : inpfld2_->key();
    EM::SurfaceIOData sd;
    uiString errmsg;
    if ( !EM::EMM().getSurfaceData(key,sd,errmsg) )
	return false;

    for ( int idx=0; idx<sd.valnames.size(); idx++ )
    {
	if ( attrnm != sd.valnames.get(idx) )
	    continue;

	attridx = idx;
	break;
    }

    if ( attridx < 0 )
	return true;

    BufferString msg( "This surface already has an attribute called:\n" );
    msg += sd.valnames.get(attridx);
    msg += "\nDo you wish to overwrite this data?";
    return uiMSG().askOverwrite( msg );
}


bool uiTutHorTools::acceptOK( CallBacker* )
{
    if ( !inpfld_->ioobj() ) return false;

    const bool isthick = taskfld_->getBoolValue();
    return isthick ? doThicknessCalc() : doSmoother();
}


#define mGetHor(varnm,fld) \
    RefMan<EM::EMObject> varnm##_emobj = \
	EM::EMM().loadIfNotFullyLoaded( (fld)->key(), &taskrunner ); \
    mDynamicCastGet(EM::Horizon3D*,varnm,varnm##_emobj.ptr()) \
    if ( !varnm ) return false;

bool uiTutHorTools::doThicknessCalc()
{
    if ( inpfld_->key() == inpfld2_->key()
	    && !uiMSG().askGoOn(tr("Input horizon same as Output. Continue?")) )
	return false;

    if ( !inpfld2_->ioobj() )
	return false;

    const bool cont = checkAttribName();
    if ( !cont ) return false;

    uiTaskRunner taskrunner( this );
    Tut::ThicknessCalculator* calc = new Tut::ThicknessCalculator;
    const bool top = selfld_->getBoolValue();
    mGetHor( hor1, top ? inpfld_ : inpfld2_ );
    mGetHor( hor2, top ? inpfld2_ : inpfld_ );
    calc->setHorizons( hor1, hor2 );
    calc->init( attribnamefld_->text() );

    if ( !taskrunner.execute(*calc) )
	return false;

    PtrMan<Executor> saver = calc->dataSaver();
    return taskrunner.execute( *saver );
}


bool uiTutHorTools::doSmoother()
{
    if ( !outfld_->ioobj() )
	return false;

    uiTaskRunner taskrunner( this );
    Tut::HorSmoother* calc = new Tut::HorSmoother;
    mGetHor( hor, inpfld_ );
    calc->setHorizons( hor );
    calc->setWeak( strengthfld_->getBoolValue() ); 

    if ( !taskrunner.execute(*calc) )
	return false;

    PtrMan<Executor> saver = calc->dataSaver( outfld_->key() );
    return taskrunner.execute( *saver );
}
