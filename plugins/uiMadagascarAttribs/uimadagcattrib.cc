/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          Sep  2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimadagcattrib.h"
#include "madagcattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uistepoutsel.h"

using namespace Attrib;

mInitAttribUI(uiMadAGCAttrib,MadAGC,"Madagascar AGC","Madagascar AGC")


uiMadAGCAttrib::uiMadAGCAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,mNoHelpID)
{
    inpfld_ = createInpFld( is2d, "Input Data");
    
    smoothzradiusfld_ = new uiGenInput( this, "Z smoothing radius (samples)",
	    			   	IntInpSpec(0) );
    smoothzradiusfld_->attach( alignedBelow, inpfld_ );

    uiStepOutSel::Setup setup( is2d );
    setup.seltxt( "smoothing stepout" ).allowneg( false );
    smoothradiusfld_ = new uiStepOutSel( this, setup );
    smoothradiusfld_->attach( alignedBelow, smoothzradiusfld_ );
    
    nrrepeatfld_ = new uiGenInput( this, "Repeat AGC scaling", IntInpSpec(0) );
    nrrepeatfld_->attach( alignedBelow, smoothzradiusfld_ );

    setHAlignObj( inpfld_ );
}


bool uiMadAGCAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),MadAGC::attribName()) )
	return false;

    mIfGetInt( MadAGC::smoothzradiusStr(), zrad,
	       smoothzradiusfld_->setValue(zrad) )
    mIfGetBinID( MadAGC::smoothradiusStr(), stepout,
	       smoothradiusfld_->setBinID(stepout) )
    mIfGetInt( MadAGC::nrrepeatStr(), nrtimes,nrrepeatfld_->setValue(nrtimes) )
    return true;
}


bool uiMadAGCAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiMadAGCAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),MadAGC::attribName()) )
	return false;

    mSetInt( MadAGC::smoothzradiusStr(), smoothzradiusfld_->getIntValue() );
    mSetBinID( MadAGC::smoothradiusStr(), smoothradiusfld_->getBinID() );
    mSetInt( MadAGC::nrrepeatStr(), nrrepeatfld_->getIntValue() );
    return true;
}


bool uiMadAGCAttrib::getInput( Attrib::Desc& desc )
{
    inpfld_->processInput();
    fillInp( inpfld_, desc, 0 );
    return true;
}


void uiMadAGCAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( "z smoothing radius", MadAGC::smoothzradiusStr() );
    params += EvalParam( "smoothing stepout", MadAGC::smoothradiusStr() );
    params += EvalParam( "nr times", MadAGC::nrrepeatStr() );
}
