/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiinstantattrib.h"
#include "instantattrib.h"

#include "attribdesc.h"
#include "attribparambase.h"
#include "od_helpids.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uispinbox.h"

using namespace Attrib;


mInitAttribUI(uiInstantaneousAttrib,Instantaneous,"Instantaneous",
              sKeyBasicGrp())


uiInstantaneousAttrib::uiInstantaneousAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mInstantaneousAttribHelpID) )

{
    inpfld = createImagInpFld( is2d );

    outpfld = new uiGenInput( this, uiStrings::sOutput(),
	StringListInpSpec(Instantaneous::OutTypeDef().strings()) );
    outpfld->setElemSzPol( uiObject::MedVar );
    outpfld->attach( alignedBelow, inpfld );
    outpfld->valueChanged.notify( mCB(this,uiInstantaneousAttrib,outputSelCB) );

    phaserotfld = new uiLabeledSpinBox(this,tr("Specify angle (deg)"));
    phaserotfld->box()->setInterval( -180, 180 );
    phaserotfld->attach( alignedBelow, outpfld );

    setHAlignObj( inpfld );
}


uiInstantaneousAttrib::~uiInstantaneousAttrib()
{}


bool uiInstantaneousAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != Instantaneous::attribName() )
	return false;

    mIfGetFloat( Instantaneous::rotateAngle(), rotangle_,
	    	 phaserotfld->box()->setValue( rotangle_ ) );

    return true;
}


bool uiInstantaneousAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiInstantaneousAttrib::setOutput( const Desc& desc )
{
    outpfld->setValue( desc.selectedOutput() );
    outputSelCB(0);
    return true;
}


bool uiInstantaneousAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != Instantaneous::attribName() )
	return false;

    mSetFloat( Instantaneous::rotateAngle(), phaserotfld->box()->getFValue() );

    return true;
}


bool uiInstantaneousAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}


bool uiInstantaneousAttrib::getOutput( Desc& desc )
{
    fillOutput( desc, outpfld->getIntValue() );
    return true;
}


void uiInstantaneousAttrib::outputSelCB( CallBacker* )
{
    const Instantaneous::OutType ot =
	Instantaneous::OutTypeDef().getEnumForIndex(outpfld->getIntValue());
    const bool isrot = ot == Instantaneous::RotatePhase;
    phaserotfld->display( isrot );
}


void uiInstantaneousAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    const Instantaneous::OutType ot =
	Instantaneous::OutTypeDef().getEnumForIndex(outpfld->getIntValue());
    const bool isrot = ot == Instantaneous::RotatePhase;
    if ( isrot )
	params += EvalParam( "Rotation angle",
			     Instantaneous::rotateAngle() );
}
