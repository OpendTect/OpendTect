/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          November 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: positionattrib.cc,v 1.28 2009-04-02 14:20:57 cvshelene Exp $";


#include "positionattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "statruncalc.h"
#include "valseriesinterpol.h"


#define mPositionOperMin           0
#define mPositionOperMax           1
#define mPositionOperMedian        2


namespace Attrib
{

mAttrDefCreateInstance(Position)
    
void Position::initClass()
{
    mAttrStartInitClassWithUpdate

    BinIDParam* stepout = new BinIDParam( stepoutStr() );
    stepout->setDefaultValue( BinID(0,0) );
    desc->addParam( stepout );

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-mLargestZGate,mLargestZGate) );
    desc->addParam( gate );

    EnumParam* oper = new EnumParam( operStr() );
    //Note: Ordering must be the same as numbering!
    oper->addEnum( operTypeStr(mPositionOperMin) );
    oper->addEnum( operTypeStr(mPositionOperMax) );
    oper->addEnum( operTypeStr(mPositionOperMedian) );
    oper->setDefaultValue( mPositionOperMin );
    desc->addParam( oper );

    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue( false );
    desc->addParam( steering );

    desc->addInput( InputSpec("Input attribute",true) );
    desc->addInput( InputSpec("Output attribute",true) );
    desc->addOutputDataType( Seis::UnknowData );

    InputSpec steerspec( "Steering data", false );
    steerspec.issteering = true;
    desc->addInput( steerspec );

    mAttrEndInitClass
}


void Position::updateDesc( Desc& desc )
{
    desc.inputSpec(2).enabled =
		desc.getValParam( steeringStr() )->getBoolValue();
}


const char* Position::operTypeStr( int type )
{
    if ( type==mPositionOperMin ) return "Min";
    if ( type==mPositionOperMax ) return "Max";
    return "Median";
}

    
Position::Position( Desc& desc_ )
    : Provider( desc_ )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    mGetBinID( stepout_, stepoutStr() );
    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1/zFactor() );

    mGetEnum( oper_, operStr() );
    mGetBool( dosteer_, steeringStr() );

    BinID pos;
    for ( pos.inl=-stepout_.inl; pos.inl<=stepout_.inl; pos.inl++ )
    {
	for ( pos.crl=-stepout_.crl; pos.crl<=stepout_.crl; pos.crl++ )
	{
	    positions_ += pos;
	    if ( dosteer_ ) steerindexes_ += getSteeringIndex( pos );
	}
    }

    outdata_ = new Array2DImpl<const DataHolder*>( stepout_.inl*2+1,
						   stepout_.crl*2+1 );

    const float maxso = mMAX( stepout_.inl*inldist(), stepout_.crl*crldist() );
    desgate_ = Interval<float>( gate_.start-maxso*mMAXDIPSECURE, 
	    			gate_.stop+maxso*mMAXDIPSECURE );
}


Position::~Position()
{
    delete outdata_;
}


void Position::initSteering()
{
    for( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx] && inputs[idx]->getDesc().isSteering() )
	    inputs[idx]->initSteering(stepout_);
    }
}


bool Position::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !dosteer_ || input==0 || input==1 ) 
	return Provider::getInputOutput( input, res );

    for ( int idx=0; idx<positions_.size(); idx++ )
	res += steerindexes_[idx];

    return true;
}


bool Position::getInputData( const BinID& relpos, int zintv )
{
    const int nrpos = positions_.size();
    const int inlsz = stepout_.inl * 2 + 1;
    const int crlsz = stepout_.crl * 2 + 1;
    BinID bidstep = inputs[0]->getStepoutStep();
    //bidstep.inl = abs(bidstep.inl); bidstep.crl = abs(bidstep.crl);

    while ( inputdata_.size()<nrpos )
	inputdata_ += 0;
    
    for ( int posidx=0; posidx<nrpos; posidx++ )
    {
	BinID truepos = relpos + positions_[posidx] * bidstep;
	const DataHolder* indata = inputs[0]->getData( truepos, zintv );

	const DataHolder* odata = inputs[1]->getData( truepos, zintv );
	if ( !indata || !odata ) return false;

	inputdata_.replace( posidx, indata );
	outdata_->set( positions_[posidx].inl + stepout_.inl,
		       positions_[posidx].crl + stepout_.crl, odata );
    }
    
    inidx_ = getDataIndex( 0 );
    outidx_ = getDataIndex( 1 );
    steerdata_ = dosteer_ ? inputs[2]->getData( relpos, zintv ) : 0;

    return true;
}


bool Position::computeData( const DataHolder& output, const BinID& relpos,
			    int z0, int nrsamples, int threadid ) const
{
    if ( inputdata_.isEmpty() || !outdata_ ) return false;
    
    const int nrpos = positions_.size();
    const int cposnr = (int)(nrpos/2);

    const Interval<int> samplegate( mNINT(gate_.start/refstep),
				    mNINT(gate_.stop/refstep) );

    const Stats::Type statstype =  oper_ == 2 ? Stats::Median
				: (oper_ == 1 ? Stats::Max
					      : Stats::Min );
    Stats::RunCalc<float> stats( Stats::RunCalcSetup().require(statstype) );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = z0 + idx;
	TypeSet<BinIDValue> bidv;
	stats.clear();

	for ( int idp=0; idp<nrpos; idp++ )
	{
	    const DataHolder* dh = inputdata_[idp];
	    if ( !dh || dh->isEmpty() || !dh->series(inidx_) ) 
		continue;

	    ValueSeriesInterpolator<float> interp( dh->nrsamples_-1 );
	    int ds = samplegate.start;

	    float sample = cursample;
	    const int steeridx = dosteer_ ? steerindexes_[idp] : -1;
	    if ( dosteer_ && steerdata_->series(steeridx) )
		sample += getInputValue( *steerdata_, steeridx, idx, z0 );
		
	    for ( int ids=0; ids<samplegate.width()+1; ids++ )
	    {
		float place = sample + ds - dh->z0_;
		stats += interp.value( *(dh->series(inidx_)), place );
		bidv += BinIDValue( positions_[idp], sample+ds );
		ds++;
	    }
	}
	if ( stats.isEmpty() ) return false;

	const int posidx = stats.getIndex( statstype );
	BinID bid = bidv[posidx].binid;
	float sample = bidv[posidx].value;
	const DataHolder* odata = outdata_->get( bid.inl+stepout_.inl, 
						bid.crl+stepout_.crl );

	float val = 0;
	if ( odata && !odata->isEmpty() && odata->series(outidx_) )
	{
	    ValueSeriesInterpolator<float> intp( odata->nrsamples_-1 );
	    val = intp.value( *(odata->series(outidx_)), sample-odata->z0_ );
	}

	setOutputValue( output, 0, idx, z0, val );
    }

    return true;
}


#define mAdjustGate( cond, gatebound, plus )\
{\
    if ( cond )\
    {\
	int minbound = (int)(gatebound / refstep);\
	int incvar = plus ? 1 : -1;\
	gatebound = (minbound+incvar) * refstep;\
    }\
}

void Position::prepPriorToBoundsCalc()
{
    const int truestep = mNINT( refstep*zFactor() );
    if( truestep == 0 )
	return Provider::prepPriorToBoundsCalc();

    bool chgstartr = mNINT(gate_.start*zFactor()) % truestep;
    bool chgstopr = mNINT(gate_.stop*zFactor()) % truestep;
    bool chgstartd = mNINT(desgate_.start*zFactor()) % truestep;
    bool chgstopd = mNINT(desgate_.stop*zFactor()) % truestep;

    mAdjustGate( chgstartr, gate_.start, false )
    mAdjustGate( chgstopr, gate_.stop, true )
    mAdjustGate( chgstartd, desgate_.start, false )
    mAdjustGate( chgstopd, desgate_.stop, true )

    Provider::prepPriorToBoundsCalc();
}


const Interval<float>* Position::reqZMargin(int input,int output) const
{
    return &gate_;
}


const Interval<float>* Position::desZMargin(int input,int output) const
{
    return &desgate_;
}


} // namespace Attrib
