/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uieventattrib.h"
#include "eventattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "od_helpids.h"


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
        : uiAttrDescEd(p,is2d, mODHelpKey(mEventAttribHelpID) )

{
    inpfld_ = createInpFld( is2d );

    issinglefld_ = new uiGenInput( this, uiStrings::sUse(),
		BoolInpSpec(true,tr("Single event"),tr("Multiple events")) );
    issinglefld_->attach( alignedBelow, inpfld_ );
    issinglefld_->valuechanged.notify( mCB(this,uiEventAttrib,isSingleSel) );

    evtypefld_ = new uiGenInput( this, tr("Event type"),
				 StringListInpSpec(evtypestrs) );
    evtypefld_->attach( alignedBelow, issinglefld_ );
    evtypefld_->valuechanged.notify( mCB(this,uiEventAttrib,isGateSel) );
    evtypefld_->display( false );

    outampfld_ = new uiGenInput( this, tr("Compute"),
		BoolInpSpec(true, tr("Distance between 2 consecutive events"),
			    uiStrings::phrOutput( uiStrings::sAmplitude() )) );
    outampfld_->valuechanged.notify( mCB(this,uiEventAttrib,outAmpSel) );
    outampfld_->attach( alignedBelow, evtypefld_ );

    tonextfld_ = new uiGenInput( this, tr("starting from"),
		BoolInpSpec(true,tr("Top"), uiStrings::sBottom()) );
    tonextfld_->attach( alignedBelow, outampfld_ );
    tonextfld_->display( false );

    outpfld_ = new uiGenInput( this, uiStrings::sOutput(),
			       StringListInpSpec(outpstrs) );
    outpfld_->attach( alignedBelow, issinglefld_);

    gatefld_ = new uiGenInput( this, gateLabel(),
		FloatInpIntervalSpec().setName("Z start",0)
				      .setName("Z stop",1) );
    gatefld_->attach( alignedBelow, tonextfld_);
    gatefld_->display( false );

    setHAlignObj( issinglefld_ );
}


void uiEventAttrib::isSingleSel( CallBacker* )
{
    const bool issingle = issinglefld_-> getBoolValue();
    const int val = evtypefld_-> getIntValue();
    const bool iszc = !issingle && ( val==3 || val==4 || val==5 );
    if ( iszc ) outampfld_->setValue( true );
    const bool outamp = !outampfld_-> getBoolValue();
    evtypefld_->display( !issingle );
    tonextfld_->display( !issingle && val != 6 && val != 7 && !outamp );
    gatefld_->display( !issingle && ( val == 6 || val == 7 ) );
    outpfld_->display( issingle );
    outampfld_->display( !issingle );
    outampfld_->setSensitive( !iszc );
}


void uiEventAttrib::isGateSel( CallBacker* )
{
    const int val = evtypefld_->getIntValue();
    const bool issingle = issinglefld_-> getBoolValue();
    const bool tgdisplay =  (val == 6 || val == 7 ) ? true : false;
    const bool iszc = !issingle && ( val==3 || val==4 || val==5 );
    if ( iszc ) outampfld_->setValue( true );
    const bool outamp = !outampfld_-> getBoolValue();
    gatefld_->display( tgdisplay && !issingle );
    tonextfld_->display( !tgdisplay && !issingle && !outamp );
    outampfld_->display( !issingle );
    outampfld_->setSensitive( !iszc );
}


void uiEventAttrib::outAmpSel( CallBacker* )
{
    const int val = evtypefld_-> getIntValue();
    const bool issingle = issinglefld_-> getBoolValue();
    const bool tgdisplay =  (val == 6 || val == 7 ) ? true : false;
    const bool outamp = !outampfld_-> getBoolValue();
    tonextfld_->display( !tgdisplay && !issingle && !outamp );
}


bool uiEventAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( desc.attribName() != Attrib::Event::attribName() )
	return false;

    mIfGetBool( Attrib::Event::issingleeventStr(), issingleevent,
	        issinglefld_->setValue(issingleevent) );
    mIfGetBool( Attrib::Event::tonextStr(), tonext,
		tonextfld_->setValue(tonext) );
    mIfGetBool( Attrib::Event::outampStr(), outamp,
		outampfld_->setValue(!outamp) );
    mIfGetInt( Attrib::Event::eventTypeStr(), eventtype,
	        evtypefld_->setValue(eventtype) );
    mIfGetFloatInterval( Attrib::Event::gateStr(), gate,
			 gatefld_->setValue(gate) );

    isSingleSel(0);
    isGateSel(0);
    outAmpSel(0);

    return true;
}


bool uiEventAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiEventAttrib::setOutput( const Attrib::Desc& desc )
{
    outpfld_->setValue( desc.selectedOutput() );
    return true;
}


bool uiEventAttrib::getParameters( Attrib::Desc& desc )
{
    if ( desc.attribName() != Attrib::Event::attribName() )
	return false;

    mSetBool( Attrib::Event::issingleeventStr(), issinglefld_->getBoolValue() );
    mSetBool( Attrib::Event::tonextStr(), tonextfld_->getBoolValue() );
    mSetBool( Attrib::Event::outampStr(), !outampfld_->getBoolValue() );
    mSetInt( Attrib::Event::eventTypeStr(), evtypefld_->getIntValue() );
    mSetFloatInterval( Attrib::Event::gateStr(), gatefld_->getFInterval() );

    return true;
}


bool uiEventAttrib::getInput( Attrib::Desc& desc )
{
    inpfld_->processInput();
    fillInp( inpfld_, desc, 0 );
    return true;
}


bool uiEventAttrib::getOutput( Attrib::Desc& desc )
{
    const int outp = issinglefld_->getBoolValue() ? outpfld_->getIntValue() : 0;
    fillOutput( desc, outp );
    return true;
}
