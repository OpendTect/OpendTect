/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimatchdeltaattrib.h"
#include "matchdeltaattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "od_helpids.h"

using namespace Attrib;

mInitAttribUI(uiMatchDeltaAttrib,MatchDelta,"Match delta","Trace match")


uiMatchDeltaAttrib::uiMatchDeltaAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mMatchDeltaHelpID) )
{
    refcubefld_ = createInpFld( is2d, "Reference Cube");
    
    mtchcubefld_ = createInpFld( is2d, "Match Cube" );
    mtchcubefld_->attach( alignedBelow, refcubefld_ );

    maxshiftfld_ = new uiGenInput( this, zDepLabel(tr("Maximum"),tr("shift")),
	    			   FloatInpSpec(10) );
    maxshiftfld_->attach( alignedBelow, mtchcubefld_ );
    
    setHAlignObj( maxshiftfld_ );
}


uiMatchDeltaAttrib::~uiMatchDeltaAttrib()
{}


bool uiMatchDeltaAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( desc.attribName() != MatchDelta::attribName() )
	return false;

    mIfGetFloat( MatchDelta::maxshiftStr(), 
		    maxshift, maxshiftfld_->setValue(maxshift) )
    return true;
}


bool uiMatchDeltaAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( refcubefld_, desc, 0 );
    putInp( mtchcubefld_, desc, 1 );
    return true;
}


bool uiMatchDeltaAttrib::getParameters( Attrib::Desc& desc )
{
    if ( desc.attribName() != MatchDelta::attribName() )
	return false;

    mSetFloat( MatchDelta::maxshiftStr(), maxshiftfld_->getFValue() );
    return true;
}


bool uiMatchDeltaAttrib::getInput( Attrib::Desc& desc )
{
    refcubefld_->processInput();
    fillInp( refcubefld_, desc, 0 );
    
    mtchcubefld_->processInput();
    fillInp( mtchcubefld_, desc, 1 );
    return true;
}


void uiMatchDeltaAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( "maxshift", MatchDelta::maxshiftStr() );
}
