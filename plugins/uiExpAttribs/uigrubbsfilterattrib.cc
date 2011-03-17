/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          March  2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uigrubbsfilterattrib.cc,v 1.1 2011-03-17 05:23:29 cvssatyaki Exp $";


#include "uigrubbfilterattrib.h"
#include "grubbfilterattrib.h"

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
    "GrubbValue",
    "Interpolate",
    0
};


mInitAttribUI(uiGrubbFilterAttrib,GrubbFilter,"GrubbFilter",sKeyBasicGrp())


uiGrubbFilterAttrib::uiGrubbFilterAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d,"mToDoHelpID")

{
    inpfld_ = createInpFld( is2d );

    grubbvalfld_ = new uiGenInput( this, "Cutoff Grubb Val", FloatInpSpec() );
    grubbvalfld_->attach( alignedBelow, inpfld_ );

    gatefld_ = new uiGenInput( this, gateLabel(),
			       FloatInpIntervalSpec().setName("Z start",0)
			      			     .setName("Z stop",1) );
    gatefld_->attach( alignedBelow, grubbvalfld_ );

    stepoutfld_ = new uiStepOutSel( this, is2d );
    stepoutfld_->attach( alignedBelow, gatefld_ );
    stepoutfld_->setFieldNames( is2d ? "Trace Nr Stepout" : "Inl Stepout",
	    		       "Crl Stepout" );

    replacetype_ = new uiGenInput( this, "Replace Type",
	    			   StringListInpSpec(replacetypestr) );
    replacetype_->attach( alignedBelow, stepoutfld_ );
}


bool uiGrubbFilterAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),GrubbFilter::attribName()) )
	return false;

    mIfGetFloat(GrubbFilter::grubvalStr(),cogrubbval,
	    	grubbvalfld_->setValue(cogrubbval) )
    mIfGetFloatInterval(GrubbFilter::gateStr(),gate,gatefld_->setValue(gate))
    mIfGetBinID(GrubbFilter::stepoutStr(),stepout, 
	        stepoutfld_->setBinID(stepout) )
    mIfGetEnum(GrubbFilter::replaceValStr(),type,replacetype_->setValue(type))
    return true;
}


bool uiGrubbFilterAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiGrubbFilterAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),GrubbFilter::attribName()) )
	return false;

    mSetFloat( GrubbFilter::grubvalStr(), grubbvalfld_->getfValue() );
    mSetBinID( GrubbFilter::stepoutStr(), stepoutfld_->getBinID() );
    mSetFloatInterval( GrubbFilter::gateStr(), gatefld_->getFInterval() );
    mSetEnum(GrubbFilter::replaceValStr(),replacetype_->getIntValue());
    
    return true;
}


bool uiGrubbFilterAttrib::getInput( Attrib::Desc& desc )
{
    inpfld_->processInput();
    fillInp( inpfld_, desc, 0 );
    return true;
}


void uiGrubbFilterAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( "Cut Off GrubbValue", GrubbFilter::grubvalStr() );
    params += EvalParam( timegatestr(), GrubbFilter::gateStr() );
    params += EvalParam( stepoutstr(), GrubbFilter::stepoutStr() );
}
