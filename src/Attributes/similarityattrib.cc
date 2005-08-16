/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          June 2005
 RCS:           $Id: similarityattrib.cc,v 1.9 2005-08-16 12:40:31 cvshelene Exp $
________________________________________________________________________

-*/

#include "similarityattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "datainpspec.h"
#include "genericnumer.h"
#include "survinfo.h"
#include "runstat.h"

#define mExtensionNone		0
#define mExtensionRot90		1
#define mExtensionRot180	2
#define mExtensionCube		3

namespace Attrib
{

void Similarity::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    ZGateParam* gate = new ZGateParam(gateStr());
    gate->setLimits( Interval<float>(-1000,1000) );
    desc->addParam( gate );

    desc->addParam( new BinIDParam(pos0Str()) );
    desc->addParam( new BinIDParam(pos1Str()) );

    BinIDParam* stepout = new BinIDParam( stepoutStr() );
    stepout->setDefaultValue( BinID(1,1) );
    desc->addParam( stepout );

    //Note: Ordering must be the same as numbering!
    EnumParam* extension = new EnumParam( extensionStr() );
    extension->addEnum( extensionTypeStr(mExtensionNone) );
    extension->addEnum( extensionTypeStr(mExtensionRot90) );
    extension->addEnum( extensionTypeStr(mExtensionRot180) );
    extension->addEnum( extensionTypeStr(mExtensionCube) );
    extension->setDefaultValue( extensionTypeStr(mExtensionNone) );
    desc->addParam( extension );

    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue(true);
    desc->addParam( steering );

    BoolParam* normalize = new BoolParam( normalizeStr() );
    normalize->setDefaultValue(false);
    desc->addParam( normalize );

    desc->setNrOutputs( Seis::UnknowData, 5 );

    desc->addInput( InputSpec("Input data",true) );

    InputSpec steeringspec( "Steering data", false );
    steeringspec.issteering = true;
    desc->addInput( steeringspec );

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Similarity::createInstance( Desc& desc )
{
    Similarity* res = new Similarity( desc );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void Similarity::updateDesc( Desc& desc )
{
    BufferString extstr = desc.getValParam(extensionStr())->getStringValue();
    const bool iscube = extstr == extensionTypeStr(mExtensionCube);
    desc.setParamEnabled( pos0Str(), !iscube );
    desc.setParamEnabled( pos1Str(), !iscube );
    desc.setParamEnabled( stepoutStr(), iscube );
}


const char* Similarity::extensionTypeStr( int type )
{
    if ( type==mExtensionNone ) return "None";
    if ( type==mExtensionRot90 ) return "90";
    if ( type==mExtensionRot180 ) return "180";
    return "Cube";
}


Similarity::Similarity( Desc& desc_ )
    : Provider( desc_ )
{
    if ( !isOK() ) return;

    inputdata.allowNull(true);

    mGetFloatInterval( gate, gateStr() );
    gate.start /= zFactor();
    gate.stop /= zFactor();

    mGetBool( dosteer, steeringStr() );
    mGetBool( donormalize, normalizeStr() );

    mGetEnum( extension, extensionStr() );
    if ( extension==mExtensionCube )
	mGetBinID( stepout, stepoutStr() )
    else
    {
	mGetBinID( pos0, pos0Str() )
	mGetBinID( pos1, pos1Str() )

	stepout = BinID(mMAX(abs(pos0.inl),abs(pos1.inl)),
			mMAX(abs(pos0.crl),abs(pos1.crl)) );
    }
}


bool Similarity::init()
{
    trcpos.erase();
    if ( extension==mExtensionCube )
    {
	BinID bid;
	for ( bid.inl=-stepout.inl; bid.inl<=stepout.inl; bid.inl++ )
	    for ( bid.crl=-stepout.crl; bid.crl<=stepout.crl; bid.crl++ )
		trcpos += bid;
    }
    else
    {
	trcpos += pos0;
	trcpos += pos1;

	if ( extension==mExtensionRot90 )
	{
	    trcpos += BinID(pos0.crl,-pos0.inl);
	    trcpos += BinID(pos1.crl,-pos1.inl);
	}
	else if ( extension==mExtensionRot180 )
	{
	    trcpos += BinID(-pos0.inl,-pos0.crl);
	    trcpos += BinID(-pos1.inl,-pos1.crl);
	}
    }

    return true;
}


bool Similarity::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !dosteer || !input ) return Provider::getInputOutput( input, res );

    for ( int idx=0; idx<trcpos.size(); idx++ )
	res += getSteeringIndex( trcpos[idx] );

    return true;
}


bool Similarity::getInputData( const BinID& relpos, int index )
{
    while ( inputdata.size() < trcpos.size() )
	inputdata += 0;

    const BinID bidstep = inputs[0]->getStepoutStep();
    for ( int idx=0; idx<trcpos.size(); idx++ )
    {
	const DataHolder* data = 
		    inputs[0]->getData( relpos+trcpos[idx]*bidstep, index );
	if ( !data ) return false;
	inputdata.replace( idx, data );
	if ( dosteer )
	    steeridx += getSteeringIndex( trcpos[idx] );
    }

    steeringdata = dosteer ? inputs[1]->getData(relpos,index) : 0;

    return true;
}


bool Similarity::computeData( const DataHolder& output, const BinID& relpos, 
			      int t0, int nrsamples ) const
{
    if ( !inputdata.size() ) return false;

    Interval<int> samplegate( mNINT(gate.start/refstep),
	                      mNINT(gate.stop/refstep) );

    const int gatesz = samplegate.width();
    const int nrpairs = inputdata.size()/2;
    int firstsample = inputdata[0] ? t0 -inputdata[0]->t0_ : t0;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	RunningStatistics<float> stats;
	for ( int pair=0; pair<nrpairs; pair++ )
	{
	    int idx1 = extension==mExtensionCube ? pair : pair*2;
	    int idx2 = extension==mExtensionCube ? 
				inputdata.size() - (pair+1) : pair*2 +1;
	    float s0 = firstsample + idx + samplegate.start;
	    float s1 = s0;

	     if ( !inputdata[idx1] || ! inputdata[idx2])
		 continue;
	     
	     if ( dosteer )
	     {
		 ValueSeries<float>* item1 = steeringdata->item(steeridx[idx1]);
	         if ( item1 ) s0 += item1->value( idx-steeringdata->t0_ );

		 ValueSeries<float>* item2 = steeringdata->item(steeridx[idx2]);
		 if ( item2 ) s1 += item2->value( idx-steeringdata->t0_ );
	     }

	     SimiFunc vals0( *(inputdata[idx1]->item(0)) );
	     SimiFunc vals1( *(inputdata[idx2]->item(0)) );
	     stats += similarity(vals0, vals1, s0, s1, 1, gatesz, donormalize);
	}

	if ( !stats.size() )
	{
	    for (int id=0; id<outputinterest.size(); id++ )
	    {
		if ( outputinterest[id] ) 
		    output.item(id)->setValue(idx,0);
	    }
	}
	else
	{
	    if ( outputinterest[0] ) output.item(0)->setValue(idx,stats.mean());

	    if ( nrpairs>1 && outputinterest.size()>1 )
	    {
		if ( outputinterest[1] ) 
		    output.item(1)->setValue(idx,stats.median());
		if ( outputinterest[2] ) 
		    output.item(2)->setValue(idx,stats.variance());
		if ( outputinterest[3] ) 
		    output.item(3)->setValue(idx,stats.min());
		if ( outputinterest[4] ) 
		    output.item(4)->setValue(idx,stats.max());
	    }
	}
    }

    return true;
}


const BinID* Similarity::reqStepout( int inp, int out ) const
{ return inp ? 0 : &stepout; }


const Interval<float>* Similarity::reqZMargin( int inp, int ) const
{ return inp ? 0 : &gate; }






}; //namespace

