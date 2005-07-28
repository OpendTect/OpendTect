/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          November 2002
 RCS:           $Id: positionattrib.cc,v 1.2 2005-07-28 10:53:50 cvshelene Exp $
________________________________________________________________________

-*/


#include "positionattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "datainpspec.h"
#include "attribsteering.h"
#include "valseriesinterpol.h"
#include "runstat.h"


#define mPositionOperMin           0
#define mPositionOperMax           1
#define mPositionOperMedian        2


namespace Attrib
{

void Position::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    BinIDParam* stepout = new BinIDParam(stepoutStr());
    stepout->setDefaultValue(BinID(0,0));
    desc->addParam( stepout );

    ZGateParam* gate = new ZGateParam(gateStr());
    gate->setLimits( Interval<float>(-mLargestZGate, mLargestZGate) );
    desc->addParam( gate );

    EnumParam* oper = new EnumParam(operStr());
    //Note: Ordering must be the same as numbering!
    oper->addEnum(operTypeStr(mPositionOperMin));
    oper->addEnum(operTypeStr(mPositionOperMax));
    oper->addEnum(operTypeStr(mPositionOperMedian));
    oper->setDefaultValue("0");
    oper->setRequired(false);
    desc->addParam(oper);

    BoolParam* steering = new BoolParam(steeringStr());
    steering->setDefaultValue("false");
    steering->setRequired(false);
    desc->addParam(steering);

    desc->addOutputDataType( Seis::UnknowData );

    InputSpec inpspec( "Input attribute", true );
    desc->addInput( inpspec );

    InputSpec outspec( "Output attribute", true );
    desc->addInput( outspec );

    InputSpec steerspec( "Steering data", false );
    steerspec.issteering = true;
    desc->addInput( steerspec );

    desc->init();

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Position::createInstance( Desc& ds )
{
    Position* res = new Position( ds );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void Position::updateDesc( Desc& desc )
{
    bool issteer = ( (ValParam*)desc.getParam(steeringStr()) )->getBoolValue();
    desc.inputSpec(2).enabled = issteer;
}


const char* Position::operTypeStr(int type)
{
    if ( type==mPositionOperMin ) return "Min";
    if ( type==mPositionOperMax ) return "Max";
    return "Median";
}

    
Position::Position( Desc& desc_ )
    : Provider( desc_ )
    , positions(0,BinID(0,0))
{
    if ( !isOK() ) return;

    inputdata.allowNull(true);

    mGetBinID( stepout, stepoutStr() );
    mGetFloatInterval( gate, gateStr() );
    gate.start = gate.start / zFactor(); gate.stop = gate.stop / zFactor();

    mGetEnum( oper, operStr() );
    mGetBool( steering, steeringStr() );

    BinID pos;
    for ( pos.inl=-stepout.inl; pos.inl<=stepout.inl; pos.inl++ )
    {
	for ( pos.crl=-stepout.crl; pos.crl<=stepout.crl; pos.crl++ )
	{
	    positions += pos;
	}
    }

    outdata = new Array2DImpl< const DataHolder*>( stepout.inl *2 +1 , 
						    stepout.crl *2 +1 );
}


Position::~Position()
{
    delete outdata;
}


bool Position::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !steering || input==0 || input==1 ) 
	return Provider::getInputOutput( input, res );

    for ( int inl=-stepout.inl; inl<=stepout.inl; inl++ )
    {
	for ( int crl=-stepout.crl; crl<=stepout.crl; crl++ )
	    res += getSteeringIndex( BinID(inl,crl) );
    }

    return true;
}


bool Position::getInputData(const BinID& relpos, int idx)
{
    const int nrpos = positions.size();
    const int inlsz = stepout.inl * 2 + 1;
    const int crlsz = stepout.crl * 2 + 1;
    bool yn;
    BinID bidstep = inputs[0]-> getStepoutStep(yn);
    //bidstep.inl = abs(bidstep.inl); bidstep.crl = abs(bidstep.crl);

    while ( inputdata.size()<nrpos )
	inputdata += 0;
    
    for ( int idp=0; idp<nrpos; idp++ )
    {
	BinID truepos = relpos + positions[idp] * bidstep;
	const DataHolder* indata = inputs[0]->getData( truepos, idx );

	const DataHolder* odata = inputs[1]->getData( truepos, idx );
	if ( !indata || !odata ) return false;

	inputdata.replace( idp, indata );
	
	outdata->set( positions[idp].inl + stepout.inl,
		      positions[idp].crl + stepout.crl, odata);
    }
    
    steerdata = steering ? inputs[2]->getData(relpos, idx) : 0;

    return true;
}

bool Position::computeData( const DataHolder& output,
				const BinID& relpos,
				int t0, int nrsamples ) const
{
    if ( !inputdata.size() || !outdata ) return false;
    
    const int nrpos = positions.size();
    const int cposnr = (int)(nrpos/2);

    Interval<int> samplegate( mNINT(gate.start/refstep),
			    mNINT(gate.stop/refstep) );
    
    RunningStatistics<float> stats;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	int cursample = t0 + idx;
	TypeSet<BinIDValue> bidv;
	stats.clear();
	for ( int idp=0; idp<nrpos; idp++ )
	{
	    const DataHolder* dh = inputdata[idp];
	    if ( !dh ) continue;
	    ValueSeriesInterpolator<float> interp( dh->nrsamples_-1 );
	    int ds = samplegate.start;
	    int steeridx = getSteeringIndex(positions[idp]);

	    float sample = cursample;
	    if ( steering && steerdata->item(steeridx) )
		sample += steerdata->item(steeridx)->value( 
						cursample - steerdata->t0_ );
		
	    for ( int ids=0; ids< samplegate.width()+1; ids++ )
	    {
		float place = sample + ds - dh->t0_;
		stats += interp.value( *(dh->item(0)), place );
		bidv += BinIDValue( positions[idp], sample + ds );
		ds ++;
	    }
	}

	int posidx;
	float outval;
	switch ( oper )
	{
	    case 0: { outval = stats.min( &posidx ); } break;
	    case 1: { outval = stats.max( &posidx ); } break;
	    case 2: { outval = stats.median( &posidx ); } break;
	}

	BinID bid = bidv[posidx].binid;
	float sample = bidv[posidx].value;
	const DataHolder* odata = outdata->get( bid.inl + stepout.inl, 
						bid.crl + stepout.crl );
	
	float val = 0;
	if ( odata )
	{
	    ValueSeriesInterpolator<float> intp( odata->nrsamples_-1 );
	    val = intp.value( *(odata->item(0)), sample - odata->t0_ );
	}

	output.item(0)->setValue(idx, val);
    }

    return true;
}


const BinID* Position::reqStepout( int inp, int out ) const
{ return &stepout; }


const Interval<float>* Position::reqZMargin( int inp, int ) const
{ return inp==1 ? 0 : &gate; }

}//namespace
