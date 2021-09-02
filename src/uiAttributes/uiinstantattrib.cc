/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
________________________________________________________________________

-*/



#include "uiinstantattrib.h"
#include "instantattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "od_helpids.h"

using namespace Attrib;

static const char* rotphase = "Rotate phase";
const char* uiInstantaneousAttrib::outstrs[] =
{
	"Amplitude",
	"Phase",
	"Frequency",
    	"Hilbert",
	"Amplitude 1st derivative",
	"Amplitude 2nd derivative",
	"Cosine phase",
	"Envelope weighted phase",
	"Envelope weighted frequency",
	"Phase acceleration",
	"Thin bed indicator",
	"Bandwidth",
	"Q factor",
	rotphase,
	0
};


mInitAttribUI(uiInstantaneousAttrib,Instantaneous,"Instantaneous",
              sKeyBasicGrp())


uiInstantaneousAttrib::uiInstantaneousAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mInstantaneousAttribHelpID) )

{
    inpfld = createImagInpFld( is2d );

    outpfld = new uiGenInput( this, uiStrings::sOutput(), 
        StringListInpSpec(outstrs) );
    outpfld->setElemSzPol( uiObject::MedVar );
    outpfld->attach( alignedBelow, inpfld );
    outpfld->valuechanged.notify( mCB(this,uiInstantaneousAttrib,outputSelCB) );

    phaserotfld = new uiLabeledSpinBox(this,tr("Specify angle (deg)"));
    phaserotfld->box()->setInterval( -180, 180 );
    phaserotfld->attach( alignedBelow, outpfld );

    setHAlignObj( inpfld );
}


bool uiInstantaneousAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != Instantaneous::attribName() )
	return false;

    mIfGetFloat( Instantaneous::rotateAngle(), rotangle_, 
	    	 phaserotfld->box()->setValue( rotangle_ ) );

    return true;
}


bool uiInstantaneousAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiInstantaneousAttrib::setOutput( const Desc& desc )
{
    outpfld->setValue( desc.selectedOutput() );
    outputSelCB(0);
    return true;
}


bool uiInstantaneousAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != Instantaneous::attribName() )
	return false;

    mSetFloat( Instantaneous::rotateAngle(), phaserotfld->box()->getFValue() );

    return true;
}


bool uiInstantaneousAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}


bool uiInstantaneousAttrib::getOutput( Desc& desc )
{
    fillOutput( desc, outpfld->getIntValue() );
    return true;
}


void uiInstantaneousAttrib::outputSelCB( CallBacker* )
{
    const bool isrot = FixedString(rotphase) == outstrs[outpfld->getIntValue()];
    phaserotfld->display( isrot );
}


void uiInstantaneousAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{                                                                               
    params += EvalParam( Instantaneous::rotateAngle(),
	    		 Instantaneous::rotateAngle() );   
}
