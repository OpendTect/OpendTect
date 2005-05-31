/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uishiftattrib.cc,v 1.1 2005-05-31 12:33:55 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uishiftattrib.h"
#include "hashattrib.h"
#include "attribdesc.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uistepoutsel.h"

using namespace Attrib;


uiShiftAttrib::uiShiftAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld = getInpFld();

    stepoutfld = new uiStepOutSel( this, "Trace shift`Inl/Crl shift" );
    stepoutfld->attach( alignedBelow, inpfld );

    const char* zstr = zIsTime() ? "Time" : "Depth";
    typefld = new uiGenInput( this, "Use", BoolInpSpec(zstr,"Steering") );
    typefld->attach( alignedBelow, stepoutfld );
    typefld->valuechanged.notify( mCB(this,uiShiftAttrib,shiftSel) );
    typefld->setElemSzPol( uiObject::smallvar );

    timefld = new uiGenInput( this, shiftLabel(), FloatInpSpec() );
    timefld->attach( alignedBelow, typefld );

    steerfld = new uiSteeringSel( this, 0 );
    steerfld->attach( alignedBelow, typefld );

    setHAlignObj( inpfld );
}


void uiShiftAttrib::set2D( bool yn )
{
    inpfld->set2D( yn );
    stepoutfld->set2D( yn );
    steerfld->set2D( yn );
}


void uiShiftAttrib::shiftSel( CallBacker* )
{
    const bool dosteer = !typefld->getBoolValue();
    timefld->display( !dosteer );
    steerfld->display( dosteer );
}


bool uiShiftAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Hash::attribName()) )
	return false;

    mIfGetBinID( Hash::stepoutStr(), steout, stepoutfld->setBinID(stepout) );
    mIfGetFloat( Hash::timeStr(), time, timefld->setValue(time) );
    mIfGetBool( Hash::steeringStr(), steering, typefld->setValue(!steering) );

    shiftSel(0);
    return true;
}


bool uiShiftAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    putInp( steerfld, desc );
    return true;
}


bool uiShiftAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Hash::attribName()) )
	return false;

    const bool dotime = typefld->getBoolValue();
    mSetFloat( Hash::timeStr(), dotime ? timefld->getfValue() : 0 );
    mSetBool( Hash::steerStr(), dotime ? false : steerfld->willSteer() );
    mSetBinID( Hash::stepoutStr(), stepoutfld->binID() );

    return true;
}


bool uiShiftAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    fillInp( steerfld, desc, 1 );
    return true;
}
