/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: volstatsattrib.cc,v 1.13 2005-11-29 19:35:42 cvsnanne Exp $";

#include "volstatsattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "cubesampling.h"
#include "valseriesinterpol.h"

#define mShapeRectangle	 0
#define mShapeEllipse	 1

namespace Attrib
{

void VolStats::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    IntParam* nrvolumes = new IntParam( nrvolumesStr() );
    nrvolumes->setLimits( Interval<int>(1,INT_MAX) );
    nrvolumes->setDefaultValue("1");
    nrvolumes->setValue( (int)1 );
    nrvolumes->setRequired(false);
    desc->addParam( nrvolumes );

    BinIDParam* stepout = new BinIDParam(stepoutStr());
    stepout->setDefaultValue(BinID(1,1));
    desc->addParam( stepout );

    EnumParam* shape = new EnumParam(shapeStr());
    //Note: Ordering must be the same as numbering!
    shape->addEnum(shapeTypeStr(mShapeRectangle));
    shape->addEnum(shapeTypeStr(mShapeEllipse));
    shape->setDefaultValue("0");
    desc->addParam(shape);

    ZGateParam* gate = new ZGateParam(gateStr());
    gate->setLimits( Interval<float>(-mLargestZGate,mLargestZGate) );
    desc->addParam( gate );

    BoolParam* absolutegate = new BoolParam( absolutegateStr() );
    absolutegate->setDefaultValue(false);
    absolutegate->setValue( false );
    absolutegate->setRequired( false );
    desc->addParam( absolutegate );
    
    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue(false);
    desc->addParam( steering );

    int res =0;
    while ( outputtypes[res++] != -1 )
	desc->addOutputDataType( Seis::UnknowData );

    desc->addInput( InputSpec("Input data",true) );

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* VolStats::createInstance( Desc& ds )
{
    VolStats* res = new VolStats( ds );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void VolStats::updateDesc( Desc& desc )
{
    if ( desc.nrInputs() == 1 )
    {
	const int nrvolumes = desc.getValParam(nrvolumesStr())->getIntValue();
	for ( int idx=1; idx<nrvolumes; idx++ )
	{
	    BufferString str = "inputdata vol"; str += (idx+1);
	    desc.addInput( InputSpec(str.buf(),true) );
	}
	
	const bool issteer = desc.getValParam(steeringStr())->getBoolValue();
	if ( issteer )
	{
	    InputSpec steeringspec( "Steering data", true );
	    steeringspec.issteering = true;
	    desc.addInput( steeringspec );
	}
    }
}


const char* VolStats::shapeTypeStr(int type)
{
    if ( type==mShapeRectangle ) return "Rectangle";
    return "Ellipse";
}


int VolStats::outputtypes[] =
{
    RunStats::Mean,
    RunStats::Median,
    RunStats::Variance,
    RunStats::Min,
    RunStats::Max,
    RunStats::Sum,
    RunStats::NormVariance,
    -1
};

    
VolStats::VolStats( Desc& desc_ )
    : Provider( desc_ )
    , positions(0,BinID(0,0))
    , stats(0)
{
    if ( !isOK() ) return;

    inputdata.allowNull(true);
    
    mGetInt( nrvolumes, nrvolumesStr() );
    mGetBinID( stepout, stepoutStr() );
    mGetEnum( shape, shapeStr() );
    mGetFloatInterval( gate, gateStr() );
    gate.start /= zFactor(); gate.stop /= zFactor();

    float extraz = mMAX(stepout.inl*inldist(), stepout.crl*crldist()) * mMAXDIP;
    desgate = Interval<float>( gate.start - extraz, gate.stop + extraz );
    
    mGetBool( absolutegate, absolutegateStr() );
    mGetBool( steering, steeringStr() );
    
    BinID pos;
    for ( pos.inl=-stepout.inl; pos.inl<=stepout.inl; pos.inl++ )
    {
	for ( pos.crl=-stepout.crl; pos.crl<=stepout.crl; pos.crl++ )
	{
	    float relinldist = stepout.inl ? ((float)pos.inl)/stepout.inl : 0;
	    float relcrldist = stepout.crl ? ((float)pos.crl)/stepout.crl : 0;

	    float dist2 = relinldist*relinldist+relcrldist*relcrldist;

	    if ( shape==mShapeEllipse && dist2>1 )
		continue;

	    positions += pos;
	}
    }

    stats = new ObjectSet< RunningStatistics<double> >;
    for ( int idx=0; idx<nrvolumes; idx++ )
	(*stats) += new RunningStatistics<double>;
}


VolStats::~VolStats()
{
    deepErase( *stats );
    delete stats;
}


void VolStats::initSteering()
{
    for( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( !inputs[idx] )
	    continue;

	if ( inputs[idx]->getDesc().isSteering() )
	    inputs[idx]->initSteering( stepout );
    }
}


bool VolStats::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !steering || input<inputs.size()-1 ) 
	return Provider::getInputOutput( input, res );

    for ( int inl=-stepout.inl; inl<=stepout.inl; inl++ )
    {
	for ( int crl=-stepout.crl; crl<=stepout.crl; crl++ )
	    res += getSteeringIndex( BinID(inl,crl) );
    }
    return true;
}


bool VolStats::getInputData( const BinID& relpos, int idx )
{
    while ( inputdata.size() < positions.size()*nrvolumes )
	inputdata += 0;

    steeringdata = steering ? inputs[nrvolumes]->getData(relpos,idx) : 0;
    if ( steering && !steeringdata )
	return false;

    const BinID bidstep = inputs[0]->getStepoutStep();
    const int nrpos = positions.size();

    int storpos = 0;
    for ( int idy=0; idy<nrvolumes; idy++ )
    {
	for ( int idz=0; idz<nrpos; idz++ )
	{
	    const BinID truepos = relpos + positions[idz] * bidstep;
	    const DataHolder* indata = inputs[idy]->getData( truepos, idx );
	    if ( !indata ) return false;

	    inputdata.replace( storpos, indata );
	    storpos++;
	}
    }

    dataidx_ = getDataIndex( 0 );
    return true;
}


const BinID* VolStats::reqStepout( int inp, int out ) const
{ return inp<nrvolumes ? &stepout : 0; }


const Interval<float>* VolStats::reqZMargin( int inp, int ) const
{ return !absolutegate && inp<nrvolumes ? &gate : 0; }


const Interval<float>* VolStats::desZMargin( int inp, int ) const
{ 
    if ( absolutegate )
    {
	const int maxlen = mNINT(possiblevolume->zrg.width()/refstep) + 1;
	const Interval<float> absgate( -maxlen*refstep, maxlen*refstep );
	const_cast<VolStats*>(this)->absdepthgate = absgate;
    }

    return  absolutegate ? &absdepthgate : (inp<nrvolumes ? &desgate : 0);
}


bool VolStats::computeData( const DataHolder& output, const BinID& relpos,
			    int z0, int nrsamples ) const
{
    const int nrpos = positions.size();
    const int nrtraces = nrvolumes*nrpos;

    const Interval<int> samplegate( mNINT(gate.start/refstep), 
				    mNINT(gate.stop/refstep) );
    int sg = samplegate.width() + 1;

    for ( int idx=0; idx<nrsamples; idx++)
    {
	int cursample = z0 + idx;
	if ( !idx )
	{
	    for ( int vol=0; vol<nrvolumes; vol++ )
		(*stats)[vol]->clear();

	    for ( int posidx=0; posidx<nrpos; posidx++ )
	    {
		int steeridx = steering? getSteeringIndex(positions[posidx]) :0;

		float csample = cursample;
		if ( steering ) csample += steeringdata->series(steeridx)->
		    			value( cursample - steeringdata->z0_); 

		for ( int volidx=0; volidx<nrvolumes; volidx++ )
		{
		    const DataHolder* dh = inputdata[volidx*nrpos+posidx];
		    if ( !dh ) continue;

		    int s = samplegate.start;
		    ValueSeriesInterpolator<float> interp( dh->nrsamples_-1 );
		    for ( int idz=0; idz<sg; idz++ )
		    {
			float place = (absolutegate ? 0 : csample) + s;
			(*(*stats)[volidx]) += interp.value( 
					*(dh->series(dataidx_)),place-dh->z0_ );
			s++;
		    }
		}	
	    }

	    if ( !idx )
	    {
		for ( int vol=0; vol<nrvolumes; vol++ )
		    (*stats)[vol]->lock();
	    }
	}
	else if ( !absolutegate )
	{
	    for ( int posidx=0; posidx<nrpos; posidx++ )
	    {
		float csample = cursample;
		int steeridx = steering? getSteeringIndex(positions[posidx]) :0;
		
		if ( steering ) csample += steeringdata->series(steeridx)->
					value( cursample-steeringdata->z0_ ); 

		const float newsample = csample + samplegate.stop;

		for ( int volidx=0; volidx<nrvolumes; volidx++ )
		{
		    const DataHolder* dh = inputdata[volidx*nrpos+posidx];
		    if ( !dh ) continue;
					
		    ValueSeriesInterpolator<float> interp( dh->nrsamples_-1 );

		    float place = newsample - dh->z0_;
		    (*(*stats)[volidx]) += 
			interp.value( *(dh->series(dataidx_)), place);
		}
	    }
	}

        const int nroutp = outputinterest.size();
	for ( int outidx=0; outidx<nroutp; outidx++ )
	{
	    if ( outputinterest[outidx] == 0 )
		continue;

	    bool validsum = false;
	    float sum = 0;
	    for ( int vol=0; vol<nrvolumes; vol++ )
	    {
		RunningStatistics<double>* curstat = (*stats)[vol];
		if ( !curstat || !curstat->size() ) continue;
		sum += (*stats)[vol]->getValue(
			(RunStats::StatType)outputtypes[outidx] );
		validsum = true;
	    }

	    const int sampleidx = output.z0_-z0+idx;
	    const float outval = validsum ? sum / nrvolumes : mUdf(float);
	    output.series(outidx)->setValue( sampleidx, outval );
	}
    }
    
    return true;
}



}; //namespace
