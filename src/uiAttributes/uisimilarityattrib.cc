/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May  2005
 RCS:           $Id: uisimilarityattrib.cc,v 1.8 2006-01-02 07:06:07 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uisimilarityattrib.h"
#include "similarityattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uistepoutsel.h"
#include "uisteeringsel.h"

using namespace Attrib;

static const char* extstrs[] =
{
	"None",
	"Mirror 90 degrees",
	"Mirror 180 degrees",
	"Full block",
	0
};

static const char* outpstrs[] =
{
	"Average",
	"Min",
	"Max",
	0
};

static const char* outpstrsext[] =
{
    	"Average",
	"Median",
	"Variance",
	"Min",
	"Max",
	0
};


uiSimilarityAttrib::uiSimilarityAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld = getInpFld();

    gatefld = new uiGenInput( this, gateLabel(), FloatInpIntervalSpec() );
    gatefld->setValues( -28, 28 );
    gatefld->attach( alignedBelow, inpfld );

    extfld = new uiGenInput( this, "Extension", StringListInpSpec(extstrs) );
    extfld->valuechanged.notify( mCB(this,uiSimilarityAttrib,extSel) );
    extfld->setValue( extstrs[1] );
    extfld->attach( alignedBelow, gatefld );
    
    pos0fld = new uiStepOutSel( this, "Trace positions" );
    pos0fld->setVal( true, 0 ); pos0fld->setVal( false, 1 );
    pos0fld->attach( alignedBelow, extfld );
    pos1fld = new uiStepOutSel( this, "&" );
    pos1fld->setVal( true, 0 ); pos1fld->setVal( false, -1 );
    pos1fld->attach( rightOf, pos0fld );
    stepoutfld = new uiStepOutSel( this );
    stepoutfld->setVal( true, 1 ); stepoutfld->setVal( false, 1 );
    stepoutfld->attach( alignedBelow, extfld );

    outpstatsfld = new uiGenInput( this, "Output statistic",
				   StringListInpSpec(outpstrs) );
    outpstatsfld->attach( alignedBelow, stepoutfld );

    steerfld = new uiSteeringSel( this, 0 );
    steerfld->attach( alignedBelow, outpstatsfld );

    setHAlignObj( pos0fld );

    extSel(0);
}


void uiSimilarityAttrib::set2D( bool yn )
{
    inpfld->set2D( yn );
    pos0fld->set2D( yn );
    pos1fld->set2D( yn );
    stepoutfld->set2D( yn );
    steerfld->set2D( yn );

    const BufferString selextstr = extfld->text();
    BufferStringSet strs;
    strs.add( extstrs[0] );
    if ( !yn ) strs.add( extstrs[1] );
    strs.add( extstrs[2] ); strs.add( extstrs[3] );
    extfld->newSpec( StringListInpSpec(strs), 0 );
    extfld->setText( selextstr );
    extSel(0);
}


void uiSimilarityAttrib::extSel( CallBacker* )
{
    const char* ext = extfld->text();
    
    pos0fld->display( strcmp(ext,extstrs[3]) );
    pos1fld->display( strcmp(ext,extstrs[3]) );
    stepoutfld->display( !strcmp(ext,extstrs[3]) );
    outpstatsfld->display( strcmp(ext,extstrs[0]) );

    BufferString cursel = outpstatsfld->text();
    StringListInpSpec spec( !strcmp(ext,extstrs[3]) ? outpstrsext : outpstrs );
    outpstatsfld->newSpec( spec, 0 );
    outpstatsfld->setText( cursel );
}


bool uiSimilarityAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Similarity::attribName()) )
	return false;

    mIfGetFloatInterval( Similarity::gateStr(), gate, gatefld->setValue(gate) )
    mIfGetBinID( Similarity::stepoutStr(), stepout, 
	         stepoutfld->setBinID(stepout) )
    mIfGetBinID( Similarity::pos0Str(), pos0, pos0fld->setBinID(pos0) )
    mIfGetBinID( Similarity::pos1Str(), pos1, pos1fld->setBinID(pos1) )
    mIfGetEnum( Similarity::extensionStr(), extension,
	        const char* str = extstrs[extension]; extfld->setText(str) )

    extSel(0);
    return true;
}


bool uiSimilarityAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld, desc, 0 );
    putInp( steerfld, desc, 1 );
    return true;
}


bool uiSimilarityAttrib::setOutput( const Attrib::Desc& desc )
{
    const int selattr = desc.selectedOutput();
    const char* ext = extfld->text();
    const bool mirrorext = !strcmp(ext,extstrs[1]) || !strcmp(ext,extstrs[2]);
    if ( selattr>0 && mirrorext )
	outpstatsfld->setValue( selattr-2 );
    else
	outpstatsfld->setValue( selattr );

    return true;
}


bool uiSimilarityAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Similarity::attribName()) )
	return false;

    const char* ext = extfld->text();
    if ( !strcmp(ext,extstrs[3]) )
    {	mSetBinID( Similarity::stepoutStr(), stepoutfld->binID() ); }
    else
    {
	mSetBinID( Similarity::pos0Str(), pos0fld->binID() );
	mSetBinID( Similarity::pos1Str(), pos1fld->binID() );
    }

    BufferStringSet strs( extstrs );
    mSetEnum( Similarity::extensionStr(), strs.indexOf(ext) );
    mSetFloatInterval( Similarity::gateStr(), gatefld->getFInterval() );
    mSetBool( Similarity::steeringStr(), steerfld->willSteer() );

    return true;
}


bool uiSimilarityAttrib::getInput( Attrib::Desc& desc )
{
    inpfld->processInput();
    fillInp( inpfld, desc, 0 );
    fillInp( steerfld, desc, 1 );
    return true;
}


bool uiSimilarityAttrib::getOutput( Attrib::Desc& desc )
{
    int selattr = outpstatsfld->getIntValue();
    const char* ext = extfld->text();
    if ( selattr && (!strcmp(ext,extstrs[1]) || !strcmp(ext,extstrs[2])) )
	selattr += 2;
    fillOutput( desc, selattr );

    return true;
}


void uiSimilarityAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr, Similarity::gateStr() );

    if ( extfld->getIntValue() == 3 )
	params += EvalParam( stepoutstr, Similarity::stepoutStr() );
    else
	params += EvalParam( "Trace positions", Similarity::pos0Str(),
			     Similarity::pos1Str() );
}
