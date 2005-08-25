/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: energyattrib.cc,v 1.7 2005-08-25 14:57:13 cvshelene Exp $";


#include "energyattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "survinfo.h"
#include "datainpspec.h"
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

    desc->addOutputDataType( Seis::UnknowData );
    InputSpec inputspec( "Data on which the Energy should be measured", true );
    desc->addInput( inputspec );

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


Energy::Energy( Desc& desc )
        : Provider( desc )
{
    if ( !isOK() ) return;

    mGetFloatInterval( gate, gateStr() );
    gate.start = gate.start / zFactor();
    gate.stop = gate.stop / zFactor();

    inputdata.allowNull( true );
}


bool Energy::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Energy::getInputData( const BinID& relpos, int idx )
{
    if ( !inputdata.size() )
	inputdata += 0;

    const DataHolder* data = inputs[0]->getData( relpos, idx );
    inputdata.replace( 0, data );
    dataidx_ = getDataIndex( 0 );
    return data;
}


bool Energy::computeData( const DataHolder& output, const BinID& relpos,
			  int t0, int nrsamples ) const
{
    if ( !inputdata.size() || !output.nrItems() ) return false;

    ValueSeries<float>* outp = output.item(0);

    Interval<int> samplegate( mNINT(gate.start/refstep),
			      mNINT(gate.stop/refstep) );
    const int sz = samplegate.width() + 1;
    
    RunningStatistics<float> calc;

    int csample = t0 + samplegate.start;
    for ( int idx=0; idx<sz; idx++ )
    {
	const int csamp = csample + idx;
	calc += inputdata[0]->item(dataidx_)->value( csamp-inputdata[0]->t0_ );
    }

    outp->setValue( 0, calc.sqSum()/sz );

    calc.lock();

    csample = t0 + samplegate.stop;
    for ( int idx=1; idx<nrsamples; idx++ )
    {
	const int csamp = csample + idx;
	calc += inputdata[0]->item(dataidx_)->value( csamp-inputdata[0]->t0_ );
	outp->setValue( idx, calc.sqSum()/sz );
    }

    return true;
}
			
} // namespace Attrib
