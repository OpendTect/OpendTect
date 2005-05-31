/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uivolstatsattrib.cc,v 1.1 2005-05-31 12:33:55 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uivolstatsattrib.h"
#include "volstatsattrib.h"

#include "attribdesc.h"
#include "uigeninput.h"
#include "uiattrsel.h"
#include "uistepoutsel.h"

using namespace Attrib;

static const char* outpstrs[] =
{
	"Average",
	"Median",
	"Variance",
	"Min",
	"Max",
	"Sum",
	"NormVariance",
	0
};


uiVolStatsAttrib::uiVolStatsAttrib( uiParent* p )
    : uiAttrDescEd(p)
{
    inpfld = getInpFld();

    gatefld = new uiGenInput( this, gateLabel(), FloatInpIntervalSpec() );
    gatefld->attach( alignedBelow, inpfld );

    shapefld = new uiGenInput( this, "Shape", BoolInpSpec("Cylinder","Cube") );
    shapefld->attach( alignedBelow, gatefld );
    
    stepoutfld = new uiStepOutSel( this );
    stepoutfld->attach( alignedBelow, shapefld );

    outpfld = new uiGenInput( this, "Output statistic", 
			      StringListInpSpec(outpstrs) );
    outpfld->attach( alignedBelow, stepoutfld );

    steerfld = new uiSteeringSel( this, 0 );
    steerfld->attach( alignedBelow, outpfld );

    setHAlignObj( inpfld );
}


void uiVolStatsAttrib::set2D( bool yn )
{
    inpfld->set2D( yn );
    stepoutfld->set2D( yn );
    steerfld->set2D( yn );
}


bool uiVolStatsAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),VolumeStatistics::attribName()) )
	return false;

    mIfGetFloatInterval( VolumeStatistics::gateStr(), gate,
	    		 gatefld->setValue(gate) );
    mIfGetBinID( VolumeStatistics::stepoutStr(), stepout,
	         stepoutfld->setBinID(stepout) );
    mIfGetEnum( VolumeStatistics::shapeStr(), shape,
	        shapefld->setValue(!shape) );
    return true;
}


bool uiVolStatsAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    putInp( steerfld, desc );
    return true;
}


bool uiVolStatsAttrib::setOutput( const Desc& desc )
{
    outpfld->setValue( desc->selectedOutput() );
    return true;
}


bool uiVolStatsAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),VolumeStatistics::attribName()) )
	return false;

    mSetFloatInterval( VolumeStatistics::gateStr(), gatefld->getFInterval() );
    mSetBinID( VolumeStatistics::stepoutStr(), stepoutfld->binID() );
    mSetEnum( VolumeStatistics::shapeStr(), shapefld->getBoolValue() );
    mSetBool( VolumeStatistics::steerStr(), steerfld->willSteer() );

    return true;
}


bool uiVolStatsAttrib::getInput( Desc& desc )
{
    inpfld->processInput();
    fillInp( inpfld, desc, 0 );
    fillInp( steerfld, desc, 1 );
    return true;
}


bool uiVolStatsAttrib::getOutput( Desc& desc )
{
    fillOutput( desc, outpfld->getIntValue() );
}
