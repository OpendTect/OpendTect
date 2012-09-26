/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        P.F.M. de Groot
 Date:          September 2012
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uitextureattrib.h"
#include "textureattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "survinfo.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"
#include "uilabel.h"

using namespace Attrib;

static const char* actionstr[] =
{
    "Contrast",
    "Dissimilarity",
    "Homogeneity",
	"Angular Second Moment",
	"Energy",
	"Entropy",
	"GLCM Mean",
	"GLCM Variance",
	"GLCM Standard Deviation",
	"CLCM Correlation",
    0
};


mInitAttribUI(uiTextureAttrib,Texture,"Texture",sKeyBasicGrp())


uiTextureAttrib::uiTextureAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d)
{
    inpfld_ = createInpFld( is2d );

    gatefld_ = new uiGenInput( this, gateLabel(),
		    FloatInpIntervalSpec().setName("Z start",0)
					      .setName("Z stop",1) );
    gatefld_->attach( alignedBelow, inpfld_ );

    steerfld_ = new uiSteeringSel( this, 0, is2d, false );
    steerfld_->attach( alignedBelow, gatefld_ );

    stepoutfld_ = new uiStepOutSel( this, is2d );
    stepoutfld_->setFieldNames( "Stepout Inl", "Stepout Crl" );
    stepoutfld_->attach( alignedBelow, steerfld_ );

    actionfld_ = new uiGenInput( this, "Output", 
	    			StringListInpSpec(actionstr) );
    actionfld_->attach( alignedBelow, stepoutfld_ );

    glcmsizefld_ = new uiGenInput( this, "GLCM size",
				    BoolInpSpec(true,"16x16","32x32") );
    glcmsizefld_->attach( alignedBelow, actionfld_ );

    scalingtypefld_ = new uiGenInput( this, "Scaling",
			    BoolInpSpec(true,"From 1st trace","Global") );
    scalingtypefld_->attach( alignedBelow, glcmsizefld_ );
    scalingtypefld_->valuechanged.notify( mCB(this,uiTextureAttrib,scalingSel));

    globalmeanfld_ = new uiGenInput( this, "Mean", FloatInpSpec() );
    globalmeanfld_->setElemSzPol(uiObject::Small);
    globalmeanfld_->attach( alignedBelow, scalingtypefld_ );

    globalstdevfld_ = new uiGenInput( this, "Standard Deviation",
	    				    FloatInpSpec() );
    globalstdevfld_->setElemSzPol(uiObject::Small);
    globalstdevfld_->attach( rightOf, globalmeanfld_ );

    label_ = new uiLabel( this, "Tip: Use 'Display histogram' to get mean "
	    			"and standard deviation of the input data." );
    label_->attach( ensureBelow, globalmeanfld_ );
    setHAlignObj( inpfld_ );

    scalingSel(0);
}


bool uiTextureAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Texture::attribName()) )
	return false;

    mIfGetFloatInterval( "gate", gate, gatefld_->setValue(gate) );
    mIfGetFloat( "globalmean", globalmean,
	    		globalmeanfld_->setValue(globalmean) );
    mIfGetFloat( "globalstdev", globalstdev,
	    		globalstdevfld_->setValue(globalstdev) );
 
    mIfGetEnum( Texture::actionStr(), action,
	        actionfld_->setValue(action) );
    mIfGetBinID( Texture::stepoutStr(), stepout,
                stepoutfld_->setBinID(stepout) );
    mIfGetBool( "glcmsize", glcmsize, glcmsizefld_->setValue(glcmsize) );
    mIfGetBool( "scalingtype", scalingtype,
		scalingtypefld_->setValue(scalingtype) );

    scalingSel(0);
    return true;
}


bool uiTextureAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    putInp( steerfld_, desc, 1 );
    return true;
}


bool uiTextureAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Texture::attribName()) )
	return false;

    mSetFloatInterval( "gate", gatefld_->getFInterval() );

    const bool doset = scalingtypefld_->getBoolValue();
    if ( !doset )
    {	
	mSetFloat( "globalmean", globalmeanfld_->getfValue() );
	mSetFloat( "globalstdev", globalstdevfld_->getfValue() );
    }

    bool dosteer = false;
    mSetEnum( Texture::actionStr(), actionfld_->getIntValue() );
 
    BinID stepout( stepoutfld_->getBinID() );
    mSetBinID( Texture::stepoutStr(), stepout );
    dosteer = steerfld_->willSteer();

    mSetBool( Texture::steeringStr(), dosteer );
    mSetBool( "glcmsize", glcmsizefld_->getBoolValue() );
    mSetBool( "scalingtype", scalingtypefld_->getBoolValue() );

    return true;
}


bool uiTextureAttrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    fillInp( steerfld_, desc, 1 );
    return true;
}

void uiTextureAttrib::scalingSel( CallBacker* )
{
    const bool dodisp = scalingtypefld_->getBoolValue();
    globalmeanfld_->display( !dodisp );
    globalstdevfld_->display( !dodisp );
    label_->display( !dodisp );
}

void uiTextureAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr(), "gate" );
    params += EvalParam( stepoutstr(), Texture::stepoutStr() );

}
