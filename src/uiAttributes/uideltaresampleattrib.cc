/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Oct  2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uideltaresampleattrib.cc,v 1.2 2010/04/20 18:09:13 cvskris Exp $";

#include "uideltaresampleattrib.h"
#include "deltaresampleattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"

using namespace Attrib;

mInitAttribUI(uiDeltaResampleAttrib,DeltaResample,"Delta Resample",
		"Trace match")

uiDeltaResampleAttrib::uiDeltaResampleAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.103")
{
    refcubefld_ = createInpFld( is2d, "Input Cube");
    
    deltacubefld_ = createInpFld( is2d, "Delta Cube" );
    deltacubefld_->attach( alignedBelow, refcubefld_ );

    periodfld_ = new uiGenInput( this, "Input is periodic", FloatInpSpec());
    periodfld_->setWithCheck(); periodfld_->setChecked( false );
    periodfld_->attach( alignedBelow, deltacubefld_ );
    
    setHAlignObj( refcubefld_ );
}


bool uiDeltaResampleAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),DeltaResample::attribName()) )
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
    if ( strcmp(desc.attribName(),DeltaResample::attribName()) )
	return false;

    float val = periodfld_->isChecked() ? periodfld_->getfValue() : 0;
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
