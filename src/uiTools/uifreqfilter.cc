/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uifreqfilter.cc,v 1.1 2012-04-26 14:32:10 cvsbruno Exp $";


#include "uifreqfilter.h"
#include "uigeninput.h"

static const char* minmaxtxt = "Min/max frequency(Hz)";

uiFreqFilterSelFreq::uiFreqFilterSelFreq( uiParent* p)
    : uiGroup( this, "Frequency Filter Selection")
{
    static const char** typestrs = FFTFilter::TypeNames();
    typefld_ = new uiGenInput( this, "Filter type", 
	    		      StringListInpSpec(typestrs) );
    typefld_->valuechanged.notify( mCB(this,uiFreqFilterSelFreq,typeSel) );

    freqfld_ = new uiGenInput( this, minmaxtxt, 
	    FloatInpSpec().setName("Min frequency"),
	    FloatInpSpec().setName("Max frequency") );
    freqfld_->setElemSzPol( uiObject::Small );
    freqfld_->attach( alignedBelow, typefld_ );
}


uiFreqFilterSelFreq::~uiFreqFilterSelFreq()
{
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
    typefld_->setValue( params_.filtertype_ );
    freqfld_->setValue( params_.minfreq_, 0 );
    freqfld_->setValue( params_.maxfreq_, 1 );
}


void uiFreqFilterSelFreq::getFromScreen( CallBacker* )
{
    params_.filtertype_ = (FFTFilter::Type)typefld_->getIntValue();
    params_.minfreq_ = freqfld_->getfValue(0);
    params_.maxfreq_ = freqfld_->getfValue(1);
}



