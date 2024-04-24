/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipositionattrib.h"
#include "positionattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "od_helpids.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"

using namespace Attrib;

static const char* opstrs[] =
{
	"Min",
	"Max",
	"Median",
	0
};

mInitAttribUI(uiPositionAttrib,Position,"Position",sKeyPositionGrp())


uiPositionAttrib::uiPositionAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mPositionAttribHelpID) )

{
    inpfld_ = createInpFld( is2d,
				uiStrings::phrInput(uiStrings::sAttribute()) );

    stepoutfld_ = new uiStepOutSel( this, is2d );
    stepoutfld_->setFieldNames( "Inl Stepout", "Crl Stepout" );
    stepoutfld_->attach( alignedBelow, inpfld_ );

    gatefld_ = new uiGenInput( this, gateLabel(),
			      FloatInpIntervalSpec().setName("Z start",0)
						    .setName("Z stop",1) );
    gatefld_->attach( alignedBelow, stepoutfld_ );

    steerfld_ = new uiSteeringSel( this, 0, is2d );
    steerfld_->steertypeSelected_.notify(
				mCB(this,uiPositionAttrib,steerTypeSel) );
    steerfld_->attach( alignedBelow, gatefld_ );

    operfld_ = new uiGenInput( this, uiStrings::sOperator(),
                              StringListInpSpec(opstrs) );
    operfld_->attach( alignedBelow, steerfld_ );

    outfld_ = createInpFld( is2d, tr("Output attribute") );
    outfld_->attach( alignedBelow, operfld_ );

    setHAlignObj( inpfld_ );
}


uiPositionAttrib::~uiPositionAttrib()
{}


bool uiPositionAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != Position::attribName() )
	return false;

    mIfGetFloatInterval( Position::gateStr(), gate, gatefld_->setValue(gate) );
    mIfGetBinID( Position::stepoutStr(), stepout,
		 stepoutfld_->setBinID(stepout) );
    mIfGetEnum( Position::operStr(), oper, operfld_->setValue(oper) );

    return true;
}


bool uiPositionAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    putInp( outfld_, desc, 1 );
    putInp( steerfld_, desc, 2 );

    return true;
}


bool uiPositionAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != Position::attribName() )
	return false;

    mSetFloatInterval( Position::gateStr(), gatefld_->getFInterval() );
    mSetBinID( Position::stepoutStr(), stepoutfld_->getBinID() );
    mSetEnum( Position::operStr(), operfld_->getIntValue() );
    mSetBool( Position::steeringStr(), steerfld_->willSteer() );

    return true;
}


bool uiPositionAttrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    fillInp( outfld_, desc, 1 );
    fillInp( steerfld_, desc, 2 );

    return true;
}


void uiPositionAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr(), Position::gateStr() );
    params += EvalParam( stepoutstr(), Position::stepoutStr() );
}


void uiPositionAttrib::steerTypeSel( CallBacker* )
{
}
