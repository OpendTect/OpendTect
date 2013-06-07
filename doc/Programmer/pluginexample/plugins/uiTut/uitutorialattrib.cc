/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          May 2007
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uitutorialattrib.cc,v 1.1 2012/03/26 14:32:59 cvsdgb Exp $";


#include "uitutorialattrib.h"
#include "tutorialattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "survinfo.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"

using namespace Attrib;


static const char* actionstr[] =
{
    "Scale",
    "Square",
    "Smooth",
    0
};


mInitAttribUI(uiTutorialAttrib,Tutorial,"Tutorial",sKeyBasicGrp())


uiTutorialAttrib::uiTutorialAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d)
{
    inpfld_ = createInpFld( is2d );

    actionfld_ = new uiGenInput( this, "Action", 
	    			StringListInpSpec(actionstr) );
    actionfld_->valuechanged.notify( mCB(this,uiTutorialAttrib,actionSel) );
    actionfld_->attach( alignedBelow, inpfld_ );

    smoothdirfld_ = new uiGenInput( this, "Smoothing direction",
	                        BoolInpSpec(true,"Horizontal","Vertical") );
    smoothdirfld_->valuechanged.notify( mCB(this,uiTutorialAttrib,actionSel) );
    smoothdirfld_->attach( alignedBelow, actionfld_ );

    smoothstrengthfld_ = new uiGenInput( this, "Filter strength",
                                BoolInpSpec(true,"Low","High") );
    smoothstrengthfld_->attach( alignedBelow, smoothdirfld_ );

    steerfld_ = new uiSteeringSel( this, 0, is2d, false );
    steerfld_->steertypeSelected_.notify(
	    			mCB(this,uiTutorialAttrib,steerTypeSel) );
    steerfld_->attach( alignedBelow, smoothdirfld_ );

    stepoutfld_ = new uiStepOutSel( this, is2d );
    stepoutfld_->setFieldNames( "Stepout Inl", "Stepout Crl" );
    const StepInterval<int> intv( 0, 10, 1 );
    stepoutfld_->setInterval( intv, intv );
    stepoutfld_->attach( alignedBelow, steerfld_ );

    factorfld_ = new uiGenInput( this, "Factor", FloatInpSpec() );
    factorfld_->attach( alignedBelow, actionfld_ );

    shiftfld_ = new uiGenInput( this, "Shift", FloatInpSpec() );
    shiftfld_->attach( alignedBelow, factorfld_ );

    actionSel(0);

    setHAlignObj( inpfld_ );
}


void uiTutorialAttrib::actionSel( CallBacker* )
{
    const int actval = actionfld_->getIntValue();
    const bool horsmooth = smoothdirfld_->getBoolValue();

    factorfld_->display( actval==0 );
    shiftfld_->display( actval==0 );
    smoothdirfld_->display( actval==2 );
    steerfld_->display( actval==2 && horsmooth );
    stepoutfld_->display( actval==2 && horsmooth );
    smoothstrengthfld_->display( actval==2 && !horsmooth );
}


bool uiTutorialAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Tutorial::attribName()) )
	return false;

    mIfGetEnum( Tutorial::actionStr(), action,
	        actionfld_->setValue(action) );
    mIfGetFloat( Tutorial::factorStr(), factor, factorfld_->setValue(factor) );
    mIfGetFloat( Tutorial::shiftStr(), shift, shiftfld_->setValue(shift) );
    mIfGetBool( Tutorial::horsmoothStr(), horsmooth, 
	    	smoothdirfld_->setValue(horsmooth) );
    mIfGetBool( Tutorial::weaksmoothStr(), weaksmooth,
                smoothstrengthfld_->setValue(weaksmooth) );
    mIfGetBinID( Tutorial::stepoutStr(), stepout,
                stepoutfld_->setBinID(stepout) );
    actionSel(0);

    return true;
}


bool uiTutorialAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    putInp( steerfld_, desc, 1 );
    return true;
}


bool uiTutorialAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Tutorial::attribName()) )
	return false;

    bool dosteer = false;
    mSetEnum( Tutorial::actionStr(), actionfld_->getIntValue() );
    if ( actionfld_->getIntValue() == 0 )
    {
    	mSetFloat( Tutorial::factorStr(), factorfld_->getfValue() );
    	mSetFloat( Tutorial::shiftStr(), shiftfld_->getfValue() );
    }
    else if (actionfld_->getIntValue() == 2 )
    {
    	mSetBool( Tutorial::horsmoothStr(), smoothdirfld_->getBoolValue() );
	if ( smoothdirfld_->getBoolValue() )
	{
	    BinID stepout( stepoutfld_->getBinID() );
	    if ( stepout == BinID(0,0) )
		stepout.inl = stepout.crl = mUdf(int);
    	    mSetBinID( Tutorial::stepoutStr(), stepout );
	    dosteer = steerfld_->willSteer();
	}
	else
	    mSetBool( Tutorial::weaksmoothStr(),
		    		smoothstrengthfld_->getBoolValue() );
    }

    mSetBool( Tutorial::steeringStr(), dosteer );

    return true;
}


bool uiTutorialAttrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    fillInp( steerfld_, desc, 1 );
    return true;
}


void uiTutorialAttrib::steerTypeSel( CallBacker* )                              
{                                                                               
    if ( is2D() && steerfld_->willSteer() && !inpfld_->isEmpty() )              
    {                                                                           
	const char* steertxt = steerfld_->text();                               
	if ( steertxt )                                                         
	{                                                                       
	    LineKey inp( inpfld_->getInput() );                                 
	    LineKey steer( steertxt );                                          
	    if ( strcmp( inp.lineName(), steer.lineName() ) )                   
		steerfld_->clearInpField();
	}
    }
}

