/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
________________________________________________________________________

-*/

static const char* rcsID = "$Id$";


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
#include "uifreqtaper.h"

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
    inpfld = createImagInpFld( is2d );

    isfftfld = new uiGenInput( this, "Filtering method", 
			       BoolInpSpec(true,"FFT","ButterWorth") );
    isfftfld->attach( alignedBelow, inpfld );
    isfftfld->valuechanged.notify( mCB(this,uiFreqFilterAttrib,isfftSel) );
		    
    typefld = new uiGenInput( this, "Filter type", 
	    		      StringListInpSpec(typestrs) );
    typefld->attach( alignedBelow, isfftfld );
    typefld->valuechanged.notify( mCB(this,uiFreqFilterAttrib,typeSel) );

    const char* minmaxtxt = "Min/max frequency(Hz)";
    freqfld = new uiGenInput( this, minmaxtxt, 
	    FloatInpSpec().setName("Min frequency"),
	    FloatInpSpec().setName("Max frequency") );
    freqfld->setElemSzPol( uiObject::Small );
    freqfld->attach( alignedBelow, typefld );
    freqfld->valuechanged.notify( mCB(this,uiFreqFilterAttrib,freqChanged) );
    freqfld->valuechanged.notify( mCB(this,uiFreqFilterAttrib,updateTaperFreqs) );

    polesfld = new uiLabeledSpinBox( this, "Nr of poles" );
    polesfld->box()->setMinValue( 2 );
    polesfld->attach( alignedBelow, freqfld );

    uiWindowFunctionSel::Setup su; 
    su.label_ = "Window/Taper"; 
    su.winname_ = 0;

    for ( int idx=0; idx<2; idx++ )
    {
	if ( idx )
	{
	    su.label_ = "Taper"; 
	    su.onlytaper_ = true;
	    su.with2fldsinput_ = true;
	    su.inpfldtxt_ = minmaxtxt;

	    FreqTaperSetup freqsu;
	    freqsu.seisnm_ = inpfld->getInput(); 
	    freqsu.attrnm_ = inpfld->getAttrName(); 
	    winflds += new uiFreqTaperSel( this, su, freqsu );
	}
	else
	    winflds += new uiWindowFunctionSel( this, su );
	winflds[idx]->display (false);
    }
    BufferString seltxt ( "Specify " ); seltxt += "Frequency Taper";
    freqwinselfld = new uiCheckBox( this, seltxt );
    freqwinselfld->attach( alignedBelow, winflds[0] );
    freqwinselfld->activated.notify( mCB(this,uiFreqFilterAttrib,freqWinSel) );

    winflds[0]->attach( alignedBelow, polesfld );
    winflds[1]->attach( alignedBelow, freqwinselfld );
    winflds[1]->setSensitive( false );
    
    postFinalise().notify( mCB(this,uiFreqFilterAttrib,finaliseCB));
    setHAlignObj( inpfld );
}


void uiFreqFilterAttrib::finaliseCB( CallBacker* )
{
    typeSel(0);
    isfftSel(0);
    freqChanged(0);
    freqWinSel(0);
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
    {	
	Interval<float> frg( freqfld->getFInterval() );
	tap->setRefFreqs( frg ); 
    }
}


void uiFreqFilterAttrib::updateTaperFreqs( CallBacker* )
{
    mDynamicCastGet( uiFreqTaperSel*, tap, winflds[1] );
    if ( tap )
    {
	bool costaper = !strcmp(tap->windowName(),"CosTaper");
	Interval<float> frg( freqfld->getFInterval() );
	if ( costaper ) { frg.start-=5; frg.stop+=5; }
	tap->setInputFreqValue( frg.start > 0 ? frg.start : 0, 0 );
	tap->setInputFreqValue( frg.stop , 1 );
    }
}


void uiFreqFilterAttrib::freqWinSel( CallBacker* )
{
    const bool isfreqtaper = freqwinselfld->isChecked();
    winflds[1]->setSensitive( isfreqtaper );
    if ( !isfreqtaper )
	updateTaperFreqs(0);
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
    mIfGetString( FreqFilter::fwindowStr(), fwindow,
			    winflds[1]->setWindowName(fwindow) );
    mIfGetFloat( FreqFilter::paramvalStr(), variable,
	    const float resvar = float( mNINT32((1-variable)*1000) )/1000.0;
	    winflds[0]->setWindowParamValue(resvar) );
    mDynamicCastGet( uiFreqTaperSel*, taper, winflds[1] );
    if ( taper ) 
    {
	mIfGetFloat( FreqFilter::highfreqparamvalStr(), highfreqvalue,
	    const float res = float( highfreqvalue) ;
	    taper->setInputFreqValue( res, 0 ) ); 
	mIfGetFloat( FreqFilter::lowfreqparamvalStr(), lowfreqvalue,
	    const float res = float( lowfreqvalue );
	    taper->setInputFreqValue( res, 1 ) ); 
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
    mSetString( FreqFilter::fwindowStr(), winflds[1]->windowName() );

    const float resvar =
		float( mNINT32((1-winflds[0]->windowParamValue())*1000) )/1000.0;
    mSetFloat( FreqFilter::paramvalStr(), resvar );
    mDynamicCastGet( uiFreqTaperSel*, taper, winflds[1] );
    if ( taper ) 
    {
	Interval<float> freqresvar = taper->freqValues();
	const bool istaper = !strcmp(winflds[1]->windowName(),"CosTaper");
	freqresvar.start = istaper ? (freqresvar.start) 
				   : freqfld->getfValue(0);
	freqresvar.stop = istaper ? (freqresvar.stop) 
				  : freqfld->getfValue(1);
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

#define mErrWinFreqMsg(msg)\
    errmsg_=msg;\
    errmsg_+=endmsg;\
    updateTaperFreqs(0);\
    return false;
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
    mDynamicCastGet( uiFreqTaperSel*, taper, winflds[1] );
    if ( taper ) 
    {
	bool minsuccess = true, maxsuccess = true;
	Interval<float> freqresvar = taper->freqValues();
	BufferString endmsg, msg;
	if ( freqresvar.start < 0 )
	{  msg += "min frequency cannot be negative"; mErrWinFreqMsg(msg) }
	endmsg += " than this of the filter frequency.\n";
	endmsg += "Please select a different frequency.";
	if ( freqresvar.start > freqfld->getfValue(0) )
	{  msg += "Taper min frequency must be lower"; mErrWinFreqMsg(msg) }
	if ( freqresvar.stop < freqfld->getfValue(1) )
	{  msg += "Taper max frequency must be higher"; mErrWinFreqMsg(msg) }
    }
    return true;
}

