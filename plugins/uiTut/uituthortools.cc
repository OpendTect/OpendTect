/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uituthortools.h"
#include "tuthortools.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfacetr.h"
#include "keystrs.h"
#include "transl.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"


uiTutHorTools::uiTutHorTools( uiParent* p )
    : uiDialog( p ,Setup( tr("Tut Horizon tools"),
			  tr("Specify process parameters"),
			  HelpKey("tut","hor") ) )
{
    taskfld_= new uiGenInput( this, tr("Task"),
			  BoolInpSpec(true,tr("Thickness between two horizons"),
			      tr("Smooth a horizon")) );
    mAttachCB( taskfld_->valueChanged, uiTutHorTools::choiceSel );

    inpfld_ = new uiHorizon3DSel( this, true );
    inpfld_->attach( alignedBelow, taskfld_ );

    // For thickness calculation
    inpfld2_ = new uiHorizon3DSel( this, true, uiStrings::sBottomHor() );
    inpfld2_->attach( alignedBelow, inpfld_ );

    selfld_= new uiGenInput( this, tr("Add Result as an Attribute to "),
			     BoolInpSpec(true,uiStrings::sTopHor(),
					 uiStrings::sBottomHor()) );
    selfld_->attach( alignedBelow, inpfld2_ );

    attribnamefld_ = new uiGenInput( this, uiStrings::sAttribName(),
				     StringInpSpec(sKey::Thickness()) );
    attribnamefld_->attach( alignedBelow, selfld_ );

    // For smoothing
    outfld_ = new uiHorizon3DSel( this, false );
    outfld_->attach( alignedBelow, inpfld_ );

    strengthfld_ = new uiGenInput( this, tr("Filter Strength"),
				   BoolInpSpec(true,tr("Low"),tr("High")) );
    strengthfld_->attach( alignedBelow, outfld_ );

    mAttachCB( postFinalize(), uiTutHorTools::choiceSel );
}


uiTutHorTools::~uiTutHorTools()
{
    detachAllNotifiers();
}


void uiTutHorTools::choiceSel( CallBacker* )
{
    const bool isthick = taskfld_->getBoolValue();
    inpfld_->setLabelText( isthick ? uiStrings::sTopHor()
				   : tr("Input Horizon") );
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
    {
	uiMSG().error( errmsg );
	return false;
    }

    for ( int idx=0; idx<sd.valnames.size(); idx++ )
    {
	if ( attrnm != sd.valnames.get(idx) )
	    continue;

	attridx = idx;
	break;
    }

    if ( attridx < 0 )
	return true;

    uiString msg = tr("This surface already has an attribute called:\n%1"
		      "\nDo you wish to overwrite this data?")
			.arg(sd.valnames.get(attridx));
    return uiMSG().askOverwrite( msg );
}


bool uiTutHorTools::acceptOK( CallBacker* )
{
    if ( !inpfld_->ioobj() )
	return false;

    const bool isthick = taskfld_->getBoolValue();
    return isthick ? doThicknessCalc() : doSmoother();
}


#define mGetHor(varnm,fld) \
    RefMan<EM::EMObject> varnm##_emobj = \
	EM::EMM().loadIfNotFullyLoaded( (fld)->key(), &taskrunner ); \
    mDynamicCastGet(EM::Horizon3D*,varnm,varnm##_emobj.ptr()) \
    if ( !varnm ) \
	return false;


bool uiTutHorTools::doThicknessCalc()
{
    if ( inpfld_->key() == inpfld2_->key()
	   && !uiMSG().askGoOn(tr("Input horizon same as Output. Continue?")) )
	return false;

    if ( !inpfld2_->ioobj() )
	return false;

    const bool cont = checkAttribName();
    if ( !cont )
	return false;

    uiTaskRunner taskrunner( this );
    auto* calc = new Tut::ThicknessCalculator;
    const bool top = selfld_->getBoolValue();
    mGetHor( hor1, top ? inpfld_ : inpfld2_ );
    mGetHor( hor2, top ? inpfld2_ : inpfld_ );
    calc->setHorizons( hor1, hor2 );
    calc->setAttribName( attribnamefld_->text() );

    if ( !taskrunner.execute(*calc) )
	return false;

    PtrMan<Executor> saver = calc->dataSaver();

    if ( !taskrunner.execute(*saver) )
    {
	uiMSG().error(tr("Thickness calculation failed"));
	return false;
    }

    const uiString msg = tr("Process finished successfully.\n"
			    "Do you want to continue?");

    return !uiMSG().askGoOn( msg );
}


bool uiTutHorTools::doSmoother()
{
    outfld_->reset();
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
    if ( !taskrunner.execute(*saver) )
    {
	uiMSG().error(tr("Smoothing operation failed"));
	return false;
    }

    const uiString msg = tr("Process finished successfully.\n"
			    "Do you want to continue?");

    return !uiMSG().askGoOn( msg );
}
