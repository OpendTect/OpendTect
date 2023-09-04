/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uifreqfilter.h"
#include "uigeninput.h"
#include "survinfo.h"


uiFreqFilterSelFreq::uiFreqFilterSelFreq( uiParent* p)
    : uiGroup( p, "Frequency Filter Selection")
    , parchanged(this)
{
    const bool zistime = SI().zDomain().isTime();
    const char** typestrs = FFTFilter::TypeNames();
    typefld_ = new uiGenInput( this, tr("Filter type"),
				StringListInpSpec(typestrs));
    typefld_->valuechanged.notify( mCB(this,uiFreqFilterSelFreq,getFromScreen));
    typefld_->valuechanged.notify( mCB(this,uiFreqFilterSelFreq,typeSel) );
    typefld_->valuechanged.notify( mCB(this,uiFreqFilterSelFreq,parChgCB) );

    freqfld_ = new uiGenInput( this, sMinMax(),
	    FloatInpSpec().setName(zistime?"Min frequency":"Min wavenumber"),
	    FloatInpSpec().setName(zistime?"Max frequency":"Max wavenumber") );
    freqfld_->setElemSzPol( uiObject::Small );
    freqfld_->attach( alignedBelow, typefld_ );
    freqfld_->valuechanged.notify(mCB(this,uiFreqFilterSelFreq,getFromScreen));
    freqfld_->valuechanged.notify( mCB(this,uiFreqFilterSelFreq,parChgCB) );

    setHAlignObj( freqfld_ );

    const float nyq = 0.5f/SI().zStep() * (zistime ? 1.0f : 1000.0f);
    set( nyq*0.1, nyq*0.4, FFTFilter::LowPass );

    postFinalize().notify( mCB(this,uiFreqFilterSelFreq,typeSel) );
}


uiFreqFilterSelFreq::~uiFreqFilterSelFreq()
{}


void uiFreqFilterSelFreq::parChgCB( CallBacker* )
{
    parchanged.trigger();
}


const uiString uiFreqFilterSelFreq::sMinMax()
{
    const FFTFilter::Type ftype = filterType();
    const bool zistime = SI().zDomain().isTime();
    const bool zismeter = SI().zDomain().isDepth() && !SI().depthsInFeet();
    const BufferString prestr = ftype==FFTFilter::LowPass ?	"Max" :
				ftype==FFTFilter::HighPass ?	"Min" :
								"Min/max";
    return zistime ?
		tr("%1 %2(Hz)").arg(prestr).arg(uiStrings::sFrequency(true)) :
	   zismeter ?
		tr("%1 %2(/km)").arg(prestr).arg(uiStrings::sWaveNumber(true)) :
		tr("%1 %2(/kft)").arg(prestr).arg(uiStrings::sWaveNumber(true));
}


void uiFreqFilterSelFreq::typeSel( CallBacker* )
{
    const int type = typefld_->getIntValue();
    const bool hasmin = type==1 || type==2;
    const bool hasmax = !type || type==2;
    freqfld_->displayField( hasmin, 0, 0 );
    freqfld_->displayField( hasmax, 0, 1 );
    freqfld_->setTitleText( sMinMax() );
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


void uiFreqFilterSelFreq::set( float minf, float maxf, FFTFilter::Type tp )
{
    freqrg_.start = minf;
    freqrg_.stop = maxf;
    filtertype_ = tp;
    putToScreen();
}
