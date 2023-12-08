/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "instantattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "survinfo.h"
#include "math2.h"

#include <math.h>


template<>
void EnumDefImpl<Attrib::Instantaneous::OutType>::init()
{
    uistrings_ += uiStrings::sAmplitude();
    uistrings_ += uiStrings::sPhase();
    uistrings_ += uiStrings::sFrequency();
    uistrings_ += tr("Hilbert");
    uistrings_ += tr("Amplitude 1st derivative");
    uistrings_ += tr("Amplitude 2nd derivative");
    uistrings_ += tr("Cosine phase");
    uistrings_ += tr("Envelope weighted phase");
    uistrings_ += tr("Envelope weighted frequency");
    uistrings_ += tr("Phase acceleration");
    uistrings_ += tr("Thin bed indicator");
    uistrings_ += tr("Bandwidth");
    uistrings_ += tr("Q factor");
    uistrings_ += tr("Rotate phase");
    uistrings_ += tr("Sweetness");
}

namespace Attrib
{
mDefineEnumUtils(Instantaneous,OutType,"Instantaneous Attribute")
{
    "Amplitude",
    "Phase",
    "Frequency",
    "Hilbert",
    "Amplitude 1st derivative",
    "Amplitude 2nd derivative",
    "Cosine phase",
    "Envelope weighted phase",
    "Envelope weighted frequency",
    "Phase acceleration",
    "Thin bed indicator",
    "Bandwidth",
    "Q factor",
    "Rotate phase",
    "Sweetness",
    nullptr
};


mAttrDefCreateInstance(Instantaneous)

void Instantaneous::initClass()
{
    mAttrStartInitClassWithUpdate

    desc->addInput( InputSpec("Imag Data",true) );
    desc->setNrOutputs( Seis::UnknowData, 15 );

    FloatParam* rotangle_ = new FloatParam( rotateAngle() );
    rotangle_->setLimits( Interval<float>(-180,180) );
    rotangle_->setDefaultValue(90);
    rotangle_->setRequired( false );
    desc->addParam( rotangle_ );

    desc->addInput( InputSpec("Real Data",true) );

    desc->setLocality( Desc::SingleTrace );
    mAttrEndInitClass
}


void Instantaneous::updateDesc( Desc& desc )
{
    int outputidx = desc.selectedOutput();
    desc.setParamEnabled( rotateAngle(), outputidx == RotatePhase );
}


Instantaneous::Instantaneous( Desc& ds )
    : Provider( ds )
    , sampgate1_( -1,1 )
    , sampgate2_( -2,2 )
    , rotangle_(0)
{
    if ( !isOK() ) return;

    mGetFloat( rotangle_, rotateAngle() );
}


Instantaneous::~Instantaneous()
{}


bool Instantaneous::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Instantaneous::getInputData( const BinID& relpos, int zintv )
{
    realdata_ = inputs_[0]->getData( relpos, zintv );
    imagdata_ = inputs_[1]->getData( relpos, zintv );
    realidx_ = getDataIndex( 0 );
    imagidx_ = getDataIndex( 1 );
    return realdata_ && imagdata_;
}


#define mGetRVal(sidx) getInputValue( *realdata_, realidx_, sidx, z0 )
#define mGetIVal(sidx) getInputValue( *imagdata_, imagidx_, sidx, z0 )


bool Instantaneous::areAllOutputsEnabled() const
{
    for (int idx=0; idx<nrOutputs(); idx++)
	if (!outputinterest_[idx])
	    return false;
    return true;
}


void Instantaneous::getCompNames( BufferStringSet& nms ) const
{
    nms.erase();
    const char* basestr = "Inst_";
    for ( const auto* attrib : OutTypeDef().keys() )
    {
	BufferString tmpstr = basestr; tmpstr += *attrib;
	nms.add( tmpstr.buf() );
    }
}


bool Instantaneous::prepPriorToOutputSetup()
{
    return areAllOutputsEnabled();
}


bool Instantaneous::computeData( const DataHolder& output, const BinID& relpos,
				 int z0, int nrsamples, int threadid ) const
{
    if ( !realdata_ || !imagdata_ ) return false;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	if ( isOutputEnabled(Amplitude) )
	    setOutputValue( output, Amplitude, idx, z0,
			    calcAmplitude(idx,z0) );
	if ( isOutputEnabled(Phase) )
	    setOutputValue( output, Phase, idx, z0, calcPhase(idx,z0) );
	if ( isOutputEnabled(Frequency) )
	    setOutputValue( output, Frequency, idx, z0,
			    calcFrequency(idx,z0) );
	if ( isOutputEnabled(Hilbert) )
	    setOutputValue( output, Hilbert, idx, z0, mGetIVal(idx) );
	if ( isOutputEnabled(Amp1Deriv) )
	    setOutputValue( output, Amp1Deriv, idx, z0,
			    calcAmplitude1Der(idx,z0) );
	if ( isOutputEnabled(Amp2Deriv) )
	    setOutputValue( output, Amp2Deriv, idx, z0,
			    calcAmplitude2Der(idx,z0) );
	if ( isOutputEnabled(CosPhase) )
	    setOutputValue( output, CosPhase, idx, z0,
			    cos(calcPhase(idx,z0)) );
	if ( isOutputEnabled(EnvWPhase) )
	    setOutputValue( output, EnvWPhase, idx, z0,
			    calcEnvWPhase(idx,z0) );
	if ( isOutputEnabled(EnvWFreq) )
	    setOutputValue( output, EnvWFreq, idx, z0,
			    calcEnvWFreq(idx,z0) );
	if ( isOutputEnabled(PhaseAccel) )
	    setOutputValue( output, PhaseAccel, idx, z0,
			    calcPhaseAccel(idx,z0) );
	if ( isOutputEnabled(ThinBed) )
	    setOutputValue( output, ThinBed, idx, z0, calcThinBed(idx,z0) );
	if ( isOutputEnabled(Bandwidth) )
	    setOutputValue( output, Bandwidth, idx, z0,
			    calcBandWidth(idx,z0) );
	if ( isOutputEnabled(QFactor) )
	    setOutputValue( output, QFactor, idx, z0, calcQFactor(idx,z0) );
	if ( isOutputEnabled(RotatePhase) )
	    setOutputValue( output, RotatePhase, idx, z0,
			    calcRotPhase(idx,z0,rotangle_));
	if ( isOutputEnabled(Sweetness) )
	    setOutputValue( output, Sweetness, idx, z0,
			    calcSweetness(idx,z0) );
    }

    return true;
}

#define mCheckRetUdf(val1,val2) \
    if ( mIsUdf(val1) || mIsUdf(val2) ) return mUdf(float);

#define mCheckDenom(val) \
    if ( mIsZero(val,mDefEpsF) ) return mUdf(float);

float Instantaneous::calcAmplitude( int cursample, int z0 ) const
{
    const float real = mGetRVal( cursample );
    const float imag = mGetIVal( cursample );
    mCheckRetUdf( real, imag )
    return Math::Sqrt( real*real + imag*imag );
}

#define mDT ( 2.f * refstep_ )

float Instantaneous::calcAmplitude1Der( int cursample, int z0 ) const
{
    const float prev = calcAmplitude( cursample-1, z0 );
    const float next = calcAmplitude( cursample+1, z0 );
    mCheckRetUdf( prev, next )
    return ( next - prev ) / mDT;
}


float Instantaneous::calcAmplitude2Der( int cursample, int z0 ) const
{
    const float prev = calcAmplitude1Der( cursample-1, z0 );
    const float next = calcAmplitude1Der( cursample+1, z0 );
    mCheckRetUdf( prev, next )
    return ( next - prev ) / mDT;
}


float Instantaneous::calcPhase( int cursample, int z0 ) const
{
    const float real = mGetRVal( cursample );
    const float imag = mGetIVal( cursample );
    mCheckRetUdf( real, imag )
    return Math::Atan2( mGetIVal( cursample ), mGetRVal( cursample ) );
}


float Instantaneous::calcFrequency( int cursample, int z0 ) const
{
    const float prev = calcPhase( cursample-1, z0 );
    const float next = calcPhase( cursample+1, z0 );
    mCheckRetUdf( prev, next )

    float dphase = next - prev;
    if ( dphase < 0.f )
	dphase += M_2PIf;

    return dphase / ( M_2PIf * mDT );
}


float Instantaneous::calcPhaseAccel( int cursample, int z0 ) const
{
    const float prev = calcFrequency( cursample-1, z0 );
    const float next = calcFrequency( cursample+1, z0 );
    mCheckRetUdf( prev, next )
    return M_2PIf * ( next - prev ) / mDT;
}


float Instantaneous::calcBandWidth( int cursample, int z0 ) const
{
    const float denv_dt = calcAmplitude1Der( cursample, z0 );
    const float env = calcAmplitude( cursample, z0 );
    mCheckRetUdf( denv_dt, env )
    mCheckDenom( env )

    return fabs( denv_dt / ( M_2PIf * env ) );
}


float Instantaneous::calcQFactor( int cursample, int z0 ) const
{
    const float ifq = calcFrequency( cursample, z0 );
    const float bandwth = calcBandWidth( cursample, z0 );
    mCheckRetUdf( ifq, bandwth )
    mCheckDenom( bandwth )

    return -0.5f * ifq / bandwth;
}


float Instantaneous::calcRotPhase( int cursample, int z0, float angle ) const
{
    const float real = mGetRVal( cursample );
    const float imag = mGetIVal( cursample );
    mCheckRetUdf( real, imag )
    const float radians = Math::toRadians( angle );

    return real * cos( radians ) - imag * sin( radians );
}


float Instantaneous::calcEnvWeighted( int cursample, int z0,
				      bool isphase ) const
{
    float sumiampenv = 0.f;
    float sumienv = 0.f;
    int nrsamples = 0;
    Interval<int> sg( -1, 1 );
    for ( int ids=sg.start; ids<=sg.stop; ids++ )
    {
	const float iamp = isphase ? calcPhase( cursample+ids, z0 )
				   : calcFrequency( cursample+ids, z0 );
	const float ienv = calcAmplitude( cursample+ids, z0 );
	if ( mIsUdf(iamp) || mIsUdf(ienv) ) continue;

	sumienv += ienv;
	sumiampenv += iamp * ienv;
	nrsamples++;
    }

    if ( !nrsamples )
	return mUdf(float);

    mCheckDenom( sumienv )

    return sumiampenv / sumienv;
}


float Instantaneous::calcEnvWPhase( int cursample, int z0 ) const
{
    return calcEnvWeighted( cursample, z0, true );
}


float Instantaneous::calcEnvWFreq( int cursample, int z0 ) const
{
    return calcEnvWeighted( cursample, z0, false );
}


float Instantaneous::calcThinBed( int cursample, int z0 ) const
{
    const float freq = calcFrequency( cursample, z0 );
    const float envwfreq = calcEnvWFreq( cursample, z0 );
    mCheckRetUdf( freq, envwfreq )

    return freq - envwfreq;
}


float Instantaneous::calcSweetness( int cursample, int z0 ) const
{
    const float env = calcAmplitude( cursample, z0 );
    const float freq = calcFrequency( cursample, z0 );
    mCheckRetUdf( env, freq )

    if ( freq<0 || mIsZero(freq,mDefEpsF) )
	return 0.f;

    return env / Math::Sqrt(freq);
}


const Interval<int>* Instantaneous::reqZSampMargin( int inp, int out ) const
{
    const OutType type = sCast(OutType,out);
    switch ( type )
    {
	case Frequency:
	case Amp1Deriv:
	case EnvWPhase:
	case Bandwidth:
	case QFactor:
	case Sweetness:
	    return &sampgate1_;
	case Amp2Deriv:
	case EnvWFreq:
	case PhaseAccel:
	case ThinBed:
	    return &sampgate2_;
	default:
	    return nullptr;
    }
}


} // namespace Attrib
