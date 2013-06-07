
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh / Karthika
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uituthortools.cc,v 1.15 2011/11/23 11:35:55 cvsbert Exp $";

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
{
    taskfld_= new uiGenInput( this, "Task",
	    		BoolInpSpec(true,"Thickness between two horizons",
			    		 "Smooth a horizon") );
    taskfld_->valuechanged.notify( mCB(this,uiTutHorTools,choiceSel) );

    IOObjContext ctxt = mIOObjContext(EMHorizon3D);
    inpfld_ = new uiIOObjSel( this, ctxt, "Input Horizon" );
    inpfld_->attach( alignedBelow, taskfld_ );

    // For thickness calculation
    inpfld2_ = new uiIOObjSel( this, ctxt, "Bottom Horizon" );
    inpfld2_->attach( alignedBelow, inpfld_ );

    selfld_= new uiGenInput( this, "Add Result as an Attribute to ",
	                BoolInpSpec(true, "Top Horizon", "Bottom Horizon") ); 
    selfld_->attach( alignedBelow, inpfld2_ );

    attribnamefld_ = new uiGenInput( this, "Attribute name",
	    		StringInpSpec( "Thickness" ) );
    attribnamefld_->attach( alignedBelow, selfld_ );

    // For smoothing
    ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctxt, "Output Horizon" );
    outfld_->attach( alignedBelow, inpfld_ );

    strengthfld_ = new uiGenInput( this, "Filter Strength",
	    		BoolInpSpec(true, "Low", "High") );
    strengthfld_->attach( alignedBelow, outfld_ );

    postFinalise().notify( mCB(this,uiTutHorTools,choiceSel) );
}


uiTutHorTools::~uiTutHorTools()
{
}


void uiTutHorTools::choiceSel( CallBacker* )
{
    const bool isthick = taskfld_->getBoolValue();
    inpfld_->setLabelText( isthick ? "Top Horizon" : "Input Horizon" );
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
    EM::EMM().getSurfaceData( key, sd );
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
	EM::EMM().loadIfNotFullyLoaded( (fld)->key(), &tr ); \
    mDynamicCastGet(EM::Horizon3D*,varnm,varnm##_emobj.ptr()) \
    if ( !varnm ) return false;

bool uiTutHorTools::doThicknessCalc()
{
    if ( inpfld_->key() == inpfld2_->key()
	    && !uiMSG().askGoOn("Input horizon same as Output. Continue?") )
	return false;

    if ( !inpfld2_->ioobj() )
	return false;

    const bool cont = checkAttribName();
    if ( !cont ) return false;

    uiTaskRunner tr( this );
    Tut::ThicknessCalculator* calc = new Tut::ThicknessCalculator;
    const bool top = selfld_->getBoolValue();
    mGetHor( hor1, top ? inpfld_ : inpfld2_ );
    mGetHor( hor2, top ? inpfld2_ : inpfld_ );
    calc->setHorizons( hor1, hor2 );
    calc->init( attribnamefld_->text() );

    if ( !tr.execute(*calc) )
	return false;

    PtrMan<Executor> saver = calc->dataSaver();
    return tr.execute( *saver );
}


bool uiTutHorTools::doSmoother()
{
    if ( !outfld_->ioobj() )
	return false;

    uiTaskRunner tr( this );
    Tut::HorSmoother* calc = new Tut::HorSmoother;
    mGetHor( hor, inpfld_ );
    calc->setHorizons( hor );
    calc->setWeak( strengthfld_->getBoolValue() ); 

    if ( !tr.execute(*calc) )
	return false;

    PtrMan<Executor> saver = calc->dataSaver( outfld_->key() );
    return tr.execute( *saver );
}
