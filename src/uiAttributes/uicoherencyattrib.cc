/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uicoherencyattrib.cc,v 1.2 2005-09-30 15:45:13 cvshelene Exp $";

#include "uicoherencyattrib.h"
#include "coherencyattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uisteeringsel.h"
#include "uigeninput.h"
#include "uistepoutsel.h"

using namespace Attrib;

uiCoherencyAttrib::uiCoherencyAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld = getImagInpFld();

    is1fld = new uiGenInput( this, "Type", BoolInpSpec("1","2",false) );
    is1fld->attach( alignedBelow, inpfld );
    is1fld->valuechanged.notify( mCB(this,uiCoherencyAttrib,is1Sel) );

    tgfld = new uiGenInput( this, gateLabel(), FloatInpIntervalSpec() );
    tgfld->setValues( -28, 28 );
    tgfld->attach( alignedBelow, is1fld );

    maxdipfld = new uiGenInput( this, "Maximum dip", FloatInpSpec() );
    maxdipfld->attach( alignedBelow, tgfld );

    deltadipfld = new uiGenInput( this, "Delta dip", FloatInpSpec() );
    deltadipfld->attach( alignedBelow, maxdipfld );

    stepoutfld = new uiStepOutSel( this );
    stepoutfld->setVal( true, 1 ); stepoutfld->setVal( false, 1 );
    stepoutfld->attach( alignedBelow, deltadipfld );

    setHAlignObj( tgfld );
}


void uiCoherencyAttrib::set2D( bool yn )
{
    inpfld->set2D( yn );
    stepoutfld->set2D( yn );
}


void uiCoherencyAttrib::is1Sel( CallBacker* )
{
    stepoutfld->display( !is1fld->getBoolValue() );
}


bool uiCoherencyAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Coherency::attribName()) )
	return false;

    mIfGetInt(Coherency::typeStr(),type,is1fld->setValue(type==1? true:false));
    mIfGetFloatInterval( Coherency::gateStr(), gate, tgfld->setValue(gate) );
    mIfGetFloat( Coherency::maxdipStr(),maxdip,maxdipfld->setValue(maxdip) );
    mIfGetFloat( Coherency::ddipStr(),ddip,deltadipfld->setValue(ddip) );
    mIfGetBinID(Coherency::stepoutStr(), stepout, stepoutfld->setBinID(stepout))

    is1Sel(0);
    return true;
}


bool uiCoherencyAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiCoherencyAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(), Coherency::attribName()) )
	return false;
    
    mSetBool( Coherency::typeStr(), is1fld->getBoolValue() ? 1 : 2 );
    mSetFloatInterval( Coherency::gateStr(), tgfld->getFInterval() );
    mSetFloat( Coherency::maxdipStr(), maxdipfld->getfValue() );
    mSetFloat( Coherency::ddipStr(), deltadipfld->getfValue() );
    mSetBinID( Coherency::stepoutStr(), stepoutfld->binID() );
    return true;
}


bool uiCoherencyAttrib::getInput( Desc& desc )
{
    inpfld->processInput();
    fillInp( inpfld, desc, 0 );
    return true;
}
