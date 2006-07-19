/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          July  2006
 RCS:           $Id: uigapdeconattrib.cc,v 1.1 2006-07-19 13:35:51 cvshelene Exp $
________________________________________________________________________

-*/

#include "uigapdeconattrib.h"
#include "gapdeconattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uispinbox.h"

using namespace Attrib;

uiGapDeconAttrib::uiGapDeconAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld = getInpFld();

    gatefld_ = new uiGenInput(this,"Correlation window",FloatInpIntervalSpec());
    gatefld_->attach( alignedBelow, inpfld );

    lagfld_ = new uiGenInput( this, "Lag size", IntInpSpec() );
    lagfld_->attach( alignedBelow, gatefld );
    
    gapfld_ = new uiGenInput( this, "Gap size", IntInpSpec() );
    gapfld_->attach( alignedBelow, lagfld_ );
    
    noiselvlfld_ = new uiGenInput( this, "random noise added", IntInpSpec() );
    gapfld_->attach( alignedBelow, lagfld_ );
    uiLabel* percentlbl = new uiLabel( this, "%" );
    percentlbl->attach( rightOf, noiselvlfld_ );
    
    BufferString stepoutlbl = "Smoothing parameter: nr traces mixed";
    stepoutfld_ = new uiLabeledSpinBox( this, stepoutlbl );
    stepoutfld_->box()->setMinValue( 1 );
    stepoutfld_->box()->setStep( 2, true );
    stepoutfld_->attach( alignedBelow, gapfld_ );
    
    isinpzerophasefld_ = new uiGenInput( this, "Input is", 
				 BoolInpSpec("Zero phase", "Minimum phase") );
    isinpzerophasefld_->attach( alignedBelow, stepoutfld_ );
    
    isoutzerophasefld_ = new uiGenInput( this, "Output is", 
				 BoolInpSpec("Zero phase", "Minimum phase") );
    isoutzerophasefld_->attach( alignedBelow, isinpzerophasefld_ );
    
    setHAlignObj( gatefld_ );
}


bool uiGapDeconAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),GapDecon::attribName()) )
	return false;

    mIfGetFloatInterval( GapDecon::gateStr(), gate, gatefld_->setValue(gate) )
    mIfGetInt( GapDecon::lagsizeStr(), lagsz, lagfld_->setValue(lagsz) )
    mIfGetInt( GapDecon::gapsizeStr(), gapsz, gapfld_->setValue(gapsz) )
    mIfGetInt( GapDecon::nrtrcStr(), nrtmixed, stepoutfld_->setValue(nrtmixed) )
    mIfGetInt( GapDecon::noiselevelStr(), nlvl, noiselvlfld_->setValue(nlvl) )
    mIfGetBool( GapDecon::isinp0phaseStr(), isinp0ph, 
	    	isinpzerophasefld_->setValue(isinp0ph) )
    mIfGetBool( GapDecon::isout0phaseStr(), isout0ph, 
	    	isoutzerophasefld_->setValue(isout0ph) )
    return true;
}


bool uiGapDeconAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiGapDeconAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),GapDecon::attribName()) )
	return false;

    mSetFloatInterval( GapDecon::gateStr(), gatefld->getFInterval() );
    mSetInt( GapDecon::lagsizeStr(), lagfld->getIntValue() );
    mSetInt( GapDecon::gapsizeStr(), gapfld->getIntValue() );
    mSetInt( GapDecon::nrtrcStr(), stepoutfld->getIntValue() );
    mSetInt( GapDecon::noiselevelStr(), noiselvlfld->getIntValue() );
    mSetBool( GapDecon::isinp0phaseStr(), isinpzerophasefld->getBoolValue() );
    mSetBool( GapDecon::isout0phaseStr(), isoutzerophasefld->getBoolValue() );

    return true;
}


bool uiGapDeconAttrib::getInput( Attrib::Desc& desc )
{
    inpfld->processInput();
    fillInp( inpfld, desc, 0 );
    return true;
}


//see which param we would want to eval
/*
void uiGapDeconAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr, GapDecon::gateStr() );

    if ( !strcmp(extstrs[3],extfld->text()) )
	params += EvalParam( stepoutstr, GapDecon::stepoutStr() );
    else
	params += EvalParam( "Trace positions", GapDecon::pos0Str(),
			     GapDecon::pos1Str() );
}
*/
