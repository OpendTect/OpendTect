/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Aug  2006 / Jan 2018
________________________________________________________________________

-*/

#include "uimatchdeltaattrib.h"
#include "uideltaresampleattrib.h"
#include "matchdeltaattrib.h"
#include "deltaresampleattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "od_helpids.h"

using namespace Attrib;

mInitAttribUI(uiDeltaResampleAttrib,DeltaResample,tr("Delta Resample"),
		sTraceMatchGrp())
mInitGrpDefAttribUI(uiMatchDeltaAttrib,MatchDelta,tr("Match delta"),
		sTraceMatchGrp())


uiMatchDeltaAttrib::uiMatchDeltaAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mMatchDeltaHelpID) )
{
    refcubefld_ = createInpFld( is2d, tr("Reference Cube") );

    mtchcubefld_ = createInpFld( is2d, tr("Match Cube") );
    mtchcubefld_->attach( alignedBelow, refcubefld_ );

    maxshiftfld_ = new uiGenInput( this, zDepLabel(tr("Maximum"),tr("shift")),
				   FloatInpSpec(10) );
    maxshiftfld_->attach( alignedBelow, mtchcubefld_ );

    setHAlignObj( maxshiftfld_ );
}


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


uiRetVal uiMatchDeltaAttrib::getInput( Attrib::Desc& desc )
{
    uiRetVal uirv = fillInp( refcubefld_, desc, 0 );
    uirv.add( fillInp( mtchcubefld_, desc, 1 ) );
    return uirv;
}


void uiMatchDeltaAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( "maxshift", MatchDelta::maxshiftStr() );
}


uiDeltaResampleAttrib::uiDeltaResampleAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mDeltaResampleHelpID) )
{
    refcubefld_ = createInpFld( is2d, tr("Input Cube") );

    deltacubefld_ = createInpFld( is2d, tr("Delta Cube") );
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


uiRetVal uiDeltaResampleAttrib::getInput( Attrib::Desc& desc )
{
    uiRetVal uirv = fillInp( refcubefld_, desc, 0 );
    uirv.add( fillInp( deltacubefld_, desc, 1 ) );
    return uirv;
}
