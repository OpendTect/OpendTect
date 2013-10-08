#include "curvgrad.h"
#include "uicurvgrad.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "attribparamgroup.h"

#include "survinfo.h"


using namespace Attrib;


mInitAttribUI( uiCurvGrad, CurvGrad, "Curvature Gradient", sKeyDipGrp() )


uiCurvGrad::uiCurvGrad( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d)
{
    inputfld_ = createInpFld( is2d, "Curvature Data" );
    
    stepoutfld_ = new uiStepOutSel( this, is2d );
    const StepInterval<int> intv( 1, 10, 1 );
    stepoutfld_->setInterval( intv, intv );
    stepoutfld_->attach( alignedBelow, inputfld_ );

    steerfld_ = new uiSteeringSel( this, 0, is2d, false );
    steerfld_->steertypeSelected_.notify( mCB(this,uiCurvGrad,steerTypeSel) );
    steerfld_->attach( alignedBelow, stepoutfld_ );

    static const char* attributelist_[]={ "Gradient", "Azimuth", 0 };
    attributefld_ = 
	new uiGenInput( this, "Output", StringListInpSpec(attributelist_) );
    attributefld_->valuechanged.notify( mCB(this,uiCurvGrad,choiceSel) );
    attributefld_->attach( alignedBelow, steerfld_ );

    choiceSel(0);
    setHAlignObj( stepoutfld_ );
}


void uiCurvGrad::choiceSel( CallBacker* )
{}


bool uiCurvGrad::setParameters( const Desc& desc )
{
    if( strcmp(desc.attribName(),CurvGrad::attribName()) )
	return false;

    mIfGetEnum( sKey::Output(), attribute, attributefld_->setValue(attribute) );
    mIfGetBinID(CurvGrad::stepoutStr(),stepout,stepoutfld_->setBinID(stepout) );

    choiceSel(0);
    return true; 
}


bool uiCurvGrad::setInput( const Desc& desc )
{
    putInp( inputfld_, desc, 0 );
    putInp( steerfld_, desc, 1 );

    return true;
}


bool  uiCurvGrad::getParameters( Desc& desc )
{
    if( strcmp(desc.attribName(),CurvGrad::attribName()) )
	return false;

    BinID stepout( stepoutfld_->getBinID() );
    if( stepout == BinID(0,0) )
        stepout.inl() = stepout.crl() = mUdf(int);

    mSetBinID( CurvGrad::stepoutStr(), stepout );
    mSetEnum( sKey::Output(), attributefld_->getIntValue() );
    mSetBool( sKey::Steering(), steerfld_->willSteer() );

    return true;
}


bool uiCurvGrad::getInput( Desc& desc )
{
    fillInp( inputfld_, desc, 0 );
    fillInp( steerfld_, desc, 1 );

    return true;
}


void uiCurvGrad::steerTypeSel( CallBacker* )
{
    if( is2D() && steerfld_->willSteer() && !inputfld_->isEmpty() )
    {
	const char* steertxt = steerfld_->text();
	if( steertxt )
	{
	    LineKey inp( inputfld_->getInput() );
	    LineKey steer( steertxt );
	    if( strcmp(inp.lineName(), steer.lineName()) )
		steerfld_->clearInpField();
	}
    }
}

