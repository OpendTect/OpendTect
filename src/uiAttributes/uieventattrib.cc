/*+
 ________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	H. Payraudeau
 Date:		February 2005
 RCS:		$Id: uieventattrib.cc,v 1.10 2007-10-12 09:12:19 cvssulochana Exp $
 ________________________________________________________________________

-*/

#include "uieventattrib.h"
#include "eventattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"


static const char* evtypestrs[] =
{
    "Extremum",
    "Maximum",
    "Minimum",
    "Zero Crossing",
    "Negative to Positive ZC",
    "Positive to Negative ZC",
    "Maximum within the Gate",
    "Minimum within the Gate",
    0
};


static const char* outpstrs[] =
{
    "Peakedness",
    "Steepness",
    "Asymmetry",
    0	
};


mInitAttribUI(uiEventAttrib,Event,"Event",sKeyPatternGrp)


uiEventAttrib::uiEventAttrib( uiParent* p, bool is2d )
        : uiAttrDescEd(p,is2d,"101.0.4")
	  
{
    inpfld = getInpFld();

    issinglefld = new uiGenInput( this, "Use",
			BoolInpSpec(true,"Single event","Multiple events") );
    issinglefld->attach( alignedBelow, inpfld );
    issinglefld->valuechanged.notify( mCB(this,uiEventAttrib,isSingleSel) );

    evtypefld = new uiGenInput( this, "Event type",
				StringListInpSpec(evtypestrs) );
    evtypefld->attach( alignedBelow, issinglefld );
    evtypefld->valuechanged.notify( mCB(this,uiEventAttrib,isGateSel) );
    evtypefld->display(false);
    
    tonextfld = new uiGenInput( this, "Compute distance to",
	                BoolInpSpec(true,"Next event","Previous event") );
    tonextfld->attach( alignedBelow, evtypefld );
    tonextfld->display(false);
    
    outpfld = new uiGenInput( this, "Output", StringListInpSpec(outpstrs) );
    outpfld->attach( alignedBelow, issinglefld);

    gatefld = new uiGenInput( this, gateLabel(), FloatInpIntervalSpec() );
    gatefld->attach( alignedBelow, tonextfld);
    gatefld->display(false);

    setHAlignObj( issinglefld );
}


void uiEventAttrib::isSingleSel( CallBacker* )
{
    const bool issingle = issinglefld-> getBoolValue();
    const int val = evtypefld-> getIntValue();
    evtypefld->display( !issingle );
    tonextfld->display( !issingle && val != 6 && val != 7 );
    gatefld->display( !issingle && ( val == 6 || val == 7 ) );
    outpfld->display( issingle );
}


void uiEventAttrib::isGateSel( CallBacker* )
{
    const int val = evtypefld-> getIntValue();
    const bool issingle = issinglefld-> getBoolValue();
    const bool tgdisplay =  (val == 6 || val == 7 ) ? true : false;
    gatefld->display( tgdisplay && !issingle );
    tonextfld->display( !tgdisplay && !issingle );
}


bool uiEventAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Attrib::Event::attribName()) )
	return false;

    mIfGetBool( Attrib::Event::issingleeventStr(), issingleevent, 
	        issinglefld->setValue(issingleevent) );
    mIfGetBool( Attrib::Event::tonextStr(), tonext,
		tonextfld->setValue(tonext) );
    mIfGetInt( Attrib::Event::eventTypeStr(), eventtype, 
	        evtypefld->setValue(eventtype) );
    mIfGetFloatInterval( Attrib::Event::gateStr(), gate,
			 gatefld->setValue(gate) );

    isSingleSel(0);
    isGateSel(0);

    return true;
}


bool uiEventAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiEventAttrib::setOutput( const Attrib::Desc& desc )
{
    outpfld->setValue( desc.selectedOutput() );
    return true;
}


bool uiEventAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Attrib::Event::attribName()) )
	return false;

    mSetBool( Attrib::Event::issingleeventStr(), issinglefld->getBoolValue() );
    mSetBool( Attrib::Event::tonextStr(), tonextfld->getBoolValue() );
    mSetInt( Attrib::Event::eventTypeStr(), evtypefld->getIntValue() );
    mSetFloatInterval( Attrib::Event::gateStr(), gatefld->getFInterval() );

    return true;
}


bool uiEventAttrib::getInput( Attrib::Desc& desc )
{
    inpfld->processInput();
    fillInp( inpfld, desc, 0 );
    return true;
}


bool uiEventAttrib::getOutput( Attrib::Desc& desc )
{
    const int outp = issinglefld->getBoolValue() ? outpfld->getIntValue() : 0;
    fillOutput( desc, outp );
    return true;
}
