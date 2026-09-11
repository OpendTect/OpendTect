/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uituthortools.h"
#include "tuthortools.h"

#include "binidvalset.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfacetr.h"
#include "keystrs.h"
#include "transl.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"

#include "uiodapplmgr.h"
#include "uiempartserv.h"


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

    uiStringSet options;
    options.add( tr("BinIDValueSet Importer") )
	   .add( tr("SetArray2D") )
	   .add( tr("Direct Geometry Element") );
    copymethodfld_ = new uiGenInput( this,
				 tr("Copy Method: Use"),
				 StringListInpSpec(options) );
    copymethodfld_->attach( alignedBelow, strengthfld_ );

    displayfld_ = new uiCheckBox( this, tr("Display Result") );
    displayfld_->attach( alignedBelow, copymethodfld_ );
    displayfld_->setChecked( true );

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
    copymethodfld_->display( !isthick );
    displayfld_->display( !isthick );
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

    const bool top = selfld_->getBoolValue();
    mGetHor( hor1, top ? inpfld_ : inpfld2_ );
    mGetHor( hor2, top ? inpfld2_ : inpfld_ );

    PtrMan<Tut::ThicknessCalculator> calc = new Tut::ThicknessCalculator;
    calc->setHorizons( *hor1, hor2 );
    calc->setAttribName( attribnamefld_->text() );

    if ( !taskrunner.execute(*calc) )
	return false;

    PtrMan<Executor> saver = calc->dataSaver();
    if ( !saver || !taskrunner.execute(*saver) )
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

    const EM::ObjectID emid = EM::EMM().createObject( EM::Horizon3D::typeStr(),
						      outfld_->name() );
    mDynamicCastGet( EM::Horizon3D*, horizonoutput, EM::EMM().getObject(emid) )
    horizonoutput_ = horizonoutput;
    if ( !horizonoutput_ )
	return false;

    horizonoutput_->setName( outfld_->ioobj()->name() );
    horizonoutput_->setMultiID( outfld_->ioobj()->key() );

    PtrMan<Tut::HorSmoother> calc =
				new Tut::HorSmoother( *horizonoutput_ );

    uiTaskRunner taskrunner( this );

    mGetHor( hor, inpfld_ );
    calc->setHorizons( *hor );
    calc->setWeak( strengthfld_->getBoolValue() );

    if( copymethodfld_->getIntValue() == 0 )
    {
	ManagedObjectSet<BinIDValueSet> sections;
	auto* section = new BinIDValueSet( 1, true );
	sections.add( section );

	const auto& geom = hor->geometry();
	geom.fillBinIDValueSet( *sections.first(), nullptr );

	if ( sections.isEmpty() ||
	    !sections.first() ||
	     sections.first()->isEmpty() )
	    return false;

	PtrMan<Executor> importer =
	    horizonoutput_->importer( sections, hor->range() );
	if ( !taskrunner.execute( *importer ) )
	    return false;
    }
    else if ( copymethodfld_->getIntValue() == 1 )
    {
	PtrMan<Array2D<float>> arr = hor->createArray2D();
	if ( !arr )
	    return false;

	if ( !horizonoutput_->setArray2D(arr.ptr(), hor->range().start_,
					 hor->range().step_, false) )
	    return false;
    }
    else if ( copymethodfld_->getIntValue() == 2 )
    {
	const auto* geomel = hor->geometry().geometryElement();
	auto* outgeomel = horizonoutput_->geometry().geometryElement();
	if ( !geomel || !outgeomel )
	    return false;

	*outgeomel = *geomel;
    }
    else
	return false;

    if ( !taskrunner.execute(*calc) )
	return false;

    PtrMan<Executor> saver = calc->dataSaver( outfld_->key() );
    if ( !saver || !taskrunner.execute(*saver) )
    {
	uiMSG().error(tr("Smoothing operation failed"));
	return false;
    }

    if ( displayfld_->isChecked() )
    {
	mDynamicCastGet( uiODMain*, odmain, parent() );
	if ( odmain )
	{
	    auto* emserv = odmain->applMgr().EMServer();
	    emserv->displayEMObject( outfld_->key() );
	}
    }

    const uiString msg = tr("Process finished successfully.\n"
			    "Do you want to continue?");

    return !uiMSG().askGoOn( msg );
}
