/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: positionattrib.cc,v 1.41 2012-07-27 08:25:57 cvsnageswara Exp $";


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
    steerspec.issteering_ = true;
    desc->addInput( steerspec );

    mAttrEndInitClass
}


void Position::updateDesc( Desc& desc )
{
    desc.inputSpec(2).enabled_ =
		desc.getValParam( steeringStr() )->getBoolValue();
}


const char* Position::operTypeStr( int type )
{
    if ( type==mPositionOperMin ) return "Min";
    if ( type==mPositionOperMax ) return "Max";
    return "Median";
}

    
Position::Position( Desc& desc )
    : Provider( desc )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    mGetBinID( stepout_, stepoutStr() );
    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1./zFactor() );

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
    const float maxsecdip = maxSecureDip();
    desgate_ = Interval<float>( gate_.start-maxso*maxsecdip, 
	    			gate_.stop+maxso*maxsecdip );
}


Position::~Position()
{
    delete outdata_;
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
    BinID bidstep = inputs_[0]->getStepoutStep();
    //bidstep.inl = abs(bidstep.inl); bidstep.crl = abs(bidstep.crl);

    while ( inputdata_.size()<nrpos )
	inputdata_ += 0;
    
    for ( int posidx=0; posidx<nrpos; posidx++ )
    {
	BinID truepos = relpos + positions_[posidx] * bidstep;
	const DataHolder* indata = inputs_[0]->getData( truepos, zintv );

	const DataHolder* odata = inputs_[1]->getData( truepos, zintv );
	if ( !indata || !odata ) return false;

	inputdata_.replace( posidx, indata );
	outdata_->set( positions_[posidx].inl + stepout_.inl,
		       positions_[posidx].crl + stepout_.crl, odata );
    }
    
    inidx_ = getDataIndex( 0 );
    outidx_ = getDataIndex( 1 );
    steerdata_ = dosteer_ ? inputs_[2]->getData( relpos, zintv ) : 0;

    return true;
}


bool Position::computeData( const DataHolder& output, const BinID& relpos,
			    int z0, int nrsamples, int threadid ) const
{
    if ( inputdata_.isEmpty() || !outdata_ ) return false;
    
    const Interval<int> samplegate( mNINT32(gate_.start/refstep_),
				    mNINT32(gate_.stop/refstep_) );

    const Stats::Type statstype =  oper_ == 2 ? Stats::Median
				: (oper_ == 1 ? Stats::Max
					      : Stats::Min );
    Stats::RunCalc<float> stats( Stats::CalcSetup().require(statstype) );
    const float extrasamp = output.extrazfromsamppos_/refstep_;

    const int nrpos = positions_.size();
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	TypeSet<BinIDValue> bidv;
	stats.clear();

	for ( int idp=0; idp<nrpos; idp++ )
	{
	    const DataHolder* dh = inputdata_[idp];
	    if ( !dh || dh->isEmpty() || !dh->series(inidx_) ) 
		continue;

	    int ds = samplegate.start;

	    float sample = idx + extrasamp;
	    const int steeridx = dosteer_ ? steerindexes_[idp] : -1;
	    if ( dosteer_ && steerdata_->series(steeridx) )
		sample += getInputValue( *steerdata_, steeridx, idx, z0 );
		
	    for ( int ids=0; ids<samplegate.width()+1; ids++ )
	    {
		float place = sample + ds;
		stats += getInterpolInputValue( *dh, inidx_, place, z0 );
		bidv += BinIDValue( positions_[idp], place );
		ds++;
	    }
	}

	float val = mUdf(float);
	if ( !stats.isEmpty() ) 
	{
    	    const int posidx = stats.getIndex( statstype );
    	    BinID bid = bidv[posidx].binid;
    	    const DataHolder* odata = outdata_->get( bid.inl+stepout_.inl, 
						     bid.crl+stepout_.crl );
	    val = 0;
    	    if ( odata && !odata->isEmpty() && odata->series(outidx_) )
    		val = getInterpolInputValue( *odata, outidx_,
			bidv[posidx].value, z0 );
	}

	setOutputValue( output, 0, idx, z0, val );
    }

    return true;
}


#define mAdjustGate( cond, gatebound, plus )\
{\
    if ( cond )\
    {\
	int minbound = (int)(gatebound / refstep_);\
	int incvar = plus ? 1 : -1;\
	gatebound = (minbound+incvar) * refstep_;\
    }\
}

void Position::prepPriorToBoundsCalc()
{
    const int truestep = mNINT32( refstep_*zFactor() );
    if( truestep == 0 )
	return Provider::prepPriorToBoundsCalc();

    bool chgstartr = mNINT32(gate_.start*zFactor()) % truestep;
    bool chgstopr = mNINT32(gate_.stop*zFactor()) % truestep;
    bool chgstartd = mNINT32(desgate_.start*zFactor()) % truestep;
    bool chgstopd = mNINT32(desgate_.stop*zFactor()) % truestep;

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
