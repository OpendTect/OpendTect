/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: shiftattrib.cc,v 1.1 2005-07-06 15:02:07 cvshelene Exp $";

#include "hashattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "valseriesinterpol.h"
#include "datainpspec.h"
#include "stdio.h"


namespace Attrib
{

void Hash::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    BinIDParam* pos = new BinIDParam(posStr());
    pos->setDefaultValue(BinID(0,0));
    desc->addParam( pos );

    desc->addParam( new ValParam( timeStr(), new FloatInpSpec() ) );

    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue("false");
    steering->setRequired(false);
    desc->addParam( steering );

    desc->addOutputDataType( Seis::UnknowData );

    InputSpec inputspec( "Data on which the Reference shift should be done",
	                             true );
    desc->addInput( inputspec );

    InputSpec steeringspec( "Steering data", false );
    steeringspec.issteering = true;
    desc->addInput( steeringspec );

    desc->init();

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Hash::createInstance( Desc& ds )
{
    Hash* res = new Hash( ds );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}

    
void Hash::updateDesc( Desc& desc )
{
    bool issteer = ((ValParam*)desc.getParam(steeringStr()))->getBoolValue();
        desc.inputSpec(1).enabled = issteer;
}


Hash::Hash( Desc& desc_ )
    : Provider( desc_ )
{
    mGetBinID( pos, posStr() );
    mGetFloat( time, timeStr() );
    mGetBool( steering, steeringStr() );

    stepout = BinID( abs(pos.inl), abs(pos.crl) );
    interval = Interval<float>(-time/zFactor(), time/zFactor());
}


bool Hash::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !steering || !input ) return Provider::getInputOutput( input, res );

    res += getSteeringIndex( pos );
    return true;
}


bool Hash::getInputData(const BinID& relpos, int idx)
{
    const BinID bidstep = inputs[0]-> getStepoutStep();
    const BinID posneeded = relpos + bidstep*pos;
    inputdata = inputs[0]->getData(posneeded, idx);
    if ( !inputdata )
	return false;
    
    steeringdata = steering ? inputs[1]->getData(relpos, idx) : 0;
    if ( !steeringdata && steering )
	return false;

    return true;
}


bool Hash::computeData( const DataHolder& output, const BinID& relpos,
			int t0, int nrsamples ) const
{
    int steeringpos = steering ? getSteeringIndex(pos) : 0;
    if ( !outputinterest[0] ) return false;
    ValueSeriesInterpolator<float> interp(inputdata->nrsamples_-1);

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	int cursample = t0 - inputdata->t0_ + idx;
	const float steer = steering ? 
	    		steeringdata->item(steeringpos)->value(idx) : 0;
	const float val = steering 
	     ? interp.value( *(inputdata->item(0)), cursample + steer ) 
	     : interp.value( *(inputdata->item(0)), 
		     			cursample + time/(zFactor()*refstep) );
	
	output.item(0)->setValue(idx, val);
    }

    return true;
}


const BinID* Hash::reqStepout( int inp, int out ) const
{ return inp==1 ? 0 : &stepout; }


const Interval<float>* Hash::desZMargin( int inp, int ) const
{ return inp==1 ? 0 : &interval; }

};//namespace
