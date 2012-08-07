/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          May 2007
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: uitutorialattrib.cc,v 1.16 2012-08-07 04:23:04 cvsmahant Exp $";


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
    "Power",
    "Smooth",
    "Invert",
    "Differences",    
    0
};//Modified


mInitAttribUI(uiTutorialAttrib,Tutorial,"Tutorial/Dummy Attribute",sKeyBasicGrp())


uiTutorialAttrib::uiTutorialAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d)
{
    inpfld_ = createInpFld( is2d );

    actionfld_ = new uiGenInput( this, "Action", 
    			StringListInpSpec(actionstr) );
    actionfld_->valuechanged.notify( mCB(this,uiTutorialAttrib,actionSel) );
    actionfld_->attach( alignedBelow, inpfld_ );
    //actionfld_->attach( alignedBelow, stepoutfld_ ); //Does not work!!
    
      
    smoothdirfld_ = new uiGenInput( this, "Smoothing direction",
                        BoolInpSpec(true,"Horizontal(---)","Vertical(---)") );
    smoothdirfld_->valuechanged.notify( mCB(this,uiTutorialAttrib,actionSel) );
    smoothdirfld_->attach( alignedBelow, actionfld_ );

    smoothstrengthfld_ = new uiGenInput( this, "Filter strength",
                        BoolInpSpec(true,"Low Pass","High Pass") );
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

    

     //Modified
    actionfld1_ = new uiGenInput( this, "Duplicate Action",
		                    StringListInpSpec(actionstr) );
    actionfld1_->valuechanged.notify( mCB(this,uiTutorialAttrib,actionSel) );
    actionfld1_->attach( alignedBelow, stepoutfld_ );
    
    //actionfld_->attach( ensureBelow, actionfld1_ ); //Does not work!!
    //actionfld1_->display(true);

    //



    factorfld_ = new uiGenInput( this, "Factor", FloatInpSpec() );
    factorfld_->attach( alignedBelow, actionfld_ );

    shiftfld_ = new uiGenInput( this, "Shift", FloatInpSpec() );
    shiftfld_->attach( alignedBelow, factorfld_ );

    //Modified
    indexfld_ = new uiGenInput( this, "Index", FloatInpSpec() );
    indexfld_->attach( alignedBelow, actionfld_ );
    //



    //Modified
    scalespeedfld_ = new uiGenInput( this, "Scaling speed",
	                                    BoolInpSpec(true,"Slow","Fast") );
    scalespeedfld_->valuechanged.notify( mCB(this,uiTutorialAttrib,actionSel) );
    scalespeedfld_->attach( alignedBelow, shiftfld_ );
    //



    actionSel(0);

    setHAlignObj( inpfld_ );
}


void uiTutorialAttrib::actionSel( CallBacker* )
{
    const int actval = actionfld_->getIntValue();
    const bool horsmooth = smoothdirfld_->getBoolValue();
    const bool scalespeed = scalespeedfld_->getBoolValue();//Modified
    

    //actionfld_->display(actval==0 || actval==1 ||actval==3);//Modified

    factorfld_->display( actval==0);
    shiftfld_->display( actval==0);
    scalespeedfld_->display(actval==0); //Modified
    indexfld_->display(actval==1); //Modified
    actionfld1_->display(!scalespeed);//Modified
    
    smoothdirfld_->display( actval==2 );
    steerfld_->display( actval==2 && horsmooth );
    stepoutfld_->display( actval==2 && horsmooth );
    smoothstrengthfld_->display( actval==2 && !horsmooth );
    
    //Modified/////////////////////////////////////////
    if (actval==3)
    {
	//delete actionfld_;
	//delete actionfld1_;

	//actionfld_ = new uiGenInput( this, "Action",
	//	                        StringListInpSpec(actionstr) );
        //actionfld_->valuechanged.notify( mCB(this,uiTutorialAttrib,actionSel) );
	//   actionfld_->attach( alignedBelow, stepoutfld_ ); //Does not work!!


        //actionfld1_ = new uiGenInput( this, "Duplicate Action",
	//	                         StringListInpSpec(actionstr) );
        //actionfld1_->valuechanged.notify( mCB(this,uiTutorialAttrib,actionSel));
	actionfld1_->display(false);   
    }
    ///////////////////////////////////////////////////


}



bool uiTutorialAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Tutorial::attribName()) )
	return false;

    mIfGetEnum( Tutorial::actionStr(), action,
	        actionfld_->setValue(action) );
    mIfGetFloat( Tutorial::factorStr(), factor, factorfld_->setValue(factor) );
    mIfGetFloat( Tutorial::shiftStr(), shift, shiftfld_->setValue(shift) );
    


    mIfGetFloat( Tutorial::indexStr(), index, indexfld_->setValue(index) );//Modified


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
    
    //Modified
    else if ( actionfld_->getIntValue() == 1 )
    {
	mSetFloat( Tutorial::indexStr(), indexfld_->getfValue() );	
    }

    //

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

