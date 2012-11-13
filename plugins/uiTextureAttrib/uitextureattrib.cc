/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        P.F.M. de Groot
 Date:          September 2012
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "uitextureattrib.h"
#include "textureattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "survinfo.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"

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
    globalminfld_ = new uiGenInput( this, "Input Data Minimum", FloatInpSpec() );
    globalminfld_->setElemSzPol(uiObject::Small);
    globalminfld_->attach( alignedBelow, glcmsizefld_ );
    globalmaxfld_ = new uiGenInput( this, "Maximum",
		    FloatInpSpec() );
    globalmaxfld_->setElemSzPol(uiObject::Small);
    globalmaxfld_->attach( rightOf, globalminfld_ );
    setHAlignObj( inpfld_ );
}


bool uiTextureAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName()!=Texture::attribName() )
	return false;

    mIfGetFloatInterval( Texture::gateStr(), 
			gate, gatefld_->setValue(gate) );
    mIfGetFloat( Texture::globalminStr(), globalmin,
		globalminfld_->setValue(globalmin) );
    mIfGetFloat( Texture::globalmaxStr(), globalmax,
		globalmaxfld_->setValue(globalmax) );
    mIfGetEnum( Texture::actionStr(), action,
		actionfld_->setValue(action) );
    mIfGetBinID( Texture::stepoutStr(), stepout,
		stepoutfld_->setBinID(stepout) );
    mIfGetBool( Texture::glcmsizeStr(), glcmsize,
		glcmsizefld_->setValue(glcmsize) );

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
    if ( desc.attribName()!=Texture::attribName() )
	return false;
    
    const float globalmin = globalminfld_->getfValue();
    const float globalmax = globalmaxfld_->getfValue();
    if ( mIsEqual( globalmin, globalmax, 1e-3 ))
    {
	BufferString errstr = 
	    "Minimum and Maximum values cannot be the same.\n";
	errstr += "Values represent the clipping range of the input.";
	uiMSG().error( errstr.buf() );
	return false;
    }

    mSetFloatInterval( Texture::gateStr(), gatefld_->getFInterval() );
    mSetFloat( Texture::globalminStr(), globalmin );
    mSetFloat( Texture::globalmaxStr(), globalmax );
  
    bool dosteer = false;
    mSetEnum( Texture::actionStr(), actionfld_->getIntValue() );
    BinID stepout( stepoutfld_->getBinID() );
    mSetBinID( Texture::stepoutStr(), stepout );
    dosteer = steerfld_->willSteer();
    mSetBool( Texture::steeringStr(), dosteer );
    mSetBool( Texture::glcmsizeStr(), glcmsizefld_->getBoolValue() );

    return true;
}

bool uiTextureAttrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    fillInp( steerfld_, desc, 1 );
    return true;
}

void uiTextureAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr(), Texture::gateStr() );
    params += EvalParam( stepoutstr(), Texture::stepoutStr() );
}
