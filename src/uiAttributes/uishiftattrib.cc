/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2001
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id$";


#include "uishiftattrib.h"
#include "shiftattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"

using namespace Attrib;


mInitAttribUI(uiShiftAttrib,Shift,"Reference shift",sKeyPositionGrp())

uiShiftAttrib::uiShiftAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.13")

{
    inpfld_ = createInpFld( is2d );

    uiStepOutSel::Setup setup( is2d );
    setup.seltxt( "Shift" ).allowneg( true );
    stepoutfld_ = new uiStepOutSel( this, setup );
    stepoutfld_->setFieldNames( "Inl shift", "Crl shift" );
    stepoutfld_->attach( alignedBelow, inpfld_ );

    timefld_ = new uiGenInput( this, zDepLabel(0,0),
			       FloatInpSpec().setName("Z shift") );
    timefld_->setElemSzPol(uiObject::Small);
    timefld_->attach( rightTo, stepoutfld_ );

    dosteerfld_ = new uiGenInput( this, "Use steering", BoolInpSpec(true) );
    dosteerfld_->attach( alignedBelow, stepoutfld_ );
    dosteerfld_->valuechanged.notify( mCB(this,uiShiftAttrib,steerSel) );
    dosteerfld_->setElemSzPol( uiObject::SmallVar );

    steerfld_ = new uiSteeringSel( this, 0, is2d );
    steerfld_->steertypeSelected_.notify( mCB(this,uiShiftAttrib,steerTypeSel));
    steerfld_->attach( alignedBelow, dosteerfld_ );

    steerSel(0);
    setHAlignObj( inpfld_ );
}


void uiShiftAttrib::steerSel( CallBacker* )
{
    steerfld_->display( dosteerfld_->getBoolValue() );
}


bool uiShiftAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Shift::attribName()) )
	return false;

    mIfGetBinID( Shift::posStr(), pos, stepoutfld_->setBinID(pos) );
    mIfGetFloat( Shift::timeStr(), time, timefld_->setValue(time) );
    mIfGetBool( Shift::steeringStr(), dosteer,	dosteerfld_->setValue(dosteer));

    steerSel(0);
    return true;
}


bool uiShiftAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    putInp( steerfld_, desc, 1 );
    return true;
}


bool uiShiftAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Shift::attribName()) )
	return false;

    const bool dosteering = dosteerfld_->getBoolValue();
    mSetFloat( Shift::timeStr(), timefld_->getfValue() );
    mSetBool( Shift::steeringStr(), dosteering ? steerfld_->willSteer() :false);
    mSetBinID( Shift::posStr(), stepoutfld_->getBinID() );

    return true;
}


bool uiShiftAttrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    fillInp( steerfld_, desc, 1 );
    return true;
}


void uiShiftAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    BufferString str = zIsTime() ? "Time" : "Depth"; str += " shift";
    params += EvalParam( str, Shift::timeStr() );
    params += EvalParam( stepoutstr(), Shift::posStr() );
}


void uiShiftAttrib::steerTypeSel( CallBacker* )
{
    if ( is2D() && steerfld_->willSteer() && !inpfld_->isEmpty() )              
    {                                                                           
	const char* steertxt = steerfld_->text();                               
	if ( steertxt )                                                         
	{                                                                       
	    LineKey inp( inpfld_->getInput() );                                 
	    LineKey steer( steertxt );                                          
	    if ( strcmp( inp.lineName(), steer.lineName() ) )                   
		steerfld_->clearInpField();
	}
    }
}
