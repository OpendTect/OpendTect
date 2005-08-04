/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uivolstatsattrib.cc,v 1.3 2005-08-04 14:38:30 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uivolstatsattrib.h"
#include "volstatsattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uigeninput.h"
#include "uiattrsel.h"
#include "uisteeringsel.h"
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


uiVolumeStatisticsAttrib::uiVolumeStatisticsAttrib( uiParent* p )
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


void uiVolumeStatisticsAttrib::set2D( bool yn )
{
    inpfld->set2D( yn );
    stepoutfld->set2D( yn );
    steerfld->set2D( yn );
}


bool uiVolumeStatisticsAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),VolStats::attribName()) )
	return false;

    mIfGetFloatInterval( VolStats::gateStr(), gate,
	    		 gatefld->setValue(gate) );
    mIfGetBinID( VolStats::stepoutStr(), stepout,
	         stepoutfld->setBinID(stepout) );
    mIfGetEnum( VolStats::shapeStr(), shape,
	        shapefld->setValue(!shape) );
    return true;
}


bool uiVolumeStatisticsAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    putInp( steerfld, desc, 1 );
    return true;
}


bool uiVolumeStatisticsAttrib::setOutput( const Desc& desc )
{
    outpfld->setValue( desc.selectedOutput() );
    return true;
}


bool uiVolumeStatisticsAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),VolStats::attribName()) )
	return false;

    mSetFloatInterval( VolStats::gateStr(), gatefld->getFInterval() );
    mSetBinID( VolStats::stepoutStr(), stepoutfld->binID() );
    mSetEnum( VolStats::shapeStr(), shapefld->getBoolValue() );
    mSetBool( VolStats::steeringStr(), steerfld->willSteer() );

    return true;
}


bool uiVolumeStatisticsAttrib::getInput( Desc& desc )
{
    inpfld->processInput();
    fillInp( inpfld, desc, 0 );
    fillInp( steerfld, desc, 1 );
    return true;
}


bool uiVolumeStatisticsAttrib::getOutput( Desc& desc )
{
    fillOutput( desc, outpfld->getIntValue() );
    return true;
}
