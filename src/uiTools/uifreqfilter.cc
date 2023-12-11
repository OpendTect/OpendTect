/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uifreqfilter.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "survinfo.h"


uiFreqFilterSelFreq::uiFreqFilterSelFreq( uiParent* p)
    : uiGroup( p, "Frequency Filter Selection")
    , parchanged(this)
{
    const bool zistime = SI().zDomain().isTime();
    const char** typestrs = FFTFilter::TypeNames();
    typefld_ = new uiGenInput( this, tr("Filter type"),
				StringListInpSpec(typestrs));
    typefld_->valueChanged.notify( mCB(this,uiFreqFilterSelFreq,getFromScreen));
    typefld_->valueChanged.notify( mCB(this,uiFreqFilterSelFreq,typeSel) );
    typefld_->valueChanged.notify( mCB(this,uiFreqFilterSelFreq,parChgCB) );

    freqfld_ = new uiGenInput( this, sMinMax(),
	    FloatInpSpec().setName(zistime?"Min frequency":"Min wavenumber"),
	    FloatInpSpec().setName(zistime?"Max frequency":"Max wavenumber") );
    freqfld_->setElemSzPol( uiObject::Small );
    freqfld_->attach( alignedBelow, typefld_ );
    freqfld_->valueChanged.notify(mCB(this,uiFreqFilterSelFreq,getFromScreen));
    freqfld_->valueChanged.notify( mCB(this,uiFreqFilterSelFreq,parChgCB) );

    setHAlignObj( freqfld_ );

    const float nyq = 0.5f/SI().zStep() * (zistime ? 1.0f : 1000.0f);
    set( 0.0, nyq*0.4, FFTFilter::LowPass );

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


uiFreqFilter::uiFreqFilter( uiParent* p )
    : uiGroup( p, "Frequency Filter Selection")
    , valueChanged(this)
{
    const char** typestrs = FFTFilter::TypeNames();
    typefld_ = new uiGenInput( this, tr("Filter type"),
				StringListInpSpec(typestrs));

    const bool zistime = SI().zDomain().isTime();
    const bool zismeter = SI().zDomain().isDepth() && !SI().depthsInFeet();
    uiString txt = tr("%1 (%2)").arg( zistime ? uiStrings::sFrequency(true)
					      : uiStrings::sWaveNumber(true) )
			    .arg( zistime ? "Hz" : zismeter ? "/km" : "/kft" );
    const float nyq = 0.5f/SI().zStep() * (zistime ? 1.0f : 1000.0f);
    const float deffrq = 0.4*nyq;
    freqfld_ = new uiGenInput( this, txt, FloatInpSpec(deffrq),
			       FloatInpSpec(deffrq), FloatInpSpec(deffrq) );
    freqfld_->addInput( FloatInpSpec(deffrq) );
    freqfld_->attach( alignedBelow, typefld_ );

    setHAlignObj( typefld_ );

    mAttachCB(postFinalize(), uiFreqFilter::initGrp);
}


uiFreqFilter::~uiFreqFilter()
{
    detachAllNotifiers();
}


bool uiFreqFilter::zisStdTime() const
{
    return SI().zDomain().isTime() && SI().zStep()>1e-4 && SI().zStep()<1e-2;
}


void uiFreqFilter::initGrp( CallBacker* )
{
    if ( zisStdTime() )
	setLowPass( 10.f, 15.f );
    else
    {
	const float nyq = 0.5f/SI().zStep() *
				    (SI().zDomain().isTime() ? 1.0f : 1000.0f);
	setLowPass( nyq*0.4f, nyq*0.5f );
    }

    mAttachCB(typefld_->valueChanged, uiFreqFilter::typeSelCB);
    mAttachCB(freqfld_->valueChanged, uiFreqFilter::freqChgCB);
    typeSelCB( nullptr );
}


void uiFreqFilter::setLowPass( float f3, float f4 )
{
    if ( f3>f4 )
	return;

    setFilter( mUdf(float), mUdf(float), f3, f4, FFTFilter::LowPass );
}


void uiFreqFilter::setHighPass( float f1, float f2 )
{
    if ( f1>f2 )
	return;

    setFilter( f1, f2, mUdf(float), mUdf(float), FFTFilter::HighPass );
}


void uiFreqFilter::setBandPass( float f1, float f2, float f3, float f4 )
{
    if ( f1>f2 || f2>f3 || f3>f4 )
	return;

    setFilter( f1, f2, f3, f4, FFTFilter::LowPass );
}


void uiFreqFilter::setFilter( float f1, float f2, float f3, float f4,
			      FFTFilter::Type type )
{
    const bool islowpass = type==FFTFilter::LowPass;
    const bool ishighpass = type==FFTFilter::HighPass;
    const bool zisstdtime = zisStdTime();
    const float nyq = 0.5f/SI().zStep() *
				    (SI().zDomain().isTime() ? 1.0f : 1000.0f);
    if ( islowpass )
    {
	f1_ = f2_ = mUdf(float);
	f3_ = mIsUdf(f3) || f3<0.0f ? (zisstdtime ? 10.0f : 0.4f*nyq) : f3;
	f4_ = mIsUdf(f4) || f3_>f4 ? (zisstdtime ? 15.0f : f3_+0.1f*nyq) : f4;
    }
    else if ( ishighpass )
    {
	f3_ = f4_ = mUdf(float);
	f1_ = mIsUdf(f1) || f1<0 ? (zisstdtime ? 50.0f : 0.1f*nyq) : f1;
	f2_ = mIsUdf(f2) || f1_>f2 ? (zisstdtime ? 60.0f : f1_+0.2f*nyq) : f2;
    }
    else
    {
	f1_ = mIsUdf(f1) || f1<0 ? (zisstdtime ? 10.0f : 0.1f*nyq) : f1;
	f2_ = mIsUdf(f2) || f1_>f2 ? (zisstdtime ? 15.0f : f1_+0.1f*nyq) : f2;
	f3_ = mIsUdf(f3) || f2_>f3 ? (zisstdtime ? 50.0f : f2_+0.2f*nyq) : f3;
	f4_ = mIsUdf(f4) || f3_>f4 ? (zisstdtime ? 60.0f : f3_+0.1f*nyq) : f4;
    }

    filtertype_ = type;
    putToScreen();
}


void uiFreqFilter::putToScreen()
{
    NotifyStopper nstype( typefld_->valueChanged );
    NotifyStopper nsfreq( freqfld_->valueChanged );
    typefld_->setValue( filtertype_ );
    const bool islowpass = filtertype_==FFTFilter::LowPass;
    const bool ishighpass = filtertype_==FFTFilter::HighPass;
    if ( islowpass )
    {
	freqfld_->setValue( f3_, 0 );
	freqfld_->setValue( f4_, 1 );
	freqfld_->setEmpty( 2 );
	freqfld_->setEmpty( 3 );
    }
    else if ( ishighpass )
    {
	freqfld_->setValue( f1_, 0 );
	freqfld_->setValue( f2_, 1 );
	freqfld_->setEmpty( 2 );
	freqfld_->setEmpty( 3 );
    }
    else
    {
	freqfld_->setValue( f1_, 0 );
	freqfld_->setValue( f2_, 1 );
	freqfld_->setValue( f3_, 2 );
	freqfld_->setValue( f4_, 3 );
    }

    valueChanged.trigger();
}


void uiFreqFilter::typeSelCB( CallBacker* )
{
    const auto type = (FFTFilter::Type)typefld_->getIntValue();
    const bool isbandpass = type==FFTFilter::BandPass;
    const bool islowpass = type==FFTFilter::LowPass;
    const bool ishighpass = type==FFTFilter::HighPass;
    freqfld_->displayField( isbandpass || ishighpass || islowpass, 0, 0 );
    freqfld_->displayField( isbandpass || ishighpass || islowpass, 0, 1 );
    freqfld_->displayField( isbandpass, 0, 2 );
    freqfld_->displayField( isbandpass, 0, 3 );
    setFilter( mUdf(float), mUdf(float), mUdf(float), mUdf(float), type );
}


void uiFreqFilter::freqChgCB( CallBacker* )
{
    const auto type = (FFTFilter::Type)typefld_->getIntValue();
    const bool islowpass = type==FFTFilter::LowPass;
    const float f1 = freqfld_->getFValue( 0 );
    const float f2 = freqfld_->getFValue( 1 );
    const float f3 = freqfld_->getFValue( islowpass ? 0 : 2 );
    const float f4 = freqfld_->getFValue( islowpass ? 1 : 3 );
    setFilter( f1, f2, f3, f4, type );
}


TypeSet<float> uiFreqFilter::frequencies() const
{
    TypeSet<float> res;
    const bool isbandpass = filtertype_==FFTFilter::BandPass;
    const bool islowpass = filtertype_==FFTFilter::LowPass;
    res += islowpass ? f3_ : f1_;
    res += islowpass ? f4_ : f2_;
    if ( isbandpass )
    {
	res += f3_;
	res += f4_;
    }

    return res;
}
