/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: volstatsattrib.cc,v 1.16 2006-01-12 20:37:38 cvsnanne Exp $";

#include "volstatsattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "runstat.h"
#include "valseriesinterpol.h"

#define mShapeRectangle	 0
#define mShapeEllipse	 1

namespace Attrib
{

void VolStats::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

/*
    IntParam* nrvolumes = new IntParam( nrvolumesStr() );
    nrvolumes->setLimits( Interval<int>(1,INT_MAX) );
    nrvolumes->setDefaultValue( 1 );
    nrvolumes->setValue( 1 );
    nrvolumes->setRequired( false );
    desc->addParam( nrvolumes );
*/

    BinIDParam* stepout = new BinIDParam( stepoutStr() );
    stepout->setDefaultValue( BinID(1,1) );
    desc->addParam( stepout );

    EnumParam* shape = new EnumParam( shapeStr() );
    //Note: Ordering must be the same as numbering!
    shape->addEnum( shapeTypeStr(mShapeRectangle) );
    shape->addEnum( shapeTypeStr(mShapeEllipse) );
    shape->setDefaultValue( mShapeRectangle );
    desc->addParam( shape );

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-mLargestZGate,mLargestZGate) );
    desc->addParam( gate );

/*
    BoolParam* absolutegate = new BoolParam( absolutegateStr() );
    absolutegate->setDefaultValue( false );
    absolutegate->setValue( false );
    absolutegate->setRequired( false );
    desc->addParam( absolutegate );
*/
    
    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue( false );
    desc->addParam( steering );

    desc->addInput( InputSpec("Input data",true) );

    InputSpec steeringspec( "Steering data", false );
    steeringspec.issteering = true;
    desc->addInput( steeringspec );

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* VolStats::createInstance( Desc& ds )
{
    VolStats* res = new VolStats( ds );
    res->ref();
    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void VolStats::updateDesc( Desc& desc )
{
    desc.inputSpec(1).enabled = desc.getValParam(steeringStr())->getBoolValue();
}


const char* VolStats::shapeTypeStr( int type )
{
    return type==mShapeRectangle ? "Rectangle" : "Ellipse";
}


int VolStats::outputtypes[] =
{
    RunStats::Mean,
    RunStats::Median,
    RunStats::Variance,
    RunStats::Min,
    RunStats::Max,
    RunStats::Sum,
    RunStats::NormVariance,
    -1
};

    
VolStats::VolStats( Desc& ds )
    : Provider( ds )
    , positions(0,BinID(0,0))
{
    if ( !isOK() ) return;

    inputdata.allowNull(true);
    
    mGetBinID( stepout, stepoutStr() );
    mGetEnum( shape, shapeStr() );
    mGetFloatInterval( gate, gateStr() );
    gate.start /= zFactor(); gate.stop /= zFactor();

    float extraz = mMAX(stepout.inl*inldist(), stepout.crl*crldist()) * mMAXDIP;
    desgate = Interval<float>( gate.start-extraz, gate.stop+extraz );
    
    mGetBool( steering, steeringStr() );
    
    BinID pos;
    for ( pos.inl=-stepout.inl; pos.inl<=stepout.inl; pos.inl++ )
    {
	for ( pos.crl=-stepout.crl; pos.crl<=stepout.crl; pos.crl++ )
	{
	    float relinldist = stepout.inl ? ((float)pos.inl)/stepout.inl : 0;
	    float relcrldist = stepout.crl ? ((float)pos.crl)/stepout.crl : 0;

	    float dist2 = relinldist*relinldist+relcrldist*relcrldist;

	    if ( shape==mShapeEllipse && dist2>1 )
		continue;

	    positions += pos;
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
	    inputs[idx]->initSteering( stepout );
    }
}


bool VolStats::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !steering || input<inputs.size()-1 ) 
	return Provider::getInputOutput( input, res );

    for ( int inl=-stepout.inl; inl<=stepout.inl; inl++ )
    {
	for ( int crl=-stepout.crl; crl<=stepout.crl; crl++ )
	    res += getSteeringIndex( BinID(inl,crl) );
    }
    return true;
}


bool VolStats::getInputData( const BinID& relpos, int zintv )
{
    while ( inputdata.size() < positions.size() )
	inputdata += 0;

    steeringdata = steering ? inputs[1]->getData( relpos, zintv ) : 0;
    if ( steering && !steeringdata )
	return false;

    const BinID bidstep = inputs[0]->getStepoutStep();
    const int nrpos = positions.size();

    for ( int posidx=0; posidx<nrpos; posidx++ )
    {
	const BinID truepos = relpos + positions[posidx] * bidstep;
	const DataHolder* indata = inputs[0]->getData( truepos, zintv );
	if ( !indata ) return false;

	inputdata.replace( posidx, indata );
    }

    dataidx_ = getDataIndex( 0 );
    return true;
}


const BinID* VolStats::reqStepout( int inp, int out ) const
{ return inp == 0 ? &stepout : 0; }


const Interval<float>* VolStats::reqZMargin( int inp, int ) const
{ return inp==0 ? &gate : 0; }


const Interval<float>* VolStats::desZMargin( int inp, int ) const
{ return inp==0 ? &desgate : 0; }


bool VolStats::computeData( const DataHolder& output, const BinID& relpos,
			    int z0, int nrsamples ) const
{
    const int nrpos = positions.size();
    const Interval<int> samplegate( mNINT(gate.start/refstep), 
				    mNINT(gate.stop/refstep) );
    const int gatesz = samplegate.width() + 1;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = z0 + idx;
	RunningStatistics<float> stats;
	for ( int posidx=0; posidx<nrpos; posidx++ )
	{
	    const DataHolder* dh = inputdata[posidx];
	    if ( !dh ) continue;

	    float shift = 0;
	    if ( steering )
	    {
		const int steeridx = getSteeringIndex( positions[posidx] );
		shift = steeringdata->series(steeridx)->
				    value( cursample-steeringdata->z0_ );
	    }

	    ValueSeriesInterpolator<float> interp( dh->nrsamples_-1 );

	    if ( !idx )
	    {
		int s = samplegate.start;
		for ( int idz=0; idz<gatesz; idz++ )
		{
		    const float samplepos = cursample + shift + s;
		    stats += interp.value( *dh->series(dataidx_),
					   samplepos-dh->z0_ );
		    s++;
		}
	    }
	    else
	    {
		const float samplepos = cursample + samplegate.stop;
		stats += interp.value( *dh->series(dataidx_),
					samplepos-dh->z0_ );
	    }
	}

	if ( !idx ) stats.lock();

        const int nroutp = outputinterest.size();
	for ( int outidx=0; outidx<nroutp; outidx++ )
	{
	    if ( outputinterest[outidx] == 0 )
		continue;

	    const float outval = stats.size() 
		? stats.getValue( (RunStats::StatType)outputtypes[outidx] )
		: mUdf(float);
	    const int sampleidx = z0-output.z0_+idx;
	    output.series(outidx)->setValue( sampleidx, outval );
	}
    }

    return true;
}

} // namespace Attrib
