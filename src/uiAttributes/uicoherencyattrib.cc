/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:		$Id: uicoherencyattrib.cc,v 1.14 2007-10-12 09:12:19 cvssulochana Exp $
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
#include "settings.h"

using namespace Attrib;

mInitAttribUI(uiCoherencyAttrib,Coherency,"Coherency",sKeyBasicGrp)


uiCoherencyAttrib::uiCoherencyAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.0")
    	, is1fld(0)
    	, stepoutfld(0)
{
    inpfld = getImagInpFld();

    if ( Settings::common().isTrue("Attribs.Enable Coh type 2") )
    {
	is1fld = new uiGenInput( this, "Type", BoolInpSpec(true,"1","2") );
	is1fld->attach( alignedBelow, inpfld );
	is1fld->valuechanged.notify( mCB(this,uiCoherencyAttrib,is1Sel) );
    }

    tgfld = new uiGenInput( this, gateLabel(), FloatInpIntervalSpec() );
    if ( is1fld )
	tgfld->attach( alignedBelow, is1fld );
    else
	tgfld->attach( alignedBelow, inpfld );

    maxdipfld = new uiGenInput( this, "Maximum dip", FloatInpSpec() );
    maxdipfld->attach( alignedBelow, tgfld );

    deltadipfld = new uiGenInput( this, "Delta dip", FloatInpSpec() );
    deltadipfld->attach( alignedBelow, maxdipfld );

    if ( is1fld )
    {
	stepoutfld = new uiStepOutSel( this, is2d );
	stepoutfld->attach( alignedBelow, deltadipfld );
    }

    setHAlignObj( tgfld );
}


void uiCoherencyAttrib::is1Sel( CallBacker* )
{
    if ( !is1fld || !stepoutfld ) return;
    stepoutfld->display( !is1fld->getBoolValue() );
}


bool uiCoherencyAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Coherency::attribName()) )
	return false;

    mIfGetFloatInterval( Coherency::sKeyGate(), gate, tgfld->setValue(gate) );
    mIfGetFloat( Coherency::sKeyMaxDip(),maxdip,maxdipfld->setValue(maxdip) );
    mIfGetFloat( Coherency::sKeyDDip(),ddip,deltadipfld->setValue(ddip) );

    if ( is1fld )
    {
	mIfGetInt( Coherency::sKeyType(), type,
		   is1fld->setValue(type==1? true:false));
	mIfGetBinID( Coherency::sKeyStepout(), stepout,
		     stepoutfld->setBinID(stepout) );
    }

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
    
    mSetFloatInterval( Coherency::sKeyGate(), tgfld->getFInterval() );
    mSetFloat( Coherency::sKeyMaxDip(), maxdipfld->getfValue() );
    mSetFloat( Coherency::sKeyDDip(), deltadipfld->getfValue() );
    if ( !is1fld )
    {
	mSetInt( Coherency::sKeyType(), 1 );
    }
    else
    {
	mSetInt( Coherency::sKeyType(), is1fld->getBoolValue() ? 1 : 2 );
	mSetBinID( Coherency::sKeyStepout(), stepoutfld->getBinID() );
    }
    return true;
}


bool uiCoherencyAttrib::getInput( Desc& desc )
{
    inpfld->processInput();
    fillInp( inpfld, desc, 0 );
    return true;
}
