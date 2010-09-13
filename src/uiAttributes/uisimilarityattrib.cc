/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May  2005
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uisimilarityattrib.cc,v 1.27 2010-09-13 14:10:33 cvshelene Exp $";


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
	"Coherency-like",
	0
};

static const char* extstrs2d[] =
{
	"None",
	"Mirror 180 degrees",
	"Full block",
	"Coherency-like",
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

static const char* outpcoh2dstrs[] =
{
	"Average",
	"Median",
	"Variance",
	"Min",
	"Max",
	"Trace Dip (max coherency)",
	0
};

static const char* outpcoh3dstrs[] =
{
    	"Average",
	"Median",
	"Variance",
	"Min",
	"Max",
	"Inline Dip (max coherency)",
	"Crossline Dip (max coherency)",
	0
};

mInitAttribUI(uiSimilarityAttrib,Similarity,"Similarity",sKeyBasicGrp())


uiSimilarityAttrib::uiSimilarityAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.14")

{
    inpfld_ = createInpFld( is2d );

    gatefld_ = new uiGenInput( this, gateLabel(),
			      FloatInpIntervalSpec().setName("Z start",0)
			      			    .setName("Z stop",1) );

    gatefld_->attach( alignedBelow, inpfld_ );

    extfld_ = new uiGenInput( this, "Extension",
	    		     StringListInpSpec( is2d_ ? extstrs2d : extstrs3d));
    extfld_->valuechanged.notify( mCB(this,uiSimilarityAttrib,extSel) );
    extfld_->attach( alignedBelow, gatefld_ );
    
    uiStepOutSel::Setup setup( is2d );
    setup.seltxt( "Trace positions" ).allowneg( true );
    pos0fld_ = new uiStepOutSel( this, setup );
    pos0fld_->setFieldNames( "Trc1 Inl", "Trc1 Crl" );
    pos0fld_->attach( alignedBelow, extfld_ );
    setup.seltxt( "&" );
    pos1fld_ = new uiStepOutSel( this, setup );
    pos1fld_->setFieldNames( "Trc2 Inl", "Trc2 Crl" );
    pos1fld_->attach( rightOf, pos0fld_ );

    stepoutfld_ = new uiStepOutSel( this, is2d );
    stepoutfld_->attach( alignedBelow, extfld_ );
    stepoutfld_->setFieldNames( "Inl Stepout", "Crl Stepout" );

    outpstatsfld_ = new uiGenInput( this, "Output statistic",
				   StringListInpSpec(outpstrs) );
    outpstatsfld_->attach( alignedBelow, stepoutfld_ );

    steerfld_ = new uiSteeringSel( this, 0, is2d );
    steerfld_->attach( alignedBelow, outpstatsfld_ );

    BufferString mdlbl = "Maximum dip";
    mdlbl += zIsTime() ? " (us/m)" : " (mm/m)";
    maxdipfld_ = new uiGenInput( this, mdlbl, FloatInpSpec() ); 
    maxdipfld_->attach( alignedBelow, outpstatsfld_ );                   
										
    BufferString ddlbl = "Delta dip";                                 
    ddlbl += zIsTime() ? " (us/m)" : " (mm/m)";
    deltadipfld_ = new uiGenInput( this, ddlbl, FloatInpSpec() );
    deltadipfld_->attach( alignedBelow, maxdipfld_ );

    setHAlignObj( pos0fld_ );

    extSel(0);
}


void uiSimilarityAttrib::extSel( CallBacker* )
{
    const char* ext = extfld_->text();
    
    pos0fld_->display( strcmp(ext,extstrs3d[3]) && strcmp(ext,extstrs3d[4]) );
    pos1fld_->display( strcmp(ext,extstrs3d[3]) && strcmp(ext,extstrs3d[4]) );
    stepoutfld_->display( !strcmp(ext,extstrs3d[3])||!strcmp(ext,extstrs3d[4]));
    outpstatsfld_->display( strcmp(ext,extstrs3d[0]) );
    maxdipfld_->display( !strcmp(ext,extstrs3d[4]) );
    deltadipfld_->display( !strcmp(ext,extstrs3d[4]) );

    BufferString cursel = outpstatsfld_->text();
    const char** outlist = !strcmp(ext,extstrs3d[3])
				? outpstrsext
				: !strcmp(ext,extstrs3d[4])
				    ? ( is2d_ ? outpcoh2dstrs : outpcoh3dstrs )
				    : outpstrs;
    StringListInpSpec spec( outlist );
    outpstatsfld_->newSpec( spec, 0 );
    outpstatsfld_->setText( cursel );
}


bool uiSimilarityAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Similarity::attribName()) )
	return false;

    mIfGetFloatInterval( Similarity::gateStr(), gate, gatefld_->setValue(gate) )
    mIfGetBinID( Similarity::stepoutStr(), stepout, 
	         stepoutfld_->setBinID(stepout) )
    mIfGetBinID( Similarity::pos0Str(), pos0, pos0fld_->setBinID(pos0) )
    mIfGetBinID( Similarity::pos1Str(), pos1, pos1fld_->setBinID(pos1) )
    mIfGetEnum( Similarity::extensionStr(), extension,
		extfld_->setText(extstrs3d[extension]) )
    mIfGetFloat( Similarity::maxdipStr(), maxdip, maxdipfld_->setValue(maxdip));
    mIfGetFloat( Similarity::ddipStr(), ddip, deltadipfld_->setValue(ddip) );

    extSel(0);
    return true;
}


bool uiSimilarityAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    putInp( steerfld_, desc, 1 );
    return true;
}


bool uiSimilarityAttrib::setOutput( const Attrib::Desc& desc )
{
    const int selattr = desc.selectedOutput();
    const char* ext = extfld_->text();
    const bool mirrorext = !strcmp(ext,extstrs3d[1]) || 
			   !strcmp(ext,extstrs3d[2]);
    if ( selattr>0 && mirrorext )
	outpstatsfld_->setValue( selattr-2 );
    else
	outpstatsfld_->setValue( selattr );

    return true;
}


bool uiSimilarityAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Similarity::attribName()) )
	return false;

    const char* ext = extfld_->text();
    if ( !strcmp(ext,extstrs3d[3]) || !strcmp(ext,extstrs3d[4]) )
    {	mSetBinID( Similarity::stepoutStr(), stepoutfld_->getBinID() ); }
    else
    {
	mSetBinID( Similarity::pos0Str(), pos0fld_->getBinID() );
	mSetBinID( Similarity::pos1Str(), pos1fld_->getBinID() );
    }

    BufferStringSet strs( extstrs3d );
    mSetEnum( Similarity::extensionStr(), strs.indexOf(ext) );
    mSetFloatInterval( Similarity::gateStr(), gatefld_->getFInterval() );
    mSetBool( Similarity::steeringStr(), steerfld_->willSteer() );

    if ( !strcmp(ext,extstrs3d[4]) )
    {
	mSetFloat( Similarity::maxdipStr(), maxdipfld_->getfValue() );
	mSetFloat( Similarity::ddipStr(), deltadipfld_->getfValue() );
    }

    return true;
}


bool uiSimilarityAttrib::getInput( Attrib::Desc& desc )
{
    inpfld_->processInput();
    fillInp( inpfld_, desc, 0 );
    fillInp( steerfld_, desc, 1 );
    return true;
}


bool uiSimilarityAttrib::getOutput( Attrib::Desc& desc )
{
    int selattr = outpstatsfld_->getIntValue();
    const char* ext = extfld_->text();
    if ( selattr && (!strcmp(ext,extstrs3d[1]) || !strcmp(ext,extstrs3d[2])) )
	selattr += 2;
    fillOutput( desc, selattr );

    return true;
}


void uiSimilarityAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr(), Similarity::gateStr() );

    if (   !strcmp(extstrs3d[3],extfld_->text())
	|| !strcmp(extstrs3d[4],extfld_->text()) )
	params += EvalParam( stepoutstr(), Similarity::stepoutStr() );
    else
	params += EvalParam( "Trace positions", Similarity::pos0Str(),
			     Similarity::pos1Str() );
}
