/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

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

static const char* sKeyDeltaDip = "Delta dip";
static const char* sKeyMaxDip = "Maximum dip";


mInitAttribUI(uiCoherencyAttrib,Coherency,"Coherency",sKeyBasicGrp())


uiCoherencyAttrib::uiCoherencyAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.0")
    	, is1fld(0)
    	, stepoutfld(0)
{
    inpfld = createImagInpFld(is2d);

    if ( Settings::common().isTrue("Attribs.Enable Coh type 2") )
    {
	is1fld = new uiGenInput( this, "Type", BoolInpSpec(true,"1","2") );
	is1fld->attach( alignedBelow, inpfld );
	is1fld->valuechanged.notify( mCB(this,uiCoherencyAttrib,is1Sel) );
    }

    tgfld = new uiGenInput( this, gateLabel(), 
			    FloatInpIntervalSpec().setName("Z start",0)
						  .setName("Z stop",1) );
    if ( is1fld )
	tgfld->attach( alignedBelow, is1fld );
    else
	tgfld->attach( alignedBelow, inpfld );

    BufferString mdlbl = sKeyMaxDip; mdlbl += zIsTime() ? " (us/m)" : " (mm/m)";
    maxdipfld = new uiGenInput( this, mdlbl, FloatInpSpec() );
    maxdipfld->attach( alignedBelow, tgfld );

    BufferString ddlbl = sKeyDeltaDip;
    ddlbl += zIsTime() ? " (us/m)" : " (mm/m)";
    deltadipfld = new uiGenInput( this, ddlbl, FloatInpSpec() );
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


void uiCoherencyAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr(), Coherency::sKeyGate() );
    params += EvalParam( sKeyMaxDip, Coherency::sKeyMaxDip() );
    params += EvalParam( sKeyDeltaDip, Coherency::sKeyDDip() );
}
