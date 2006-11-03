/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          November 2006
 RCS:           $Id: uiconvolveattrib.cc,v 1.1 2006-11-03 16:01:36 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiconvolveattrib.h"
#include "convolveattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "attribfactory.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
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
        0
};


static const char* outpstrs[] =
{
	"Average gradient",
        "Inline gradient",
        "Crossline gradient",
        "Time gradient",
        0
};


mInitAttribUI(uiConvolveAttrib,Convolve,"Convolve",sKeyFilterGrp)


uiConvolveAttrib::uiConvolveAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld = getInpFld();

    kernelfld = new uiGenInput( this, "Filter type",
                                StringListInpSpec( kerstrs ) );
    kernelfld->attach( alignedBelow, inpfld );
    kernelfld->valuechanged.notify( mCB(this,uiConvolveAttrib,kernelSel) );

    szfld = new uiLabeledSpinBox( this, "Filter size" );
    szfld->box()->setMinValue( cMinVal );
    szfld->box()->setStep( cStepVal, true );
    szfld->attach( alignedBelow, kernelfld );

    shapefld = new uiGenInput( this, "Shape", BoolInpSpec( "Sphere", "Cube" ) );
    shapefld->attach( alignedBelow, szfld );

    outpfld = new uiGenInput( this, "Output", StringListInpSpec(outpstrs) );
    outpfld->attach( alignedBelow, kernelfld );

    kernelSel(0);
    setHAlignObj( inpfld );
}


void uiConvolveAttrib::set2D( bool yn )
{
    inpfld->set2D( yn );

    const int oldval = outpfld->getIntValue();
    BufferStringSet strs;
    if ( yn )
    {
	strs.add( "Line gradient" );
	strs.add( outpstrs[3] );
    }
    else
	strs = outpstrs;

    outpfld->newSpec( StringListInpSpec(strs), 0 );
    outpfld->setValue( oldval );
}


void uiConvolveAttrib::kernelSel( CallBacker* cb )
{
    int kernelval = kernelfld->getIntValue();

    szfld->display( kernelval != 2 );
    szfld->box()->setMaxValue( cMaxVal );
    shapefld->display( kernelval < 2 );
    outpfld->display( kernelval == 2 );
}


bool uiConvolveAttrib::setParameters( const Desc& desc )
{
    if ( !strcmp(desc.attribName(),Convolve::attribName()) )
	return false;

    mIfGetEnum( Convolve::kernelStr(), kernel, kernelfld->setValue(kernel) )
    mIfGetEnum( Convolve::shapeStr(), shape, shapefld->setValue(shape) )
    mIfGetInt( Convolve::sizeStr(), size, szfld->box()->setValue(size) )

    kernelSel(0);
    return true;
}


bool uiConvolveAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiConvolveAttrib::setOutput( const Desc& desc )
{
    if ( kernelfld->getIntValue() == 2 )
    {
	const int selout = desc.selectedOutput();
	outpfld->setValue( inpfld->is2D() ? selout-2 : selout );
    }

    return true;
}


bool uiConvolveAttrib::getParameters( Desc& desc )
{
    if ( !strcmp(desc.attribName(),Convolve::attribName()) )
	return false;

    mSetEnum( Convolve::kernelStr(), kernelfld->getIntValue() );
    mSetEnum( Convolve::shapeStr(), shapefld->getIntValue() );
    mSetInt( Convolve::sizeStr(), szfld->box()->getValue() );

    return true;
}


bool uiConvolveAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}


bool uiConvolveAttrib::getOutput( Desc& desc )
{
    int selout = 0;
    if ( kernelfld->getIntValue() == 2 )
    {
	const int index = outpfld->getIntValue();
	selout = inpfld->is2D() ? index + 2 : index;
    }
    
    fillOutput( desc, selout );
    return true;
}


void uiConvolveAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    if ( kernelfld->getIntValue() != 2 )
	params += EvalParam( filterszstr, Convolve::sizeStr() );
}
