/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          March  2011
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uigrubbsfilterattrib.h"
#include "grubbsfilterattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uistepoutsel.h"

using namespace Attrib;

static const char* replacetypestr[] =
{
    "Average",
    "Median",
    "Threshold",
    "GrubbsValue",
    "Interpolate",
    0
};


mInitAttribUI(uiGrubbsFilterAttrib,GrubbsFilter,"Grubbs Filter",sKeyFilterGrp())


uiGrubbsFilterAttrib::uiGrubbsFilterAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d,"mToDoHelpID")

{
    inpfld_ = createInpFld( is2d );

    grubbsvalfld_ = new uiGenInput( this, "Cutoff Grubbs Value",
				    FloatInpSpec() );
    grubbsvalfld_->attach( alignedBelow, inpfld_ );

    gatefld_ = new uiGenInput( this, gateLabel(),
			       FloatInpIntervalSpec().setName("Z start",0)
			      			     .setName("Z stop",1) );
    gatefld_->attach( alignedBelow, grubbsvalfld_ );

    stepoutfld_ = new uiStepOutSel( this, is2d );
    stepoutfld_->attach( alignedBelow, gatefld_ );
    stepoutfld_->setFieldNames( is2d ? "Trace Nr Stepout" : "Inl Stepout",
	    		       "Crl Stepout" );

    replacetype_ = new uiGenInput( this, "Replace Type",
	    			   StringListInpSpec(replacetypestr) );
    replacetype_->attach( alignedBelow, stepoutfld_ );
    replacetype_->valuechanged.notify(
	    mCB(this,uiGrubbsFilterAttrib,replaceTypChanged) );

    setHAlignObj( gatefld_ );
}


void uiGrubbsFilterAttrib::replaceTypChanged( CallBacker* )
{
    if ( replacetype_->getIntValue() != 3 )
    {
	grubbsvalfld_->display( true );
	return;
    }

    grubbsvalfld_->display( false );
}


bool uiGrubbsFilterAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),GrubbsFilter::attribName()) )
	return false;

    mIfGetFloat(GrubbsFilter::grubbsvalStr(),cogrubbsval,
	    	grubbsvalfld_->setValue(cogrubbsval) )
    mIfGetFloatInterval(GrubbsFilter::gateStr(),gate,gatefld_->setValue(gate))
    mIfGetBinID(GrubbsFilter::stepoutStr(),stepout, 
	        stepoutfld_->setBinID(stepout) )
    mIfGetEnum(GrubbsFilter::replaceValStr(),type,replacetype_->setValue(type))
    return true;
}


bool uiGrubbsFilterAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiGrubbsFilterAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),GrubbsFilter::attribName()) )
	return false;

    mSetFloat( GrubbsFilter::grubbsvalStr(), grubbsvalfld_->getfValue() );
    mSetBinID( GrubbsFilter::stepoutStr(), stepoutfld_->getBinID() );
    mSetFloatInterval( GrubbsFilter::gateStr(), gatefld_->getFInterval() );
    mSetEnum(GrubbsFilter::replaceValStr(),replacetype_->getIntValue());
    
    return true;
}


bool uiGrubbsFilterAttrib::getInput( Attrib::Desc& desc )
{
    inpfld_->processInput();
    fillInp( inpfld_, desc, 0 );
    return true;
}


void uiGrubbsFilterAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( "Cutoff Grubbs Value", GrubbsFilter::grubbsvalStr() );
    params += EvalParam( timegatestr(), GrubbsFilter::gateStr() );
    params += EvalParam( stepoutstr(), GrubbsFilter::stepoutStr() );
}
