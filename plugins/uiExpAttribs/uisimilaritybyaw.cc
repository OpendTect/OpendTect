/*+
_______________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Haibin Di
 Date:          August 2013
 ________________________________________________________________________
-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisimilaritybyaw.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "similaritybyaw.h"
#include "survinfo.h"

using namespace Attrib;

mInitAttribUI( uiSimilaritybyAW, SimilaritybyAW, 
	"Similarity by Adaptive Time Gate", sKeyBasicGrp() )

uiSimilaritybyAW::uiSimilaritybyAW( uiParent* p, bool is2d )
    :  uiAttrDescEd(p, is2d)
{
    inputfld_ = createInpFld( is2d, "Input Data" );

    reftimegatefld_ = 
	new uiGenInput( this, "Ref time gate (ms)", FloatInpIntervalSpec() );
    reftimegatefld_->attach( alignedBelow, inputfld_ );

    searchrangefld_ = 
	new uiGenInput( this, "Search range (ms)", FloatInpIntervalSpec() );
    searchrangefld_->attach( alignedBelow, reftimegatefld_ );

    stepoutfld_ = new uiStepOutSel( this, is2d );
    const StepInterval<int> intv( 1, 10, 1 );
    stepoutfld_->setInterval( intv, intv );
    stepoutfld_->attach( alignedBelow, searchrangefld_ );

    steerfld_ = new uiSteeringSel( this, 0, is2d, false );
    steerfld_->steertypeSelected_.notify(
	    mCB(this, uiSimilaritybyAW, steerTypeSel) );
    steerfld_->attach( alignedBelow, stepoutfld_ );

    static const char* attributelist_[] = { "Optimal similarity", 
					    "Optimal time gate", 0 };
    attributefld_ = 
	new uiGenInput( this, "Output", StringListInpSpec(attributelist_) );
    attributefld_->valuechanged.notify( 
	    mCB(this, uiSimilaritybyAW, choiceSel) );
    attributefld_->attach(alignedBelow, steerfld_);

    choiceSel(0);
    setHAlignObj( inputfld_ );
}


void uiSimilaritybyAW::choiceSel( CallBacker* )
{
}


bool uiSimilaritybyAW::setParameters( const Desc& desc )
{
    if( strcmp(desc.attribName(),SimilaritybyAW::attribName()) )
	return false;

    mIfGetEnum( sKey::Output(), attribute,
	    attributefld_->setValue(attribute) );

    mIfGetFloatInterval( SimilaritybyAW::refTimeGateStr(), refTimeGate,
	    reftimegatefld_->setValue(refTimeGate) );

    mIfGetFloatInterval ( SimilaritybyAW::searchRangeStr(), searchRange,
	    searchrangefld_->setValue(searchRange) );

    mIfGetBinID( SimilaritybyAW::stepoutStr(), stepout,
	    stepoutfld_->setBinID(stepout) );

    choiceSel(0);
    return true; 
}


bool uiSimilaritybyAW::setInput( const Desc& desc )
{
    putInp( inputfld_, desc, 0 );
    putInp( steerfld_, desc, 1 );

    return true;
}


bool  uiSimilaritybyAW::getParameters( Desc& desc )
{
    if( strcmp(desc.attribName(),SimilaritybyAW::attribName()) )
	return false;

    mSetFloatInterval( SimilaritybyAW::refTimeGateStr(), 
	    reftimegatefld_->getFInterval() );

    mSetFloatInterval( SimilaritybyAW::searchRangeStr(), 
	    searchrangefld_->getFInterval() );

    BinID stepout( stepoutfld_->getBinID() );
    if( stepout == BinID(0,0) )
        stepout.inl() = stepout.crl() = mUdf(int);
    mSetBinID( SimilaritybyAW::stepoutStr(), stepout );

    mSetEnum( sKey::Output(), attributefld_->getIntValue() );
    mSetBool( sKey::Steering(), steerfld_->willSteer() );

    return true;
}


bool uiSimilaritybyAW::getInput( Desc& desc )
{
    fillInp( inputfld_, desc, 0 );
    fillInp( steerfld_, desc, 1 );

    return true;
}


void uiSimilaritybyAW::steerTypeSel( CallBacker* )
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

