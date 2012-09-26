/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	H. Payraudeau
 Date:		February 2005
 ________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";



#include "uieventattrib.h"
#include "eventattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uilabel.h"


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


mInitAttribUI(uiEventAttrib,Event,"Event",sKeyPatternGrp())


uiEventAttrib::uiEventAttrib( uiParent* p, bool is2d )
        : uiAttrDescEd(p,is2d,"101.0.4")
	  
{
    inpfld = createInpFld( is2d );

    issinglefld = new uiGenInput( this, "Use",
			BoolInpSpec(true,"Single event","Multiple events") );
    issinglefld->attach( alignedBelow, inpfld );
    issinglefld->valuechanged.notify( mCB(this,uiEventAttrib,isSingleSel) );

    evtypefld = new uiGenInput( this, "Event type",
				StringListInpSpec(evtypestrs) );
    evtypefld->attach( alignedBelow, issinglefld );
    evtypefld->valuechanged.notify( mCB(this,uiEventAttrib,isGateSel) );
    evtypefld->display(false);
    
    tonextlblfld = new uiLabel( this,
	    		"Compute distance between 2 consecutive events" );
    tonextlblfld->attach( leftAlignedBelow, evtypefld );

    outampfld = new uiGenInput( this, "",
	       BoolInpSpec(true,"Compute distance between 2 consecutive events",
		   		"Output Amplitude") );
    outampfld->valuechanged.notify( mCB(this,uiEventAttrib,outAmpSel) );
    outampfld->attach( leftAlignedBelow, evtypefld );

    tonextfld = new uiGenInput( this, "starting from",
	    			BoolInpSpec(true,"Top","Bottom") );
    tonextfld->attach( centeredBelow, tonextlblfld );
    tonextfld->display(false);
    
    outpfld = new uiGenInput( this, "Output", StringListInpSpec(outpstrs) );
    outpfld->attach( alignedBelow, issinglefld);

    gatefld = new uiGenInput( this, gateLabel(),
	    		      FloatInpIntervalSpec().setName("Z start",0)
			      			    .setName("Z stop",1) );
    gatefld->attach( alignedBelow, tonextfld);
    gatefld->display(false);

    setHAlignObj( issinglefld );
}


void uiEventAttrib::isSingleSel( CallBacker* )
{
    const bool issingle = issinglefld-> getBoolValue();
    const int val = evtypefld-> getIntValue();
    const bool iszc = !issingle && ( val==3 || val==4 || val==5 );
    const bool outamp = !outampfld-> getBoolValue();
    evtypefld->display( !issingle );
    tonextlblfld->display( iszc );
    tonextfld->display( !issingle && val != 6 && val != 7 && !outamp );
    gatefld->display( !issingle && ( val == 6 || val == 7 ) );
    outpfld->display( issingle );
    outampfld->display( !issingle && !iszc );
}


void uiEventAttrib::isGateSel( CallBacker* )
{
    const int val = evtypefld-> getIntValue();
    const bool issingle = issinglefld-> getBoolValue();
    const bool tgdisplay =  (val == 6 || val == 7 ) ? true : false;
    const bool iszc = !issingle && ( val==3 || val==4 || val==5 );
    const bool outamp = !outampfld-> getBoolValue();
    gatefld->display( tgdisplay && !issingle );
    tonextfld->display( !tgdisplay && !issingle && !outamp );
    tonextlblfld->display( iszc );
    outampfld->display( !issingle && !iszc );
}


void uiEventAttrib::outAmpSel( CallBacker* )
{
    const int val = evtypefld-> getIntValue();
    const bool issingle = issinglefld-> getBoolValue();
    const bool tgdisplay =  (val == 6 || val == 7 ) ? true : false;
    const bool outamp = !outampfld-> getBoolValue();
    tonextfld->display( !tgdisplay && !issingle && !outamp );
}


bool uiEventAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Attrib::Event::attribName()) )
	return false;

    mIfGetBool( Attrib::Event::issingleeventStr(), issingleevent, 
	        issinglefld->setValue(issingleevent) );
    mIfGetBool( Attrib::Event::tonextStr(), tonext,
		tonextfld->setValue(tonext) );
    mIfGetBool( Attrib::Event::outampStr(), outamp,
		outampfld->setValue(!outamp) );
    mIfGetInt( Attrib::Event::eventTypeStr(), eventtype, 
	        evtypefld->setValue(eventtype) );
    mIfGetFloatInterval( Attrib::Event::gateStr(), gate,
			 gatefld->setValue(gate) );

    isSingleSel(0);
    isGateSel(0);
    outAmpSel(0);

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
    mSetBool( Attrib::Event::outampStr(), !outampfld->getBoolValue() );
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
