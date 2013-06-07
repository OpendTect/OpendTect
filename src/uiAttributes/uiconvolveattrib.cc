/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          November 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiconvolveattrib.h"
#include "convolveattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "attribfactory.h"
#include "ioman.h"
#include "survinfo.h"
#include "wavelet.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uispinbox.h"

using namespace Attrib;

const int cMinVal = 3;
const int cMaxVal = 29;
const int cStepVal = 2;

static const char* kerstrs[] =
{
        "LowPass (smoothing filter)",
        "Laplacian (edge enhancing filter)",
        "Prewitt (gradient filter)",
	"Wavelet",
        0
};


static const char* outpstrs3d[] =
{
	"Average gradient",
        "Inline gradient",
        "Crossline gradient",
        "Time gradient",
        0
};


static const char* outpstrs2d[] =
{
    	"Average gradient",
	"Line gradient",
        "Time gradient",
        0
};


mInitAttribUI(uiConvolveAttrib,Convolve,"Convolve",sKeyFilterGrp())


uiConvolveAttrib::uiConvolveAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.1")
{
    inpfld_ = createInpFld( is2d );

    kernelfld_ = new uiGenInput( this, "Filter type",
                                StringListInpSpec( kerstrs ) );
    kernelfld_->attach( alignedBelow, inpfld_ );
    kernelfld_->valuechanged.notify( mCB(this,uiConvolveAttrib,kernelSel) );

    szfld_ = new uiLabeledSpinBox( this, "Filter size" );
    szfld_->box()->setMinValue( cMinVal );
    szfld_->box()->setStep( cStepVal, true );
    szfld_->attach( alignedBelow, kernelfld_ );

    const char* spherestr = is2d ? "Circle" : "Sphere";
    const char* cubestr = is2d ? "Square" : "Cube";
    shapefld_ = new uiGenInput( this, "Shape",
	    			BoolInpSpec(true, spherestr, cubestr) );
    shapefld_->attach( alignedBelow, szfld_ );

    outpfld_ = new uiGenInput( this, "Output",
		       StringListInpSpec( is2d_ ? outpstrs2d : outpstrs3d) );
    outpfld_->attach( alignedBelow, kernelfld_ );

    waveletfld_ = new uiIOObjSel( this, mIOObjContext(Wavelet) );
    waveletfld_->attach( alignedBelow, kernelfld_ );
    waveletfld_->display(false);

    kernelSel(0);
    setHAlignObj( inpfld_ );
}


uiConvolveAttrib::~uiConvolveAttrib()
{
}


void uiConvolveAttrib::kernelSel( CallBacker* cb )
{
    int kernelval = kernelfld_->getIntValue();

    szfld_->display( kernelval < 2 );
    szfld_->box()->setMaxValue( cMaxVal );
    shapefld_->display( kernelval < 2 );
    outpfld_->display( kernelval == 2 );
    waveletfld_->display( kernelval == 3 );
}


static void setFldInp( uiIOObjSel* fld, const char* str )
{
    IOObj* ioobj = IOM().get( MultiID(str) );
    if ( !ioobj ) return;
    fld->ctxtIOObj( true ).setObj( ioobj );
    fld->updateInput();
}


bool uiConvolveAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Convolve::attribName()) )
	return false;

    mIfGetEnum( Convolve::kernelStr(), kernel, kernelfld_->setValue(kernel) )
    mIfGetEnum( Convolve::shapeStr(), shape, shapefld_->setValue(shape) )
    mIfGetInt( Convolve::sizeStr(), size, szfld_->box()->setValue(size) )
    mIfGetString( Convolve::waveletStr(), wavidstr,
	    	  setFldInp(waveletfld_,wavidstr) )

    kernelSel(0);
    return true;
}


bool uiConvolveAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiConvolveAttrib::setOutput( const Desc& desc )
{
    if ( kernelfld_->getIntValue() == 2 )
    {
	const int selout = desc.selectedOutput();
	outpfld_->setValue( selout );
    }

    return true;
}


bool uiConvolveAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Convolve::attribName()) )
	return false;

    const int typeval = kernelfld_->getIntValue();
    mSetEnum( Convolve::kernelStr(), typeval );
    if ( typeval < 2 )
    {
	mSetEnum( Convolve::shapeStr(), shapefld_->getIntValue() );
	mSetInt( Convolve::sizeStr(), szfld_->box()->getValue() );
    }
    else if ( typeval == 3 )
	if ( waveletfld_->ioobj(true) )
	    mSetString( Convolve::waveletStr(), waveletfld_->key().buf() );

    return true;
}


bool uiConvolveAttrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    return true;
}


bool uiConvolveAttrib::getOutput( Desc& desc )
{
    const int selout = kernelfld_->getIntValue()==2 ? outpfld_->getIntValue()
						    : 0;
    fillOutput( desc, selout );
    return true;
}


void uiConvolveAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    if ( kernelfld_->getIntValue() < 2 )
	params += EvalParam( filterszstr(), Convolve::sizeStr() );
}
