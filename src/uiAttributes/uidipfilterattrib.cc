/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uidipfilterattrib.cc,v 1.7 2006-03-10 13:34:02 cvsbert Exp $
________________________________________________________________________

-*/

#include "ui3dfilterattrib.h"
#include "dipfilterattrib.h"
#include "convolveattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uispinbox.h"

using namespace Attrib;

const int cMinVal = 3;
const int cMaxVal_conv = 29;
const int cMaxVal_dipf = 49;
const int cStepVal = 2;

static const char* kerstrs[] =
{
        "LowPass (smoothing filter)",
        "Laplacian (edge enhancing filter)",
        "Prewitt (gradient filter)",
	"Velocity fan filter",
        0
};


static const char* fltrstrs[] =
{
	"Low",
	"High",
	"Cone",
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


ui3DFilterAttrib::ui3DFilterAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    if ( !zIsTime() )
    {
	kerstrs[3] = "Dip filter";
	outpstrs[3] = "Depth gradient";
    }

    inpfld = getInpFld();

    kernelfld = new uiGenInput( this, "Filter type",
                                StringListInpSpec( kerstrs ) );
    kernelfld->attach( alignedBelow, inpfld );
    kernelfld->valuechanged.notify( mCB(this,ui3DFilterAttrib,kernelSel) );

    szfld = new uiLabeledSpinBox( this, "Filter size" );
    szfld->box()->setMinValue( cMinVal );
    szfld->box()->setStep( cStepVal, true );
    szfld->attach( alignedBelow, kernelfld );

    BufferString fltrlbl;
    fltrlbl = zIsTime() ? "Velocity " : "Dip ";
    fltrlbl += "to pass";
    fltrtpfld = new uiGenInput( this, fltrlbl, StringListInpSpec(fltrstrs) );
    fltrtpfld->valuechanged.notify( mCB(this,ui3DFilterAttrib,filtSel) );
    fltrtpfld->attach( alignedBelow, szfld );

    FloatInpSpec fis;
    BufferString lbl( "Min/max " );
    lbl += zIsTime() ? "velocity (m/s)" : "dip (deg)";
    velfld = new uiGenInput( this, lbl, fis, fis );
    velfld->setElemSzPol( uiObject::Small );
    velfld->attach( alignedBelow, fltrtpfld );

    azifld = new uiGenInput( this, "Azimuth filter", BoolInpSpec() );
    azifld->setValue( false );
    azifld->attach( alignedBelow, velfld );
    azifld->valuechanged.notify( mCB(this,ui3DFilterAttrib,aziSel) );

    aziintfld = new uiGenInput( this, "Azimuth to pass (min/max)",
				FloatInpIntervalSpec());
    aziintfld->attach( alignedBelow, azifld );

    taperfld = new uiGenInput( this, "Taper length (%)", FloatInpSpec() );
    taperfld->attach( alignedBelow, aziintfld );

    shapefld = new uiGenInput( this, "Shape", BoolInpSpec( "Sphere", "Cube" ) );
    shapefld->attach( alignedBelow, szfld );

    outpfld = new uiGenInput( this, "Output", StringListInpSpec(outpstrs) );
    outpfld->attach( alignedBelow, kernelfld );

    kernelSel(0);
    setHAlignObj( inpfld );
}


void ui3DFilterAttrib::set2D( bool yn )
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


void ui3DFilterAttrib::filtSel( CallBacker* )
{
    int val = fltrtpfld->getIntValue();
    bool mode0 = ( val==1 || val==2 );
    bool mode1 = ( !val || val==2 );
    velfld->setSensitive( mode0, 0, 0 );
    velfld->setSensitive( mode1, 0, 1 );
    if ( !mode0 ) velfld->setText( "", 0 );
    if ( !mode1 ) velfld->setText( "", 1 );
}


void ui3DFilterAttrib::aziSel( CallBacker* )
{
    aziintfld->display( azifld->getBoolValue() );
}


void ui3DFilterAttrib::kernelSel( CallBacker* )
{
    int kernelval = kernelfld->getIntValue();
    bool dipf = kernelval == 3;

    szfld->display( kernelval != 2 );
    szfld->box()->setMaxValue( dipf ? cMaxVal_dipf : cMaxVal_conv );
    fltrtpfld->display( dipf );
    velfld->display( dipf );
    azifld->display( dipf );
    aziintfld->display( dipf );
    taperfld->display( dipf );
    shapefld->display( kernelval < 2 );
    outpfld->display( kernelval == 2 );

    if ( dipf ) { aziSel(0); filtSel(0); } 
}


const char* ui3DFilterAttrib::getAttribName() const
{
    return kernelfld->getIntValue() == 3 ? DipFilter::attribName() 
					 : Convolve::attribName();
}


bool ui3DFilterAttrib::setParameters( const Desc& desc )
{
    if ( !strcmp(desc.attribName(),Convolve::attribName()) )
    {
	mIfGetEnum( Convolve::kernelStr(), kernel, kernelfld->setValue(kernel) )
	mIfGetEnum( Convolve::shapeStr(), shape, shapefld->setValue(shape) )
	mIfGetInt( Convolve::sizeStr(), size, szfld->box()->setValue(size) )
    }
    else if ( !strcmp(desc.attribName(),DipFilter::attribName()) )
    {
	mIfGetInt( DipFilter::sizeStr(), size, szfld->box()->setValue(size) )
	mIfGetEnum( DipFilter::typeStr(), type, fltrtpfld->setValue(type) )
	mIfGetFloat( DipFilter::minvelStr(), minvel,
		     velfld->setValue(minvel,0) )
	mIfGetFloat( DipFilter::maxvelStr(), maxvel,
		     velfld->setValue(maxvel,1) )
	mIfGetBool( DipFilter::filteraziStr(), filterazi,
		    azifld->setValue(filterazi) )
	mIfGetFloat( DipFilter::minaziStr(), minazi,
		     aziintfld->setValue(minazi,0) )
	mIfGetFloat( DipFilter::maxaziStr(), maxazi,
		     aziintfld->setValue(maxazi,1) )
	mIfGetFloat( DipFilter::taperlenStr(), taperlen,
		     taperfld->setValue(taperlen) )
	kernelfld->setValue( 3 );
    }
    else
	return false;

    kernelSel(0);
    return true;
}


bool ui3DFilterAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool ui3DFilterAttrib::setOutput( const Desc& desc )
{
    if ( kernelfld->getIntValue() == 2 )
    {
	const int selout = desc.selectedOutput();
	outpfld->setValue( inpfld->is2D() ? selout-2 : selout );
    }

    return true;
}


bool ui3DFilterAttrib::getParameters( Desc& desc )
{
    if ( !strcmp(desc.attribName(),Convolve::attribName()) )
    {
	mSetEnum( Convolve::kernelStr(), kernelfld->getIntValue() );
	mSetEnum( Convolve::shapeStr(), shapefld->getIntValue() );
	mSetInt( Convolve::sizeStr(), szfld->box()->getValue() );
    }
    else if ( !strcmp(desc.attribName(),DipFilter::attribName()) )
    {
	mSetInt( DipFilter::sizeStr(), szfld->box()->getValue() );
	mSetEnum( DipFilter::typeStr(), fltrtpfld->getIntValue() );
	mSetFloat( DipFilter::minvelStr(), velfld->getfValue(0) );
	mSetFloat( DipFilter::maxvelStr(), velfld->getfValue(1) );
	mSetBool( DipFilter::filteraziStr(), azifld->getBoolValue() );
	mSetFloat( DipFilter::minaziStr(), aziintfld->getfValue(0) );
	mSetFloat( DipFilter::maxaziStr(), aziintfld->getfValue(1) );
	mSetFloat( DipFilter::taperlenStr(), taperfld->getfValue() );
    }
    else
	return false;

    return true;
}


bool ui3DFilterAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}


bool ui3DFilterAttrib::getOutput( Desc& desc )
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


void ui3DFilterAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    if ( kernelfld->getIntValue() != 2 )
	params += EvalParam( filterszstr, DipFilter::sizeStr() );
}
