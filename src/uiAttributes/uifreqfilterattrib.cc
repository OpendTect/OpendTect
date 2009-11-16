/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uifreqfilterattrib.cc,v 1.26 2009-11-16 17:08:49 cvsbruno Exp $";


#include "uifreqfilterattrib.h"
#include "freqfilterattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uiwindowfunctionsel.h"

using namespace Attrib;


static const char* typestrs[] =
{
    "LowPass",
    "HighPass",
    "BandPass",
    0
};


mInitAttribUI(uiFreqFilterAttrib,FreqFilter,"Frequency Filter",sKeyFilterGrp())


uiFreqFilterAttrib::uiFreqFilterAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.7")

{
    inpfld = getImagInpFld( is2d );

    isfftfld = new uiGenInput( this, "Filtering method", 
			       BoolInpSpec(true,"FFT","ButterWorth") );
    isfftfld->attach( alignedBelow, inpfld );
    isfftfld->valuechanged.notify( mCB(this,uiFreqFilterAttrib,isfftSel) );
		    
    typefld = new uiGenInput( this, "Filter type", 
	    		      StringListInpSpec(typestrs) );
    typefld->attach( alignedBelow, isfftfld );
    typefld->valuechanged.notify( mCB(this,uiFreqFilterAttrib,typeSel) );

    freqfld = new uiGenInput( this, "Min/max frequency(Hz)", 
	    FloatInpSpec().setName("Min frequency"),
	    FloatInpSpec().setName("Max frequency") );
    freqfld->setElemSzPol( uiObject::Small );
    freqfld->attach( alignedBelow, typefld );
    freqfld->valuechanged.notify( mCB(this,uiFreqFilterAttrib,freqChanged) );

    polesfld = new uiLabeledSpinBox( this, "Nr of poles" );
    polesfld->box()->setMinValue( 2 );
    polesfld->attach( alignedBelow, freqfld );

    BufferString wintext( "Window/Taper" );
    uiWindowFunctionSel::Setup su; su.label_ = wintext; su.winname_ = 0;
    for ( int idx=0; idx<2; idx++ )
    {
	if ( idx )
	{
	    su.ismaxfreq_ = true; 
	    su.label_ = "Taper"; su.onlytaper_ = true;
	    su.inpfldtxt_ = " Min/max frequency ";
	    winflds += new uiFreqTaperSel( this, su );
	}
	else
	    winflds += new uiWindowFunctionSel( this, su );
	winflds[idx]->display (false);
    }
    BufferString seltxt ( "Specify " ); seltxt += "Frequency Taper";
    freqwinselfld = new uiCheckBox( this, seltxt );
    freqwinselfld->attach( alignedBelow, winflds[0] );
    freqwinselfld->display (false);
    freqwinselfld->activated.notify( mCB(this,uiFreqFilterAttrib,freqWinSel) );

    winflds[0]->attach( alignedBelow, polesfld );
    winflds[1]->attach( alignedBelow, freqwinselfld );
    winflds[1]->setSensitive( false );
    
    mainObject()->finaliseDone.notify( mCB(this,uiFreqFilterAttrib,finaliseCB));
    setHAlignObj( inpfld );
}


void uiFreqFilterAttrib::finaliseCB( CallBacker* )
{
    typeSel(0);
    isfftSel(0);
    freqChanged(0);
}


void uiFreqFilterAttrib::typeSel( CallBacker* )
{
    const int type = typefld->getIntValue();
    const bool hasmin = type==1 || type==2;
    const bool hasmax = !type || type==2;
    freqfld->setSensitive( hasmin, 0, 0 );
    freqfld->setSensitive( hasmax, 0, 1 );
    mDynamicCastGet( uiFreqTaperSel*, tap, winflds[1] );
    if ( tap ) tap->setIsMinMaxFreq( hasmin, hasmax );
}


void uiFreqFilterAttrib::isfftSel( CallBacker* )
{
    const bool isfft = isfftfld->getBoolValue();
    for ( int idx=0; idx<winflds.size(); idx++)
	winflds[idx]->display( isfft );
    freqwinselfld->display( isfft );
    polesfld->display( !isfft );
}


void uiFreqFilterAttrib::freqChanged( CallBacker* )
{
    mDynamicCastGet( uiFreqTaperSel*, tap, winflds[1] );
    if ( tap ) 
	tap->setFreqSel( freqfld->getFInterval() ); 
}


void uiFreqFilterAttrib::freqWinSel( CallBacker* )
{
    const bool isfreqtaper = freqwinselfld->isChecked();
    winflds[1]->setSensitive( isfreqtaper );
    mDynamicCastGet( uiFreqTaperSel*, tap, winflds[1] );
    if ( !isfreqtaper && tap ) 
    {
	tap->setFreqValue( freqfld->getfValue(0), 0 );
	tap->setFreqValue( freqfld->getfValue(1), 1 ); 
    }
}


bool uiFreqFilterAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),FreqFilter::attribName()) )
	return false;

    mIfGetEnum( FreqFilter::filtertypeStr(), filtertype, 
	    	typefld->setValue(filtertype) );
    mIfGetFloat( FreqFilter::minfreqStr(), minfreq,
	    	 freqfld->setValue(minfreq,0) );
    mIfGetFloat( FreqFilter::maxfreqStr(), maxfreq,
	    	 freqfld->setValue(maxfreq,1) );
    mIfGetInt( FreqFilter::nrpolesStr(), nrpoles,
	       polesfld->box()->setValue(nrpoles) )

    mIfGetString( FreqFilter::windowStr(), window,
			    winflds[0]->setWindowName(window) );
    mIfGetFloat( FreqFilter::paramvalStr(), variable,
	    const float resvar = float( mNINT((1-variable)*1000) )/1000.0;
	    winflds[0]->setWindowParamValue(resvar) );
    mDynamicCastGet( uiFreqTaperSel*, taper, winflds[1] );
    if ( taper ) 
    {
	taper->setWindowName( "CosTaper" );
	mIfGetFloat( FreqFilter::highfreqparamvalStr(), highfreqvalue,
	    const float res = float( highfreqvalue) ;
	    taper->setFreqValue( res, 0 ) ); 
	mIfGetFloat( FreqFilter::lowfreqparamvalStr(), lowfreqvalue,
	    const float res = float( lowfreqvalue );
	    taper->setFreqValue( res, 1 ) ); 
    }
    mIfGetBool( FreqFilter::isfftfilterStr(), isfftfilter, 
	    	isfftfld->setValue(isfftfilter) );
    mIfGetBool( FreqFilter::isfreqtaperStr(), isfreqtaper, 
	    	freqwinselfld->setChecked(isfreqtaper) );

    finaliseCB(0);
    return true;
}


bool uiFreqFilterAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiFreqFilterAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),FreqFilter::attribName()) )
	return false;

    mSetEnum( FreqFilter::filtertypeStr(), typefld->getIntValue() );
    mSetFloat( FreqFilter::minfreqStr(), freqfld->getfValue(0) );
    mSetFloat( FreqFilter::maxfreqStr(), freqfld->getfValue(1) );
    mSetInt( FreqFilter::nrpolesStr(), polesfld->box()->getValue() );
    mSetString( FreqFilter::windowStr(), winflds[0]->windowName() );

    const float resvar =
		float( mNINT((1-winflds[0]->windowParamValue())*1000) )/1000.0;
    mSetFloat( FreqFilter::paramvalStr(), resvar );
    mDynamicCastGet( uiFreqTaperSel*, taper, winflds[1] );
    if ( taper ) 
    {
	Interval<float> freqresvar = taper->freqValues();
	freqresvar.start = mNINT((freqresvar.start));
	freqresvar.stop = mNINT((freqresvar.stop));
	mSetFloat( FreqFilter::lowfreqparamvalStr(), freqresvar.stop );
	mSetFloat( FreqFilter::highfreqparamvalStr(), freqresvar.start );
    }
    mSetBool( FreqFilter::isfftfilterStr(), isfftfld->getBoolValue() );
    mSetBool( FreqFilter::isfreqtaperStr(), freqwinselfld->isChecked() );

    return true;
}


bool uiFreqFilterAttrib::getInput( Desc& desc )
{
    inpfld->processInput();
    const bool needimag = isfftfld->getBoolValue();
    fillInp( needimag ? inpfld : (uiAttrSel*)inpfld, desc, 0 );
    return true;
}


void uiFreqFilterAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    const int passtype = typefld->getIntValue();
    if ( passtype != 0 )
	params += EvalParam( "Min frequency", FreqFilter::minfreqStr() );
    if ( passtype != 1 )
	params += EvalParam( "Max frequency", FreqFilter::maxfreqStr() );
    if ( !isfftfld->getBoolValue() )
	params += EvalParam( "Nr poles", FreqFilter::nrpolesStr() );
}


bool uiFreqFilterAttrib::areUIParsOK()
{
    if ( !strcmp( winflds[0]->windowName(), "CosTaper" ) )
    {
	float paramval = winflds[0]->windowParamValue();
	if ( paramval<0 || paramval>1  )
	{
	    errmsg_ = "Variable 'Taper length' is not\n";
	    errmsg_ += "within the allowed range: 0 to 100 (%).";
	    return false;
	}
    }

    return true;
}

