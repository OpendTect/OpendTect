/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: shiftattrib.cc,v 1.19 2006-05-12 07:55:37 cvshelene Exp $";

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
    : Provider(desc_)
    , steeridx_(-1)
{
    mGetBinID( pos_, posStr() );
    mGetFloat( time_, timeStr() );
    mGetBool( dosteer_, steeringStr() );

    stepout_ = BinID( abs(pos_.inl), abs(pos_.crl) );

    if ( dosteer_ )
    {
	time_ = mMAX(stepout_.inl*inldist(),stepout_.crl*crldist()) * mMAXDIP;
	interval_ = Interval<float>( -time_, time_ );
	time_ *= zFactor();
	steeridx_ = getSteeringIndex( pos_ );
    }
    else
    {
	interval_.start = time_<0 ? time_/zFactor() : 0;
	interval_.stop = time_>0 ? time_/zFactor() : 0;
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
			 int z0, int nrsamples ) const
{
    if ( !outputinterest[0] ) return false;

    float sampleshift = time_/(zFactor()*refstep);
    const int sampleidx = mNINT(sampleshift);
    const bool dointerpolate = dosteer_ || 
			       !mIsEqual(sampleshift,sampleidx,0.001);
    ValueSeriesInterpolator<float> interp( inputdata_->nrsamples_-1 );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	if ( dosteer_ && steeringdata_->series(steeridx_) )
	{
	    const int validx = idx + z0 - steeringdata_->z0_;
	    sampleshift = steeringdata_->series(steeridx_)->value( validx );
	}

	const ValueSeries<float>* curdata = inputdata_->series( dataidx_ );
	const int cursample = z0 - inputdata_->z0_ + idx;
	const float val = dointerpolate 
			    ? interp.value( *curdata, cursample+sampleshift ) 
			    : curdata->value( cursample+sampleidx );

	output.series(0)->setValue( z0-output.z0_+idx, val );
    }

    return true;
}


const BinID* Shift::reqStepout( int inp, int out ) const
{ return inp==1 ? 0 : &stepout_; }


const Interval<float>* Shift::reqZMargin( int inp, int ) const
{ 
   if ( inp==1 )
	return 0;
   
   bool chgstart = mNINT(interval_.start*zFactor()) % mNINT(refstep*zFactor());
   bool chgstop = mNINT(interval_.stop*zFactor()) % mNINT(refstep*zFactor());

    if ( chgstart )
    {
	int minstart = (int)(interval_.start / refstep);
	const_cast<Shift*>(this)->interval_.start = (minstart-1) * refstep;
    }
    if ( chgstop )
    {
	int minstop = (int)(interval_.stop / refstep);
	const_cast<Shift*>(this)->interval_.stop = (minstop+1) * refstep;
    }
    
    return &interval_;
}

}; // namespace Attrib
