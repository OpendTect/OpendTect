/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uidipfilterattrib.h"
#include "dipfilterattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "attribfactory.h"
#include "survinfo.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uispinbox.h"

using namespace Attrib;

const int cMinVal = 3;
const int cMaxVal = 49;
const int cStepVal = 2;

static const char* fltrstrs[] =
{
	"Low",
	"High",
	"Cone",
	0
};

mInitAttribUI(uiDipFilterAttrib,DipFilter,"Velocity Fan Filter",sKeyFilterGrp())


uiDipFilterAttrib::uiDipFilterAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.2")
	  
{
    inpfld = createInpFld( is2d );

    szfld = new uiLabeledSpinBox( this, "Filter size" );
    szfld->box()->setMinValue( cMinVal );
    szfld->box()->setStep( cStepVal, true );
    szfld->attach( alignedBelow, inpfld );

    BufferString fltrlbl;
    fltrlbl = zIsTime() ? "Velocity " : "Dip ";
    fltrlbl += "to pass";
    fltrtpfld = new uiGenInput( this, fltrlbl, StringListInpSpec(fltrstrs) );
    fltrtpfld->valuechanged.notify( mCB(this,uiDipFilterAttrib,filtSel) );
    fltrtpfld->attach( alignedBelow, szfld );

    BufferString lbl( "Min/max " );
    lbl += zIsTime() ? "velocity (m/s)" : "dip (deg)";
    const char* fldnm = zIsTime() ? " velocity" : " dip";
    velfld = new uiGenInput( this, lbl,
		FloatInpSpec().setName( BufferString("Min",fldnm).buf() ),
		FloatInpSpec().setName( BufferString("Max",fldnm).buf() ) );
    velfld->setElemSzPol( uiObject::Small );
    velfld->attach( alignedBelow, fltrtpfld );

    azifld = new uiGenInput( this, "Azimuth filter", BoolInpSpec(true) );
    azifld->setValue( false );
    azifld->attach( alignedBelow, velfld );
    azifld->valuechanged.notify( mCB(this,uiDipFilterAttrib,aziSel) );

    aziintfld = new uiGenInput( this, "Azimuth to pass (min/max)",
				FloatInpIntervalSpec().setName("Min Azimuth",0)
				.setName("Max Azimuth",1) );
    aziintfld->attach( alignedBelow, azifld );

    taperfld = new uiGenInput( this, "Taper length (%)", 
			       FloatInpSpec().setName("Taper length") );
    taperfld->attach( alignedBelow, aziintfld );

    setHAlignObj( inpfld );
    filtSel(0);
    aziSel(0);
}


void uiDipFilterAttrib::filtSel( CallBacker* )
{
    int val = fltrtpfld->getIntValue();
    bool mode0 = ( val==1 || val==2 );
    bool mode1 = ( !val || val==2 );
    velfld->setSensitive( mode0, 0, 0 );
    velfld->setSensitive( mode1, 0, 1 );
}


void uiDipFilterAttrib::aziSel( CallBacker* )
{
    aziintfld->display( azifld->getBoolValue() );
}


bool uiDipFilterAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),DipFilter::attribName()) )
	return false;

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
    filtSel(0);
    aziSel(0);
    return true;
}


bool uiDipFilterAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiDipFilterAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),DipFilter::attribName()) )
	return false;

    mSetInt( DipFilter::sizeStr(), szfld->box()->getValue() );
    mSetEnum( DipFilter::typeStr(), fltrtpfld->getIntValue() );
    mSetFloat( DipFilter::minvelStr(), velfld->getfValue(0) );
    mSetFloat( DipFilter::maxvelStr(), velfld->getfValue(1) );
    mSetBool( DipFilter::filteraziStr(), azifld->getBoolValue() );
    mSetFloat( DipFilter::minaziStr(), aziintfld->getfValue(0) );
    mSetFloat( DipFilter::maxaziStr(), aziintfld->getfValue(1) );
    mSetFloat( DipFilter::taperlenStr(), taperfld->getfValue() );

    return true;
}


bool uiDipFilterAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}


void uiDipFilterAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( filterszstr(), DipFilter::sizeStr() );

    int val = fltrtpfld->getIntValue();
    bool mode0 = ( val==1 || val==2 );
    bool mode1 = ( !val || val==2 );
    if ( mode0 )
	params += EvalParam( SI().zIsTime() ? "Minimum velocity" :"Minimum dip",
			     DipFilter::minvelStr() );
    if ( mode1 )
	params += EvalParam( SI().zIsTime() ? "Maximum velocity" :"Maximum dip",
			     DipFilter::maxvelStr() );
}
