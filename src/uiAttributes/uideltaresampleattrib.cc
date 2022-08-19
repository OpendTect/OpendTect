/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uideltaresampleattrib.h"
#include "deltaresampleattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "od_helpids.h"

using namespace Attrib;

mInitAttribUI(uiDeltaResampleAttrib,DeltaResample,"Delta Resample",
		"Trace match")

uiDeltaResampleAttrib::uiDeltaResampleAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mDeltaResampleHelpID) )
{
    refcubefld_ = createInpFld( is2d, "Input Cube");
    
    deltacubefld_ = createInpFld( is2d, "Delta Cube" );
    deltacubefld_->attach( alignedBelow, refcubefld_ );

    periodfld_ = new uiGenInput( this, tr("Input is periodic"), FloatInpSpec());
    periodfld_->setWithCheck(); periodfld_->setChecked( false );
    periodfld_->attach( alignedBelow, deltacubefld_ );
    
    setHAlignObj( refcubefld_ );
}


bool uiDeltaResampleAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( desc.attribName() != DeltaResample::attribName() )
	return false;

    float period = 0;
    mIfGetFloat( DeltaResample::periodStr(), 
		    per, period = per; periodfld_->setValue(period) )
    periodfld_->setChecked( !mIsUdf(period) && !mIsZero(period,1e-6) );

    return true;
}


bool uiDeltaResampleAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( refcubefld_, desc, 0 );
    putInp( deltacubefld_, desc, 1 );
    return true;
}


bool uiDeltaResampleAttrib::getParameters( Attrib::Desc& desc )
{
    if ( desc.attribName() != DeltaResample::attribName() )
	return false;

    float val = periodfld_->isChecked() ? periodfld_->getFValue() : 0;
    mSetFloat( DeltaResample::periodStr(), val );
    return true;
}


bool uiDeltaResampleAttrib::getInput( Attrib::Desc& desc )
{
    refcubefld_->processInput();
    fillInp( refcubefld_, desc, 0 );
    
    deltacubefld_->processInput();
    fillInp( deltacubefld_, desc, 1 );
    return true;
}
