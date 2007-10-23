/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: energyattrib.cc,v 1.22 2007-10-23 12:53:29 cvshelene Exp $";

#include "energyattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "statruncalc.h"
#include "survinfo.h"


namespace Attrib
{

mAttrDefCreateInstance(Energy)
    
void Energy::initClass()
{
    mAttrStartInitClass

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-1000,1000) );
    gate->setDefaultValue( Interval<float>(-28, 28) );
    desc->addParam( gate );

    desc->addInput( InputSpec("Input Data",true) );
    desc->setNrOutputs( Seis::UnknowData, 3 );

    mAttrEndInitClass
}


Energy::Energy( Desc& ds )
    : Provider(ds)
{
    if ( !isOK() ) return;

    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1/zFactor() );
}


bool Energy::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Energy::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs[0]->getData( relpos, zintv );
    dataidx_ = getDataIndex( 0 );
    return inputdata_;
}


bool Energy::computeData( const DataHolder& output, const BinID& relpos,
			  int z0, int nrsamples, int threadid ) const
{
    if ( !inputdata_ || inputdata_->isEmpty() || output.isEmpty() )
	return false;

    Interval<int> samplegate( mNINT(gate_.start/refstep),
			      mNINT(gate_.stop/refstep) );
    const int sz = samplegate.width() + 1;
    Stats::WindowedCalc<float> wcalc(
	    Stats::RunCalcSetup().require(Stats::SqSum), sz );

    const int stopidx = sz + nrsamples - 1;
    for ( int idx=0; idx<stopidx; idx++ )
    {
	wcalc += getInputValue( *inputdata_, dataidx_, samplegate.start+idx,z0);
	const int outidx = idx - sz + 1;
	if ( outidx >= 0 )
	{
	    float val = wcalc.sqSum() / sz;
	    if ( isOutputEnabled(0) )
		setOutputValue( output, 0, outidx, z0, val );
	    if ( isOutputEnabled(1) )
		setOutputValue( output, 1, outidx, z0, sqrt(val) );
	    if ( isOutputEnabled(2) )
		setOutputValue( output, 2, outidx, z0,
			        mIsZero(val,mDefEps) ? mUdf(float) : log(val) );
	}
    }

    return true;
}

} // namespace Attrib
