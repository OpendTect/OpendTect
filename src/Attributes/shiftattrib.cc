/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: shiftattrib.cc,v 1.27 2008-09-23 06:03:16 cvsnageswara Exp $";

#include "shiftattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "valseriesinterpol.h"


namespace Attrib
{

mAttrDefCreateInstance(Shift)
    
void Shift::initClass()
{
    mAttrStartInitClassWithUpdate
    
    BinIDParam* pos = new BinIDParam( posStr() );
    pos->setDefaultValue( BinID(0,0) );
    desc->addParam( pos );

    desc->addParam( new FloatParam( timeStr() ) );

    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue(false);
    desc->addParam( steering );

    desc->addOutputDataType( Seis::UnknowData );

    desc->addInput( InputSpec("Input data",true) );

    InputSpec steeringspec( "Steering data", false );
    steeringspec.issteering = true;
    desc->addInput( steeringspec );

    mAttrEndInitClass
}


void Shift::updateDesc( Desc& desc )
{
    desc.inputSpec(1).enabled = desc.getValParam(steeringStr())->getBoolValue();
}


Shift::Shift( Desc& desc_ )
    : Provider(desc_)
    , steeridx_(-1)
{
    mGetBinID( pos_, posStr() );
    mGetFloat( time_, timeStr() );
    mGetBool( dosteer_, steeringStr() );

    init();
}


void Shift::set( const BinID& bid, float z, bool dosteer )
{
    pos_ = bid; time_ = z; dosteer_ = dosteer;
    init();
}


void Shift::init()
{
    stepout_ = BinID( abs(pos_.inl), abs(pos_.crl) );

    interval_.start = time_<0 ? time_/zFactor() : 0;
    interval_.stop = time_>0 ? time_/zFactor() : 0;

    if ( dosteer_ )
    {
	float maxso = mMAX(stepout_.inl*inldist(),stepout_.crl*crldist());
	interval_ = interval_ + Interval<float>(-maxso*mMAXDIP, maxso*mMAXDIP);
	desinterval_ = Interval<float>( -maxso*mMAXDIPSECURE, 
					maxso*mMAXDIPSECURE );
	steeridx_ = getSteeringIndex( pos_ );
    }
}


void Shift::initSteering()
{
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx] && inputs[idx]->getDesc().isSteering() )
	    inputs[idx]->initSteering( stepout_ );
    }
}


bool Shift::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !dosteer_ || !input ) return Provider::getInputOutput( input, res );

    res += steeridx_;
    return true;
}


bool Shift::getInputData( const BinID& relpos, int zintv )
{
    const BinID bidstep = inputs[0]->getStepoutStep();
    const BinID posneeded = relpos + bidstep*pos_;
    inputdata_ = inputs[0]->getData( posneeded, zintv );
    if ( !inputdata_ )
	return false;

    dataidx_ = getDataIndex( 0 );
    if ( steeridx_<0 ) steeridx_ = getSteeringIndex( pos_ );

    steeringdata_ = dosteer_ ? inputs[1]->getData( relpos, zintv ) : 0;
    if ( !steeringdata_ && dosteer_ )
	return false;

    return true;
}


bool Shift::computeData( const DataHolder& output, const BinID& relpos,
			 int z0, int nrsamples, int threadid ) const
{
    if ( !outputinterest[0] ) return false;

    float sampleshift = time_/(zFactor()*refstep);
    const int sampleidx = mNINT(sampleshift);
    const bool dointerpolate = dosteer_ || 
			       !mIsEqual(sampleshift,sampleidx,0.001);
    ValueSeriesInterpolator<float> interp( inputdata_->nrsamples_-1 );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	float tmpsampshift = sampleshift;
	if ( dosteer_ && steeringdata_->series(steeridx_) )
	    tmpsampshift += getInputValue( *steeringdata_, steeridx_, idx, z0 );

	const ValueSeries<float>* curdata = inputdata_->series( dataidx_ );
	const int cursample = z0 - inputdata_->z0_ + idx;
	const float val = dointerpolate 
			    ? interp.value( *curdata, cursample+tmpsampshift ) 
			    : curdata->value( cursample+sampleidx );

	setOutputValue( output, 0, idx, z0, val );
    }

    return true;
}


const BinID* Shift::reqStepout( int inp, int out ) const
{ return inp==1 ? 0 : &stepout_; }


#define mAdjustGate( cond, gatebound, plus )\
{\
    if ( cond )\
    {\
	int minbound = (int)(gatebound / refstep);\
	int incvar = plus ? 1 : -1;\
	gatebound = (minbound+incvar) * refstep;\
    }\
}

void Shift::prepPriorToBoundsCalc()
{
    const int truestep = mNINT( refstep*zFactor() );
    if ( truestep == 0 )
	return Provider::prepPriorToBoundsCalc();

    bool chstartr = mNINT(interval_.start*zFactor()) % truestep;
    bool chstopr = mNINT(interval_.stop*zFactor()) % truestep;
    bool chstartd =mNINT(desinterval_.start*zFactor()) % truestep;
    bool chstopd = mNINT(desinterval_.stop*zFactor()) % truestep;

    mAdjustGate( chstartr, interval_.start, false )
    mAdjustGate( chstopr, interval_.stop, true )
    mAdjustGate( chstartd, desinterval_.start, false )
    mAdjustGate( chstopd, desinterval_.stop, true )

    Provider::prepPriorToBoundsCalc();
}


const Interval<float>* Shift::reqZMargin( int inp, int ) const
{ 
    return inp==1 ? 0 : &interval_;
}


const Interval<float>* Shift::desZMargin( int inp, int ) const
{ 
   if ( inp==1 || !dosteer_ )
	return 0;
   
    return &desinterval_;
}

}; // namespace Attrib
