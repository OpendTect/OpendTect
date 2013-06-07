/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May  2005
________________________________________________________________________

-*/

static const char* rcsID = "$Id$";


#include "uisimilarityattrib.h"
#include "similarityattrib.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uistepoutsel.h"

using namespace Attrib;

static const char* extstrs3d[] =
{
	"None",
	"Mirror 90 degrees",
	"Mirror 180 degrees",
	"Full block",
	"Cross",
	"All Directions",
	"Diagonal",
	0
};

static const char* extstrs2d[] =
{
	"None",
	"Mirror 180 degrees",
	"Full block",
	"All Directions",		//TODO get good name from users
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

static const char* outpdip3dstrs[] =
{
	"Inline Dip",
	"Crossline Dip",
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

    steerfld_ = new uiSimilarityAttrib::uiSimiSteeringSel( this, 0, is2d );
    steerfld_->typeSelected.notify( mCB(this,uiSimilarityAttrib,steerTypeSel) );
    steerfld_->attach( alignedBelow, stepoutfld_ );

    BufferString mdlbl = "Maximum dip";
    mdlbl += zIsTime() ? " (us/m)" : " (mm/m)";
    maxdipfld_ = new uiGenInput( this, mdlbl, FloatInpSpec() ); 
    maxdipfld_->attach( alignedBelow, steerfld_ );                   
										
    BufferString ddlbl = "Delta dip";                                 
    ddlbl += zIsTime() ? " (us/m)" : " (mm/m)";
    deltadipfld_ = new uiGenInput( this, ddlbl, FloatInpSpec() );
    deltadipfld_->attach( alignedBelow, maxdipfld_ );

    dooutpstatsfld_ = new uiGenInput( this, "Output",
			BoolInpSpec(true,"Statistics","Dip at max similarity"));
    dooutpstatsfld_->valuechanged.notify( mCB(this,uiSimilarityAttrib,outSel) );
    dooutpstatsfld_->attach( alignedBelow, deltadipfld_ );

    outpstatsfld_ = new uiGenInput( this, "Output statistic",
				   StringListInpSpec(outpstrs) );
    outpstatsfld_->attach( alignedBelow, dooutpstatsfld_ );

    outpdipfld_ = new uiGenInput( this, "Select output",
				  StringListInpSpec(outpdip3dstrs) );
    outpdipfld_->attach( alignedBelow, dooutpstatsfld_ );
    outpdipfld_->display(false);

    setHAlignObj( pos0fld_ );

    extSel(0);
    steerTypeSel(0);
    outSel(0);
}


void uiSimilarityAttrib::extSel( CallBacker* )
{
    const char* ext = extfld_->text();
    
    const bool iscube = !strcmp(ext,extstrs3d[3]);
    const bool iscross = !strcmp(ext,extstrs3d[4]);
    const bool isalldir = !strcmp(ext,extstrs3d[5]);
    const bool isdiag = !strcmp(ext,extstrs3d[6]);
    const bool needstepoutfld = iscube || iscross || isalldir || isdiag;
    pos0fld_->display( !needstepoutfld);
    pos1fld_->display( !needstepoutfld );
    stepoutfld_->display( needstepoutfld );
    outpstatsfld_->display( strcmp(ext,extstrs3d[0]) );
    if ( !iscross )
    {
	dooutpstatsfld_->setValue( true );
	outSel(0);
    }

    BufferString cursel = outpstatsfld_->text();
    StringListInpSpec spec( !strcmp(ext,extstrs3d[3]) ? outpstrsext : outpstrs);
    outpstatsfld_->newSpec( spec, 0 );
    outpstatsfld_->setText( cursel );
}


void uiSimilarityAttrib::outSel(CallBacker*)
{
    const bool wantbrowsedip = steerfld_->wantBrowseDip();
    const bool outstats = !wantbrowsedip || dooutpstatsfld_->getBoolValue();
    outpdipfld_->display( !outstats );
    outpstatsfld_->display( outstats );
    if ( wantbrowsedip && !outstats )
	extfld_->setText(extstrs3d[4]);
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
    mIfGetBool( Similarity::browsedipStr(), bdip,
	    	steerfld_->setType( bdip ? steerfld_->browseDipIdxInList() :0));

    extSel(0);
    steerTypeSel(0);
    return true;
}


bool uiSimilarityAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld_, desc, 0 );

    bool browsedip = false;
    mIfGetBool( Similarity::browsedipStr(), bdip, browsedip = bdip );
    if ( !browsedip )
	putInp( steerfld_, desc, 1 );

    return true;
}


bool uiSimilarityAttrib::setOutput( const Attrib::Desc& desc )
{
    const int selattr = desc.selectedOutput();
    const char* ext = extfld_->text();
    const bool mirrorext = !strcmp(ext,extstrs3d[1]) || 
			   !strcmp(ext,extstrs3d[2]);
    dooutpstatsfld_->setValue( selattr<5 );
    
    if ( selattr<5 )
    {
	if ( selattr>0 && mirrorext )
	    outpstatsfld_->setValue( selattr-2 );
	else
	    outpstatsfld_->setValue( selattr );
    }
    else
	outpdipfld_->setValue( selattr-5 );

    outSel(0);
    return true;
}


bool uiSimilarityAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Similarity::attribName()) )
	return false;

    const char* ext = extfld_->text();
    if ( !strcmp(ext,extstrs3d[3]) || !strcmp(ext,extstrs3d[4]) 
	 || !strcmp(ext,extstrs3d[5]) || !strcmp(ext,extstrs3d[6]) )
    {	mSetBinID( Similarity::stepoutStr(), stepoutfld_->getBinID() ); }
    else
    {
	mSetBinID( Similarity::pos0Str(), pos0fld_->getBinID() );
	mSetBinID( Similarity::pos1Str(), pos1fld_->getBinID() );
    }

    BufferStringSet strs( extstrs3d );
    mSetEnum( Similarity::extensionStr(), strs.indexOf(ext) );
    mSetFloatInterval( Similarity::gateStr(), gatefld_->getFInterval() );

    const bool usesteer = steerfld_->willSteer();
    mSetBool( Similarity::steeringStr(), usesteer );

    if ( !usesteer )
    {
	const bool wantbdip = steerfld_->wantBrowseDip();
	mSetBool( Similarity::browsedipStr(), wantbdip );
	if ( wantbdip )
	{
	    mSetFloat( Similarity::maxdipStr(), maxdipfld_->getfValue() );
	    mSetFloat( Similarity::ddipStr(), deltadipfld_->getfValue() );
	}
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
    int selattr = 0;
    const bool wantbdip = steerfld_->wantBrowseDip();
    if ( wantbdip && !dooutpstatsfld_->getBoolValue() )
	selattr = outpdipfld_->getIntValue() + 5;
    else
    {
	selattr = outpstatsfld_->getIntValue();
	const char* ext = extfld_->text();
	if (selattr && (!strcmp(ext,extstrs3d[1]) || !strcmp(ext,extstrs3d[2])))
	    selattr += 2;
    }

    fillOutput( desc, selattr );
    return true;
}


void uiSimilarityAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr(), Similarity::gateStr() );

    if ( !strcmp(extstrs3d[3],extfld_->text())
	    || !strcmp(extstrs3d[4],extfld_->text())
	    || !strcmp(extstrs3d[5],extfld_->text())
	    || !strcmp(extstrs3d[6],extfld_->text()) )
	params += EvalParam( stepoutstr(), Similarity::stepoutStr() );
    else
	params += EvalParam( "Trace positions", Similarity::pos0Str(),
			     Similarity::pos1Str() );
}


void uiSimilarityAttrib::steerTypeSel(CallBacker*)
{
    const bool wantbdip = steerfld_->wantBrowseDip();
    maxdipfld_->display( wantbdip );
    deltadipfld_->display( wantbdip );
    dooutpstatsfld_->display( wantbdip );

    if ( is2D() && steerfld_->willSteer() && !inpfld_->isEmpty() )
    {
	const char* steertxt = steerfld_->text();
	if ( steertxt )
	{
	    LineKey inp( inpfld_->getInput() );
	    LineKey steer( steertxt );
	    if ( strcmp( inp.lineName(), steer.lineName() ) )
		steerfld_->clearInpField();
	}
    }

    outSel(0);
}


uiSimilarityAttrib::uiSimiSteeringSel::uiSimiSteeringSel( uiParent* p,
						    const Attrib::DescSet* dset,
						    bool is2d )
    : uiSteeringSel( p, dset, is2d, true, false )
    , typeSelected(this)
{
    const char* res = uiAF().attrNameOf( "Curvature" );
    if ( !res )
    {
	BufferStringSet steertyps;
	steertyps.add( "None" );
	typfld_ = new uiGenInput( this, "Steering",
				  StringListInpSpec(steertyps) );
	typfld_->valuechanged.notify(
		    mCB(this,uiSimilarityAttrib::uiSimiSteeringSel,typeSel));
    }
    else
	createFields();

    DataInpSpec* inpspec = const_cast<DataInpSpec*>(typfld_->dataInpSpec());
    mDynamicCastGet(StringListInpSpec*,listspec,inpspec);
    if ( !listspec ) return;

    listspec->addString("Browse dip");
//    typfld_->newSpec(listspec,0);
    setHAlignObj( typfld_ );
}


void uiSimilarityAttrib::uiSimiSteeringSel::typeSel(CallBacker*)
{
    typeSelected.trigger();
    uiSteeringSel::typeSel(0);
}


bool uiSimilarityAttrib::uiSimiSteeringSel::willSteer() const
{
    if ( !typfld_ ) return false;

    int typ = typfld_->getIntValue();
    return typ && !wantBrowseDip();
}


bool uiSimilarityAttrib::uiSimiSteeringSel::wantBrowseDip() const
{
    if ( !typfld_ ) return false;

    const char* hassteerplug = uiAF().attrNameOf( "Curvature" );
    int typ = typfld_->getIntValue();

    return typ == browseDipIdxInList();
}


int uiSimilarityAttrib::uiSimiSteeringSel::browseDipIdxInList() const
{
    const char* hassteerplug = uiAF().attrNameOf( "Curvature" );
    return hassteerplug ? withconstdir_ ? 4 : 3 : 1;
}
