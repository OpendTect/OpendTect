/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uishiftattrib.cc,v 1.10 2006-11-23 12:55:40 cvsbert Exp $
________________________________________________________________________

-*/

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


mInitAttribUI(uiShiftAttrib,Shift,"Reference shift",sKeyPositionGrp)

uiShiftAttrib::uiShiftAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld = getInpFld();

    stepoutfld = new uiStepOutSel( this, uiStepOutSel::Setup().seltxt("Shift"));
    stepoutfld->attach( alignedBelow, inpfld );

    const char* zstr = zIsTime() ? "Time" : "Depth";
    typefld = new uiGenInput( this, "Use", BoolInpSpec(zstr,"Steering") );
    typefld->attach( alignedBelow, stepoutfld );
    typefld->valuechanged.notify( mCB(this,uiShiftAttrib,shiftSel) );
    typefld->setElemSzPol( uiObject::SmallVar );

    timefld = new uiGenInput( this, shiftLabel(), FloatInpSpec() );
    timefld->attach( alignedBelow, typefld );

    steerfld = new uiSteeringSel( this, 0 );
    steerfld->attach( alignedBelow, typefld );

    shiftSel(0);
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
    if ( strcmp(desc.attribName(),Shift::attribName()) )
	return false;

    mIfGetBinID( Shift::posStr(), pos, stepoutfld->setBinID(pos) );
    mIfGetFloat( Shift::timeStr(), time, timefld->setValue(time) );
    mIfGetBool( Shift::steeringStr(), steering, typefld->setValue(!steering) );

    shiftSel(0);
    return true;
}


bool uiShiftAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    putInp( steerfld, desc, 1 );
    return true;
}


bool uiShiftAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Shift::attribName()) )
	return false;

    const bool dotime = typefld->getBoolValue();
    mSetFloat( Shift::timeStr(), dotime ? timefld->getfValue() : 0 );
    mSetBool( Shift::steeringStr(), dotime ? false : steerfld->willSteer() );
    mSetBinID( Shift::posStr(), stepoutfld->getBinID() );

    return true;
}


bool uiShiftAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    fillInp( steerfld, desc, 1 );
    return true;
}


void uiShiftAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    BufferString str = zIsTime() ? "Time" : "Depth"; str += " shift";
    if ( typefld->getBoolValue() )
	params += EvalParam( str, Shift::timeStr() );
    params += EvalParam( stepoutstr, Shift::posStr() );
}
