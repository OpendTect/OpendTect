/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: energyattrib.cc,v 1.10 2005-12-23 16:09:46 cvsnanne Exp $";

#include "energyattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "runstat.h"


namespace Attrib
{

void Energy::initClass()
{
    Desc* desc = new Desc( attribName() );
    desc->ref();

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-1000,1000) );
    desc->addParam( gate );

    desc->addInput( InputSpec("Input Data",true) );
    desc->addOutputDataType( Seis::UnknowData );

    desc->init();

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Energy::createInstance( Desc& desc )
{
    Energy* res = new Energy( desc );
    res->ref();
    if ( !res->isOK() )
    {
        res->unRef();
        return 0;
    }

    res->unRefNoDelete();
    return res;
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
			  int z0, int nrsamples ) const
{
    if ( !inputdata_ || !inputdata_->nrSeries() || !output.nrSeries() )
	return false;

    ValueSeries<float>* res = output.series(0);
    if ( !res ) return false;

    RunningStatistics<float> calc;

    Interval<int> samplegate( mNINT(gate_.start/refstep),
			      mNINT(gate_.stop/refstep) );
    const int sz = samplegate.width() + 1;
    int csample = z0 + samplegate.start;
    for ( int idx=0; idx<sz; idx++ )
    {
	const int csamp = csample + idx;
	calc += inputdata_->series(dataidx_)->value( csamp-inputdata_->z0_ );
    }

    const int z0_offset = z0-output.z0_;
    res->setValue( z0_offset, calc.sqSum()/sz );

    calc.lock();
    csample = z0 + samplegate.stop;
    for ( int idx=1; idx<nrsamples; idx++ )
    {
	const int csamp = csample + idx;
	calc += inputdata_->series(dataidx_)->value( csamp-inputdata_->z0_ );
	res->setValue( z0_offset+idx, calc.sqSum()/sz );
    }

    return true;
}

} // namespace Attrib
