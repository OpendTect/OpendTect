/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Paul
 * DATE     : Dec 2012
-*/


#include "ceemdattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "ceemdalgo.h"
#include "statrand.h"
#include "survinfo.h"


namespace Attrib
{

mAttrDefCreateInstance(CEEMD)

void CEEMD::initClass()
{
    mAttrStartInitClassWithDescAndDefaultsUpdate

    EnumParam* ttype = new EnumParam( emdmethodStr() );
    //Note: Ordering must be the same as numbering!
    ttype->addEnum( transMethodNamesStr(mDecompModeEMD) );
    ttype->addEnum( transMethodNamesStr(mDecompModeEEMD) );
    ttype->addEnum( transMethodNamesStr(mDecompModeCEEMD) );
    ttype->setDefaultValue( mDecompModeCEEMD );
    desc->addParam( ttype );

    desc->addInput( InputSpec("Input Data",true) );
    desc->addOutputDataType( Seis::UnknowData );
    desc->setNrOutputs( Seis::UnknowData, 1  );
    desc->setLocality( Desc::SingleTrace );

    EnumParam* otype = new EnumParam( attriboutputStr() );
    otype->addEnum( transOutputNamesStr(mDecompOutputFreq) );
    otype->addEnum( transOutputNamesStr(mDecompOutputPeakFreq) );
    otype->addEnum( transOutputNamesStr(mDecompOutputPeakAmp) );
    otype->addEnum( transOutputNamesStr(mDecompOutputIMF) );
    otype->setDefaultValue( mDecompOutputFreq );
    desc->addParam( otype );

    FloatParam* stopimf = new FloatParam( stopimfStr() );
    float defval = 0.005;
    stopimf->setDefaultValue( defval );
    desc->addParam( stopimf );

    FloatParam* stopsift = new FloatParam( stopsiftStr() );
    defval = 0.2;
    stopsift->setDefaultValue( defval );
    desc->addParam( stopsift );

    IntParam* maxnrimf = new IntParam( maxnrimfStr() );
    maxnrimf->setDefaultValue( 16 );
    desc->addParam( maxnrimf );

    IntParam* maxsift = new IntParam( maxsiftStr() );
    maxsift->setDefaultValue( 10 );
    desc->addParam( maxsift );

    FloatParam* outputfreq = new FloatParam( outputfreqStr() );
    outputfreq->setDefaultValue( 5 );
    desc->addParam( outputfreq );

    FloatParam* stepoutfreq = new FloatParam( stepoutfreqStr() );
    stepoutfreq->setDefaultValue( 5 );
    desc->addParam( stepoutfreq );

    IntParam* outputcomp = new IntParam( outputcompStr() );
    outputcomp->setDefaultValue( 1 );
    outputcomp->setLimits( Interval<int>(1,50));
    desc->addParam( outputcomp );

    BoolParam* symmetricboundary = new BoolParam( symmetricboundaryStr() );
    symmetricboundary->setDefaultValue( true );
    symmetricboundary->setRequired(false);
    desc->addParam( symmetricboundary );

    BoolParam* usetfpanel = new BoolParam( usetfpanelStr() );
    usetfpanel->setDefaultValue( false );
    usetfpanel->setRequired(false);
    desc->addParam( usetfpanel );

    FloatParam* noisepercentage = new FloatParam( noisepercentageStr() );
    defval = 10.0;
    noisepercentage->setDefaultValue( defval );
    noisepercentage->setRequired(false);
    desc->addParam( noisepercentage );

    IntParam* maxnoiseloop = new IntParam( maxnoiseloopStr() );
    maxnoiseloop->setDefaultValue( 50 );
    maxnoiseloop->setRequired(false);
    desc->addParam( maxnoiseloop );


    desc->setLocality( Desc::SingleTrace );

    mAttrEndInitClass
}


const char* CEEMD::transMethodNamesStr(int type)
{
    if ( type==mDecompModeEMD ) return "EMD";
    if ( type==mDecompModeEEMD ) return "EEMD";
    return "CEEMD";
}

const char* CEEMD::transOutputNamesStr(int type)
{
    if ( type==mDecompOutputFreq ) return "Frequency";
    if ( type==mDecompOutputPeakFreq ) return "Peak Frequency";
    if ( type==mDecompOutputPeakAmp ) return "Peak Amplitude";
    return "IMF Component";
}


void CEEMD::updateDesc( Desc& desc )
{
    int nyquist = mCast( int, 1.f/(2.f*SI().zStep()));
    if ( desc.getValParam(usetfpanelStr())->getBoolValue() )
    {
	desc.setNrOutputs( Seis::UnknowData, nyquist+1 );
	return;
    }

    if ( desc.getValParam(attriboutputStr())->getIntValue()
	    == mDecompOutputIMF )
    {
	int maxnrimf =	desc.getValParam(maxnrimfStr())->getIntValue();
	desc.setNrOutputs( Seis::UnknowData, maxnrimf+1 );
    }

    if ( desc.getValParam(attriboutputStr())->getIntValue()
	    == mDecompOutputFreq )
    {
	float stepoutfreq =  desc.getValParam(stepoutfreqStr())->getFValue();

	const float nyqfreq = 0.5f / mCast(float,SI().zStep());
	const int nrattribs = (int)( nyqfreq / stepoutfreq );
	desc.setNrOutputs( Seis::UnknowData, nrattribs );
    }

	if ( (desc.getValParam(attriboutputStr())->getIntValue()
	    == mDecompOutputPeakFreq) ||
	    (desc.getValParam(attriboutputStr())->getIntValue()
	    == mDecompOutputPeakAmp) )
    {
	desc.setNrOutputs( Seis::UnknowData, 1 );
    }

}


CEEMD::CEEMD( Desc& desc )
    : Provider( desc )
{
    if ( !isOK() )
	return;

    mGetEnum( method_, emdmethodStr() );
    mGetEnum( attriboutput_, attriboutputStr() );
    mGetFloat( stopimf_, stopimfStr() );
    mGetFloat( stopsift_, stopsiftStr() );
    mGetInt( maxnrimf_, maxnrimfStr() );
    mGetInt( maxsift_, maxsiftStr() );
    mGetBool( usetfpanel_, usetfpanelStr() );
    mGetFloat( outputfreq_, outputfreqStr() );
    mGetFloat( stepoutfreq_, stepoutfreqStr() );
    if ( usetfpanel_ )
	stepoutfreq_ = 1;

    mGetInt( outputcomp_, outputcompStr() );
    mGetBool( symmetricboundary_, symmetricboundaryStr() );
    mGetFloat( noisepercentage_, noisepercentageStr() );
    mGetInt( maxnoiseloop_, maxnoiseloopStr() );

    dessampgate_ =  Interval<int>( -(1024-1), 1024-1 );
}


CEEMD::~CEEMD()
{
    delete gen_;
}


bool CEEMD::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool CEEMD::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs_[0]->getData( relpos, zintv );
    dataidx_ = getDataIndex( 0 );
    return inputdata_;
}


bool CEEMD::computeData( const DataHolder& output, const BinID& /* relpos */,
			 int z0, int nrsamples, int /* threadid */ ) const
{
    if ( !inputdata_ || inputdata_->isEmpty() || output.isEmpty() )
	return false;

    if ( method_ != mDecompModeEMD && !gen_ )
    {
	auto* gen = new Stats::NormalRandGen();
	mSelf().gen_ = gen;
    }

    const int inpsz = inputdata_->nrsamples_;
    DecompInput input( Setup().method(method_)
				.attriboutput(attriboutput_)
				.maxnrimf(maxnrimf_)
				.maxsift(maxsift_)
				.outputfreq(outputfreq_)
				.stepoutfreq(stepoutfreq_)
				.outputcomp(outputcomp_)
				.stopsift(stopsift_)
				.stopimf(stopimf_)
				    , inpsz, gen_ );

    int nyquist = mCast( int, 1.f/(2.f*SI().zStep()));
    int first = 0;
    int last = 0;
    getFirstAndLastOutEnabled(first, last);
    float startfreq = usetfpanel_ ? 0 : (first+1)*stepoutfreq_;
    float endfreq = usetfpanel_ ? nyquist : (last+1)*stepoutfreq_;
    int startcomp = first;
    int endcomp = last;
    const float stepoutfreq = usetfpanel_ ? 1.f : stepoutfreq_;

    for ( int idx=0; idx<nrsamples; idx++ )
	input.values_[idx] = getInputValue( *inputdata_, dataidx_, idx, z0 );

    Array2DImpl<float> decompoutput( inpsz, outputinterest_.size() );
    if ( !decompoutput.isOK() )
	return false;

    const bool success = input.doDecompMethod( inpsz, refstep_, decompoutput,
				attriboutput_, startfreq, endfreq, stepoutfreq,
				startcomp, endcomp );
    if ( !success )
	return false;

    for ( int comp=0; comp<outputinterest_.size(); comp++ )
    {
	if ( !outputinterest_[comp] ) continue;

	for ( int idx=0; idx<nrsamples; idx++ )
	{
	    const float val = decompoutput.get(idx, comp );
	    setOutputValue( output, comp, idx, z0, val );
	}
    }

    return true;
}

void CEEMD::getCompNames( BufferStringSet& nms ) const
{
    nms.erase();
    if ( attriboutput_ == 0 ) // frequency output
    {
	const float fnyq = 0.5f / refstep_;
	const char* basestr = "frequency = ";
	BufferString suffixstr = zIsTime() ? " Hz" : " cycles/mm";
	float startfreq = usetfpanel_ ? 0 : outputfreq_;
	float stepoutfreq = usetfpanel_ ? 1 : stepoutfreq_;
	for ( float freq=startfreq; freq<fnyq; freq+=stepoutfreq )
	{
	    BufferString tmpstr = basestr; tmpstr += freq; tmpstr += suffixstr;
	    nms.add( tmpstr.buf() );
	}
    }
    else if ( attriboutput_ == 3 ) // components output
    {
	const char* basestr = "component = ";
	for ( int comp=1; comp<=maxnrimf_; comp++ )
	{
	    BufferString tmpstr = basestr; tmpstr += comp;
	    nms.add( tmpstr.buf() );
	}
	BufferString tmpstr = "DC component";
	nms.add( tmpstr.buf() );
    }
    else return;
}


bool CEEMD::prepPriorToOutputSetup()
{
    return areAllOutputsEnabled();
}


bool CEEMD::areAllOutputsEnabled() const
{
    for ( int idx=0; idx<nrOutputs(); idx++ )
	if ( !outputinterest_[idx] )
	    return false;

    return true;
}


void CEEMD::getFirstAndLastOutEnabled( int& first, int& last ) const
{
    for ( int idx=0; idx<nrOutputs(); idx++ )
	if ( outputinterest_[idx] )
	{
	    first = idx;
	    break;
	}

    for ( int idx=nrOutputs()-1; idx>=0; idx-- )
	if ( outputinterest_[idx] )
	{
	    last = idx;
	    break;
	}
}

}; // namespace Attrib
