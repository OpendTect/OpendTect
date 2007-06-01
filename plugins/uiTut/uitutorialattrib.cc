/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          May 2007
 RCS:           $Id: uitutorialattrib.cc,v 1.1 2007-06-01 06:35:45 cvsraman Exp $
________________________________________________________________________

-*/

#include "uitutorialattrib.h"
#include "tutorialattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "survinfo.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"

using namespace Attrib;


static const char* actionstr[] =
{
    "Scale",
    "Square",
    "Smooth",
    0
};


mInitAttribUI(uiTutorialAttrib,Tutorial,"Tutorial",sKeyBasicGrp)


uiTutorialAttrib::uiTutorialAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d)
{
    inpfld_ = getInpFld();

    actionfld_ = new uiGenInput( this, "Action", 
	    			StringListInpSpec(actionstr) );
    actionfld_->valuechanged.notify( mCB(this,uiTutorialAttrib,actionSel) );
    actionfld_->attach( alignedBelow, inpfld_ );

    smoothszfld_ = new uiGenInput( this, "Filter strength",
	                        BoolInpSpec(true,"Low","High") );
    smoothszfld_->attach( alignedBelow, actionfld_ );

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

    factorfld_->display( actval==0 );
    shiftfld_->display( actval==0 );
    smoothszfld_->display( actval==2 );
}


bool uiTutorialAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Tutorial::attribName()) )
	return false;

    mIfGetEnum( Tutorial::actionStr(), action,
	        actionfld_->setValue(action) );
    mIfGetFloat( Tutorial::factorStr(), factor, factorfld_->setValue(factor) );
    mIfGetFloat( Tutorial::shiftStr(), shift, shiftfld_->setValue(shift) );
    mIfGetBool( Tutorial::smoothStr(), smooth, smoothszfld_->setValue(smooth) );
    actionSel(0);
    return true;
}


bool uiTutorialAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiTutorialAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Tutorial::attribName()) )
	return false;

    mSetEnum( Tutorial::actionStr(), actionfld_->getIntValue() );
    mSetFloat( Tutorial::factorStr(), factorfld_->getfValue() );
    mSetFloat( Tutorial::shiftStr(), shiftfld_->getfValue() );
    mSetBool( Tutorial::smoothStr(), smoothszfld_->getBoolValue() );

    return true;
}


bool uiTutorialAttrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    return true;
}

