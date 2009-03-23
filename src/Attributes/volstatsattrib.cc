/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: volstatsattrib.cc,v 1.43 2009-03-23 11:11:30 cvshelene Exp $";

#include "volstatsattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "statruncalc.h"
#include "valseriesinterpol.h"

#define mShapeRectangle	 0
#define mShapeEllipse	 1

namespace Attrib
{


static int outputtypes[] =
{
    Stats::Average,
    Stats::Median,
    Stats::Variance,
    Stats::Min,
    Stats::Max,
    Stats::Sum,
    Stats::NormVariance,
    Stats::MostFreq,
    Stats::RMS,
    -1
};


mAttrDefCreateInstance(VolStats)

void VolStats::initClass()
{
    mAttrStartInitClassWithUpdate

    const BinID defstepout( 1, 1 );
    BinIDParam* stepout = new BinIDParam( stepoutStr() );
    stepout->setDefaultValue( defstepout );
    desc->addParam( stepout );

    EnumParam* shape = new EnumParam( shapeStr() );
    //Note: Ordering must be the same as numbering!
    shape->addEnum( shapeTypeStr(mShapeRectangle) );
    shape->addEnum( shapeTypeStr(mShapeEllipse) );
    shape->setDefaultValue( mShapeRectangle );
    desc->addParam( shape );

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-mLargestZGate,mLargestZGate) );
    gate->setDefaultValue( Interval<float>(-28, 28) );
    desc->addParam( gate );

    IntParam* nrtrcs = new IntParam( nrtrcsStr() );
    nrtrcs->setDefaultValue( 1 );
    nrtrcs->setRequired( false );
    desc->addParam( nrtrcs );

    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue( false );
    desc->addParam( steering );

    desc->addInput( InputSpec("Input data",true) );

    InputSpec steeringspec( "Steering data", false );
    steeringspec.issteering = true;
    desc->addInput( steeringspec );

    int res =0;
    while ( outputtypes[res++] != -1 )
	desc->addOutputDataType( Seis::UnknowData );

    mAttrEndInitClass
}


void VolStats::updateDesc( Desc& desc )
{
    desc.inputSpec(1).enabled = desc.getValParam(steeringStr())->getBoolValue();
}


const char* VolStats::shapeTypeStr( int type )
{
    return type==mShapeRectangle ? "Rectangle" : "Ellipse";
}

   
VolStats::VolStats( Desc& ds )
    : Provider( ds )
    , positions_(0,BinID(0,0))
    , desgate_(0,0)
    , reqgate_(0,0)
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);
    
    mGetBinID( stepout_, stepoutStr() );
    mGetInt( minnrtrcs_, nrtrcsStr() );
    mGetEnum( shape_, shapeStr() );
    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1/zFactor() );

    mGetBool( dosteer_, steeringStr() );

    if ( dosteer_ )
    {
	float maxso = mMAX(stepout_.inl*inldist(), stepout_.crl*crldist());
	reqgate_ = Interval<float>( gate_.start-maxso*mMAXDIP, 
				    gate_.stop+maxso*mMAXDIP );
	desgate_ = Interval<float>( gate_.start-maxso*mMAXDIPSECURE, 
				    gate_.stop+maxso*mMAXDIPSECURE );
    }
    else
	reqgate_ = gate_;

    BinID pos;
    for ( pos.inl=-stepout_.inl; pos.inl<=stepout_.inl; pos.inl++ )
    {
	for ( pos.crl=-stepout_.crl; pos.crl<=stepout_.crl; pos.crl++ )
	{
	    const float relinldist =
			stepout_.inl ? ((float)pos.inl)/stepout_.inl : 0;
	    const float relcrldist =
			stepout_.crl ? ((float)pos.crl)/stepout_.crl : 0;

	    const float dist2 = relinldist*relinldist + relcrldist*relcrldist;
	    if ( shape_==mShapeEllipse && dist2>1 )
		continue;

	    positions_ += pos;
	    if ( dosteer_ )
		steerindexes_ += getSteeringIndex( pos );
	}
    }
}


VolStats::~VolStats()
{
}


void VolStats::initSteering()
{
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx] && inputs[idx]->getDesc().isSteering() )
	    inputs[idx]->initSteering( stepout_ );
    }
}


bool VolStats::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !dosteer_ || input<inputs.size()-1 ) 
	return Provider::getInputOutput( input, res );

    for ( int idx=0; idx<positions_.size(); idx++ )
	res += steerindexes_[idx];

    return true;
}


bool VolStats::getInputData( const BinID& relpos, int zintv )
{
    inputdata_.erase();
    while ( inputdata_.size()<positions_.size() )
	inputdata_ += 0;

    steeringdata_ = dosteer_ ? inputs[1]->getData( relpos, zintv ) : 0;
    if ( dosteer_ && !steeringdata_ )
	return false;

    const BinID bidstep = inputs[0]->getStepoutStep();
    const int nrpos = positions_.size();

    int nrvalidtrcs = 0;
    for ( int posidx=0; posidx<nrpos; posidx++ )
    {
	const bool atcenterpos = positions_[posidx] == BinID(0,0);
	const BinID truepos = relpos + positions_[posidx] * bidstep;
	const DataHolder* indata = inputs[0]->getData( truepos, zintv );
	if ( !indata )
	{
	    if ( atcenterpos )
		return false;
	    else
		continue;
	}

	if ( dosteer_ )
	{
	    const int steeridx = steerindexes_[posidx];
	    if ( !steeringdata_->series(steeridx) )
	    {
		if ( atcenterpos )
		    return false;
		else
		    continue;
	    }
	}

	inputdata_.replace( posidx, indata );
	nrvalidtrcs++;
    }

    if ( nrvalidtrcs < minnrtrcs_ )
	return false;

    dataidx_ = getDataIndex( 0 );
    return true;
}


const BinID* VolStats::desStepout( int inp, int out ) const
{ return inp == 0 ? &stepout_ : 0; }


#define mAdjustGate( cond, gatebound, plus )\
{\
    if ( cond )\
    {\
	int minbound = (int)(gatebound / refstep);\
	int incvar = plus ? 1 : -1;\
	gatebound = (minbound+incvar) * refstep;\
    }\
}
    
void VolStats::prepPriorToBoundsCalc()
{
    const int truestep = mNINT( refstep*zFactor() );
    if ( truestep == 0 )
	return Provider::prepPriorToBoundsCalc();

    bool chgstartr = mNINT(reqgate_.start*zFactor()) % truestep;
    bool chgstopr = mNINT(reqgate_.stop*zFactor()) % truestep;
    bool chgstartd = mNINT(desgate_.start*zFactor()) % truestep;
    bool chgstopd = mNINT(desgate_.stop*zFactor()) % truestep;

    mAdjustGate( chgstartr, reqgate_.start, false )
    mAdjustGate( chgstopr, reqgate_.stop, true )
    mAdjustGate( chgstartd, desgate_.start, false )
    mAdjustGate( chgstopd, desgate_.stop, true )

    Provider::prepPriorToBoundsCalc();
}


const Interval<float>* VolStats::reqZMargin( int inp, int ) const
{
    return inp ? &gate_ : &reqgate_;
}


const Interval<float>* VolStats::desZMargin( int inp, int ) const
{     
    if ( inp || !dosteer_ ) return 0;
    
    return &desgate_;
}


bool VolStats::computeData( const DataHolder& output, const BinID& relpos,
			    int z0, int nrsamples, int threadid ) const
{
    const int nrpos = positions_.size();
    const Interval<int> samplegate( mNINT(gate_.start/refstep), 
				    mNINT(gate_.stop/refstep) );
    const int gatesz = samplegate.width() + 1;

    Stats::RunCalcSetup rcsetup;
    for ( int outidx=0; outidx<outputinterest.size(); outidx++ )
    {
	if ( outputinterest[outidx] )
	    rcsetup.require( (Stats::Type)outputtypes[outidx] );
    }
    int statsz = 0;
    for ( int posidx=0; posidx<nrpos; posidx++ )
    {
	if ( inputdata_[posidx] )
	    statsz += gatesz;
    }

    Stats::WindowedCalc<double> wcalc( rcsetup, statsz );

    for ( int idz=samplegate.start; idz<=samplegate.stop; idz++ )
    {
	for ( int posidx=0; posidx<nrpos; posidx++ )
	{
	    const DataHolder* dh = inputdata_[posidx];
	    if ( !dh ) continue;

	    float shift = 0;
	    if ( dosteer_ )
		shift = getInputValue( *steeringdata_, steerindexes_[posidx],
					idz, z0 );

	    ValueSeriesInterpolator<float> interp( dh->nrsamples_-1 );

	    const float samplepos = z0 + shift + idz;
	    wcalc += interp.value( *dh->series(dataidx_),
				   samplepos-dh->z0_ );
	}
    }

    for ( int isamp=0; isamp<nrsamples; isamp++ )
    {
	const int cursample = z0 + isamp;
	if ( isamp )
	{
	    for ( int posidx=0; posidx<nrpos; posidx++ )
	    {
		const DataHolder* dh = inputdata_[posidx];
		if ( !dh ) continue;

		float shift = 0;
		if ( dosteer_ )
		    shift = getInputValue( *steeringdata_,steerindexes_[posidx],
					    isamp+samplegate.stop, z0 );

		ValueSeriesInterpolator<float> interp( dh->nrsamples_-1 );

		const float samplepos = cursample + shift + samplegate.stop;
		wcalc += interp.value( *dh->series(dataidx_),
					samplepos-dh->z0_ );
	    }
	}

        const int nroutp = outputinterest.size();
	for ( int outidx=0; outidx<nroutp; outidx++ )
	{
	    if ( outputinterest[outidx] == 0 )
		continue;

	    const float outval = wcalc.getValue(
		    			(Stats::Type)outputtypes[outidx] );
	    setOutputValue( output, outidx, isamp, z0, outval );
	}
    }

    return true;
}

} // namespace Attrib
