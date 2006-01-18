/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: shiftattrib.cc,v 1.16 2006-01-18 13:50:08 cvshelene Exp $";

#include "shiftattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "valseriesinterpol.h"


namespace Attrib
{

void Shift::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

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

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Shift::createInstance( Desc& desc )
{
    Shift* res = new Shift( desc );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}

    
void Shift::updateDesc( Desc& desc )
{
    desc.inputSpec(1).enabled = desc.getValParam(steeringStr())->getBoolValue();
}


Shift::Shift( Desc& desc_ )
    : Provider( desc_ )
{
    mGetBinID( pos, posStr() );
    mGetFloat( time, timeStr() );
    mGetBool( steering, steeringStr() );

    stepout = BinID( abs(pos.inl), abs(pos.crl) );

    if ( steering )
    {
	time = mMAX(stepout.inl*inldist(),stepout.crl*crldist()) * mMAXDIP;
	interval = Interval<float>( -time/zFactor(), time/zFactor() );
    }
    else
    {
	interval.start = time<0 ? time/zFactor() : 0;
	interval.stop = time>0 ? time/zFactor() : 0;
    }
}


void Shift::initSteering()
{
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx] && inputs[idx]->getDesc().isSteering() )
	    inputs[idx]->initSteering( stepout );
    }
}


bool Shift::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !steering || !input ) return Provider::getInputOutput( input, res );

    res += getSteeringIndex( pos );
    return true;
}


bool Shift::getInputData( const BinID& relpos, int zintv )
{
    const BinID bidstep = inputs[0]->getStepoutStep();
    const BinID posneeded = relpos + bidstep*pos;
    inputdata = inputs[0]->getData( posneeded, zintv );
    if ( !inputdata )
	return false;

    dataidx_ = getDataIndex( 0 );
    
    steeringdata = steering ? inputs[1]->getData( relpos, zintv ) : 0;
    if ( !steeringdata && steering )
	return false;

    return true;
}


bool Shift::computeData( const DataHolder& output, const BinID& relpos,
			 int z0, int nrsamples ) const
{
    if ( !outputinterest[0] ) return false;

    float sampleshift = time/(zFactor()*refstep);
    const int sampleidx = mNINT(sampleshift);
    const bool dointerpolate = steering || 
			       !mIsEqual(sampleshift,sampleidx,0.001);
    ValueSeriesInterpolator<float> interp( inputdata->nrsamples_-1 );
    const int steeringpos = steering ? getSteeringIndex(pos) : 0;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	if ( steering )
	{
	    const int validx = idx + z0 - steeringdata->z0_;
	    sampleshift = steeringdata->series(steeringpos)->value( validx );
	}

	const ValueSeries<float>* curdata = inputdata->series( dataidx_ );
	const int cursample = z0 - inputdata->z0_ + idx;
	const float val = dointerpolate 
			    ? interp.value( *curdata, cursample+sampleshift ) 
			    : curdata->value( cursample+sampleidx );

	output.series(0)->setValue( z0-output.z0_+idx, val );
    }

    return true;
}


const BinID* Shift::reqStepout( int inp, int out ) const
{ return inp==1 ? 0 : &stepout; }


const Interval<float>* Shift::reqZMargin( int inp, int ) const
{ return inp==1 ? 0 : &interval; }

}; // namespace Attrib
