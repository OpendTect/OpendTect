/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:		$Id: uicoherencyattrib.cc,v 1.11 2007-01-26 12:00:29 cvsbert Exp $
________________________________________________________________________

-*/

#include "uicoherencyattrib.h"
#include "coherencyattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"

using namespace Attrib;

mInitAttribUI(uiCoherencyAttrib,Coherency,"Coherency",sKeyBasicGrp)


uiCoherencyAttrib::uiCoherencyAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d)
{
    inpfld = getImagInpFld();

    is1fld = new uiGenInput( this, "Type", BoolInpSpec("1","2") );
    is1fld->attach( alignedBelow, inpfld );
    is1fld->valuechanged.notify( mCB(this,uiCoherencyAttrib,is1Sel) );

    tgfld = new uiGenInput( this, gateLabel(), FloatInpIntervalSpec() );
    tgfld->attach( alignedBelow, is1fld );

    maxdipfld = new uiGenInput( this, "Maximum dip", FloatInpSpec() );
    maxdipfld->attach( alignedBelow, tgfld );

    deltadipfld = new uiGenInput( this, "Delta dip", FloatInpSpec() );
    deltadipfld->attach( alignedBelow, maxdipfld );

    stepoutfld = new uiStepOutSel( this, is2d );
    stepoutfld->attach( alignedBelow, deltadipfld );

    setHAlignObj( tgfld );
}


void uiCoherencyAttrib::is1Sel( CallBacker* )
{
    stepoutfld->display( !is1fld->getBoolValue() );
}


bool uiCoherencyAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Coherency::attribName()) )
	return false;

    mIfGetInt(Coherency::sKeyType(),type,is1fld->setValue(type==1? true:false));
    mIfGetFloatInterval( Coherency::sKeyGate(), gate, tgfld->setValue(gate) );
    mIfGetFloat( Coherency::sKeyMaxDip(),maxdip,maxdipfld->setValue(maxdip) );
    mIfGetFloat( Coherency::sKeyDDip(),ddip,deltadipfld->setValue(ddip) );
    mIfGetBinID(Coherency::sKeyStepout(),stepout, stepoutfld->setBinID(stepout))

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
    if ( strcmp(desc.attribName(),Coherency::attribName()) )
	return false;
    
    mSetInt( Coherency::sKeyType(), is1fld->getBoolValue() ? 1 : 2 );
    mSetFloatInterval( Coherency::sKeyGate(), tgfld->getFInterval() );
    mSetFloat( Coherency::sKeyMaxDip(), maxdipfld->getfValue() );
    mSetFloat( Coherency::sKeyDDip(), deltadipfld->getfValue() );
    mSetBinID( Coherency::sKeyStepout(), stepoutfld->getBinID() );
    return true;
}


bool uiCoherencyAttrib::getInput( Desc& desc )
{
    inpfld->processInput();
    fillInp( inpfld, desc, 0 );
    return true;
}
