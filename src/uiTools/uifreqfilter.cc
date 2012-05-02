/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: uifreqfilter.cc,v 1.5 2012-05-02 15:12:21 cvskris Exp $";


#include "uifreqfilter.h"
#include "uigeninput.h"

static const char* minmaxtxt = "Min/max frequency(Hz)";

uiFreqFilterSelFreq::uiFreqFilterSelFreq( uiParent* p)
    : uiGroup( p, "Frequency Filter Selection")
    , parchanged(this)  
{
    static const char** typestrs = FFTFilter::TypeNames();
    typefld_ = new uiGenInput( this, "Filter type", 
	    		      StringListInpSpec(typestrs) );
    typefld_->valuechanged.notify( mCB(this,uiFreqFilterSelFreq,getFromScreen));
    typefld_->valuechanged.notify( mCB(this,uiFreqFilterSelFreq,typeSel) );
    typefld_->valuechanged.notify( mCB(this,uiFreqFilterSelFreq,parChgCB) );

    freqfld_ = new uiGenInput( this, minmaxtxt, 
	    FloatInpSpec().setName("Min frequency"),
	    FloatInpSpec().setName("Max frequency") );
    freqfld_->setElemSzPol( uiObject::Small );
    freqfld_->attach( alignedBelow, typefld_ );
    freqfld_->valuechanged.notify(mCB(this,uiFreqFilterSelFreq,getFromScreen));
    freqfld_->valuechanged.notify( mCB(this,uiFreqFilterSelFreq,parChgCB) );

    setMinFreq( 15 );
    setMaxFreq( 50 );
    setHAlignObj( freqfld_ );
}


void uiFreqFilterSelFreq::parChgCB( CallBacker* )
{
    parchanged.trigger();
}


void uiFreqFilterSelFreq::typeSel( CallBacker* )
{
    const int type = typefld_->getIntValue();
    const bool hasmin = type==1 || type==2;
    const bool hasmax = !type || type==2;
    freqfld_->setSensitive( hasmin, 0, 0 );
    freqfld_->setSensitive( hasmax, 0, 1 );
}


void uiFreqFilterSelFreq::putToScreen()
{
    typefld_->setValue( filtertype_ );
    freqfld_->setValue( freqrg_.start, 0 );
    freqfld_->setValue( freqrg_.stop, 1 );
    typeSel(0);
}



void uiFreqFilterSelFreq::getFromScreen( CallBacker* )
{
    filtertype_ = (FFTFilter::Type)typefld_->getIntValue();
    freqrg_ = freqfld_->getFInterval();
}



