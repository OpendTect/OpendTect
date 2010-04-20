/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May  2005
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uisimilarityattrib.cc,v 1.26 2010-04-20 18:09:13 cvskris Exp $";


#include "uisimilarityattrib.h"
#include "similarityattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"

using namespace Attrib;

static const char* extstrs3d[] =
{
	"None",
	"Mirror 90 degrees",
	"Mirror 180 degrees",
	"Full block",
	0
};

static const char* extstrs2d[] =
{
	"None",
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

mInitAttribUI(uiSimilarityAttrib,Similarity,"Similarity",sKeyBasicGrp())


uiSimilarityAttrib::uiSimilarityAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.14")

{
    inpfld = createInpFld( is2d );

    gatefld = new uiGenInput( this, gateLabel(),
			      FloatInpIntervalSpec().setName("Z start",0)
			      			    .setName("Z stop",1) );

    gatefld->attach( alignedBelow, inpfld );

    extfld = new uiGenInput( this, "Extension",
	    		     StringListInpSpec( is2d_ ? extstrs2d : extstrs3d));
    extfld->valuechanged.notify( mCB(this,uiSimilarityAttrib,extSel) );
    extfld->attach( alignedBelow, gatefld );
    
    uiStepOutSel::Setup setup( is2d );
    setup.seltxt( "Trace positions" ).allowneg( true );
    pos0fld = new uiStepOutSel( this, setup );
    pos0fld->setFieldNames( "Trc1 Inl", "Trc1 Crl" );
    pos0fld->attach( alignedBelow, extfld );
    setup.seltxt( "&" );
    pos1fld = new uiStepOutSel( this, setup );
    pos1fld->setFieldNames( "Trc2 Inl", "Trc2 Crl" );
    pos1fld->attach( rightOf, pos0fld );

    stepoutfld = new uiStepOutSel( this, is2d );
    stepoutfld->attach( alignedBelow, extfld );
    stepoutfld->setFieldNames( "Inl Stepout", "Crl Stepout" );

    outpstatsfld = new uiGenInput( this, "Output statistic",
				   StringListInpSpec(outpstrs) );
    outpstatsfld->attach( alignedBelow, stepoutfld );

    steerfld = new uiSteeringSel( this, 0, is2d );
    steerfld->attach( alignedBelow, outpstatsfld );

    setHAlignObj( pos0fld );

    extSel(0);
}


void uiSimilarityAttrib::extSel( CallBacker* )
{
    const char* ext = extfld->text();
    
    pos0fld->display( strcmp(ext,extstrs3d[3]) );
    pos1fld->display( strcmp(ext,extstrs3d[3]) );
    stepoutfld->display( !strcmp(ext,extstrs3d[3]) );
    outpstatsfld->display( strcmp(ext,extstrs3d[0]) );

    BufferString cursel = outpstatsfld->text();
    StringListInpSpec spec( !strcmp(ext,extstrs3d[3]) ? outpstrsext : outpstrs);
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
		extfld->setText(extstrs3d[extension]) )

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
    const bool mirrorext = !strcmp(ext,extstrs3d[1]) || 
			   !strcmp(ext,extstrs3d[2]);
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
    if ( !strcmp(ext,extstrs3d[3]) )
    {	mSetBinID( Similarity::stepoutStr(), stepoutfld->getBinID() ); }
    else
    {
	mSetBinID( Similarity::pos0Str(), pos0fld->getBinID() );
	mSetBinID( Similarity::pos1Str(), pos1fld->getBinID() );
    }

    BufferStringSet strs( extstrs3d );
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
    if ( selattr && (!strcmp(ext,extstrs3d[1]) || !strcmp(ext,extstrs3d[2])) )
	selattr += 2;
    fillOutput( desc, selattr );

    return true;
}


void uiSimilarityAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr(), Similarity::gateStr() );

    if ( !strcmp(extstrs3d[3],extfld->text()) )
	params += EvalParam( stepoutstr(), Similarity::stepoutStr() );
    else
	params += EvalParam( "Trace positions", Similarity::pos0Str(),
			     Similarity::pos1Str() );
}
