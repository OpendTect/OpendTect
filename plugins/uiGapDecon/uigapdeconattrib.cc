/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          July  2006
 RCS:           $Id: uigapdeconattrib.cc,v 1.6 2006-09-11 13:14:10 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigapdeconattrib.h"
#include "gapdeconattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "survinfo.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uispinbox.h"

using namespace Attrib;

mInitUI( uiGapDeconAttrib, "GapDecon" )

uiGapDeconAttrib::uiGapDeconAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld_ = getInpFld();

    BufferString gatestr = "Correlation window ";
    gatestr += SI().getZUnit();
    gatefld_ = new uiGenInput( this, gatestr, FloatInpIntervalSpec() );
    gatefld_->attach( alignedBelow, inpfld_ );

    CallBack cbexam = mCB(this,uiGapDeconAttrib,examPush);
    exambut_ = new uiPushButton( this, "&Examine", cbexam, true);
    exambut_->attach( rightOf, gatefld_ );

    BufferString lagstr = "Lag size ";
    lagstr += SI().getZUnit();
    lagfld_ = new uiGenInput( this, lagstr, IntInpSpec() );
    lagfld_->attach( alignedBelow, gatefld_ );
    
    BufferString gapstr = "Gap size ";
    gapstr += SI().getZUnit();
    gapfld_ = new uiGenInput( this, gapstr, IntInpSpec() );
    gapfld_->attach( alignedBelow, lagfld_ );
    
    noiselvlfld_ = new uiGenInput( this, "random noise added", IntInpSpec() );
    noiselvlfld_->attach( alignedBelow, gapfld_ );
    uiLabel* percentlbl = new uiLabel( this, "%" );
    percentlbl->attach( rightOf, noiselvlfld_ );
    
    nrtrcsfld_ = new uiLabeledSpinBox( this, "nr traces mixed" );
    nrtrcsfld_->box()->setMinValue( 1 );
    nrtrcsfld_->box()->setStep( 2, true );
    nrtrcsfld_->attach( alignedBelow, noiselvlfld_ );
    uiLabel* stepoutlbl = new uiLabel( this, "( Smoothing parameter )" );
    stepoutlbl->attach( rightOf, nrtrcsfld_ );
    
    isinpzerophasefld_ = new uiGenInput( this, "Input is", 
				 BoolInpSpec("Zero phase", "Minimum phase") );
    isinpzerophasefld_->attach( alignedBelow, nrtrcsfld_ );
    
    isoutzerophasefld_ = new uiGenInput( this, "Output is", 
				 BoolInpSpec("Zero phase", "Minimum phase") );
    isoutzerophasefld_->attach( alignedBelow, isinpzerophasefld_ );
    
    setHAlignObj( gatefld_ );
}


const char* uiGapDeconAttrib::getAttribName() const
{ return GapDecon::attribName(); }


bool uiGapDeconAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),GapDecon::attribName()) )
	return false;

    mIfGetFloatInterval( GapDecon::gateStr(), gate, gatefld_->setValue(gate) )
    mIfGetInt( GapDecon::lagsizeStr(), lagsz, lagfld_->setValue(lagsz) )
    mIfGetInt( GapDecon::gapsizeStr(), gapsz, gapfld_->setValue(gapsz) )
    mIfGetInt( GapDecon::nrtrcsStr(), nrtmixed, nrtrcsfld_->box()->
	    						setValue(nrtmixed) )
    mIfGetInt( GapDecon::noiselevelStr(), nlvl, noiselvlfld_->setValue(nlvl) )
    mIfGetBool( GapDecon::isinp0phaseStr(), isinp0ph, 
	    	isinpzerophasefld_->setValue(isinp0ph) )
    mIfGetBool( GapDecon::isout0phaseStr(), isout0ph, 
	    	isoutzerophasefld_->setValue(isout0ph) )
    return true;
}


bool uiGapDeconAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiGapDeconAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),GapDecon::attribName()) )
	return false;

    mSetFloatInterval( GapDecon::gateStr(), gatefld_->getFInterval() );
    mSetInt( GapDecon::lagsizeStr(), lagfld_->getIntValue() );
    mSetInt( GapDecon::gapsizeStr(), gapfld_->getIntValue() );
    mSetInt( GapDecon::nrtrcsStr(), nrtrcsfld_->box()->getValue() );
    mSetInt( GapDecon::noiselevelStr(), noiselvlfld_->getIntValue() );
    mSetBool( GapDecon::isinp0phaseStr(), isinpzerophasefld_->getBoolValue() );
    mSetBool( GapDecon::isout0phaseStr(), isoutzerophasefld_->getBoolValue() );

    return true;
}


bool uiGapDeconAttrib::getInput( Attrib::Desc& desc )
{
    inpfld_->processInput();
    fillInp( inpfld_, desc, 0 );
    return true;
}


void uiGapDeconAttrib::examPush( CallBacker* cb )
{
    //TODO
}


//TODO see which param we would want to eval
void uiGapDeconAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( "noise Level (%)", GapDecon::noiselevelStr() );
}


