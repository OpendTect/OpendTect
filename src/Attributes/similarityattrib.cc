/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: similarityattrib.cc,v 1.4 2005-06-30 11:26:43 cvshelene Exp $";

#include "similarityattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "datainpspec.h"
#include "genericnumer.h"
#include "survinfo.h"

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
    desc->addParam( new BinIDParam(stepoutStr()) );

    EnumParam* extension = new EnumParam(extensionStr());

    //Note: Ordering must be the same as numbering!
    extension->addEnum(extensionTypeStr(mExtensionNone));
    extension->addEnum(extensionTypeStr(mExtensionRot90));
    extension->addEnum(extensionTypeStr(mExtensionRot180));
    extension->addEnum(extensionTypeStr(mExtensionCube));
    desc->addParam(extension);

    desc->addParam( new BoolParam(steeringStr()) );
    desc->addParam( new BoolParam(normalizeStr()) );

    desc->addOutputDataType( Seis::UnknowData );

    InputSpec inputspec( "Data on which the Similarity should be measured",
	    		 true );
    desc->addInput( inputspec );

    InputSpec steeringspec( "Steering data", false );
    steeringspec.issteering = true;
    desc->addInput( steeringspec );

    desc->init();

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Similarity::createInstance( Desc& ds )
{
    Similarity* res = new Similarity( ds );
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
    const ValParam* extension = (ValParam*)desc.getParam(extensionStr());
    if ( !strcmp(extension->getStringValue(0),extensionTypeStr(mExtensionCube)))
    {
	desc.setParamEnabled(pos0Str(),false);
	desc.setParamEnabled(pos1Str(),false);
	desc.setParamEnabled(stepoutStr(),true);
    }
    else
    {
	desc.setParamEnabled(pos0Str(),true);
	desc.setParamEnabled(pos1Str(),true);
	desc.setParamEnabled(stepoutStr(),false);
    }
    if ( !strcmp(extension->getStringValue(0),extensionTypeStr(mExtensionNone)))
	desc.setNrOutputs( Seis::UnknowData, 4 );
	
}

const char* Similarity::extensionTypeStr(int type)
{
    if ( type==mExtensionNone ) return "None";
    if ( type==mExtensionRot90 ) return "rot90";
    if ( type==mExtensionRot180 ) return "rot180";
    return "Cube";
}


Similarity::Similarity( Desc& desc_ )
    : Provider( desc_ )
{
    if ( !isOK() ) return;

    inputdata.allowNull(true);

    mGetFloatInterval( gate, gateStr() );
    gate.start = gate.start / zFactor(); gate.stop = gate.stop / zFactor();

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


bool Similarity::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !dosteer || !input ) return Provider::getInputOutput( input, res );

    if ( extension==mExtensionCube )
    {
	for ( int inl=-stepout.inl; inl<=stepout.inl; inl++ )
	{
	    for ( int crl=-stepout.crl; crl<=stepout.crl; crl++ )
		res += getSteeringIndex( BinID(inl,crl) );
	}
    }
    else
    {
	res += getSteeringIndex(pos0);
	res += getSteeringIndex(pos1);

	if ( extension==mExtensionRot90 )
	{
	    res += getSteeringIndex(BinID(pos0.crl,-pos0.inl));
	    res += getSteeringIndex(BinID(pos1.crl,-pos1.inl));
	}
	else if ( extension==mExtensionRot180 )
	{
	    res += getSteeringIndex( BinID(-pos0.inl,-pos0.crl) );
	    res += getSteeringIndex( BinID(-pos1.inl,-pos1.crl) );
	}
    }

    return true;
}


bool Similarity::getInputData(const BinID& relpos, int idx)
{
    if ( !inputdata.size() )
	inputdata += 0;

    if ( extension != mExtensionCube && inputdata.size() < 2 )
	inputdata += 0;

    steeringdata = dosteer ? inputs[1]->getData(relpos, idx) : 0;

    const BinID bidstep = inputs[0]-> getStepoutStep();
    if ( extension==mExtensionCube )
    {
	int index = 0;
	for ( BinID localpos(-stepout); localpos.inl<=stepout.inl;
	      localpos.inl+=bidstep.inl )
	{
	    for ( ; localpos.crl<=stepout.crl; localpos.crl+=bidstep.crl )
	    {
		const DataHolder* data = 
		    		inputs[0]->getData( localpos+relpos, idx );

		if ( !data )
		    return false;
		if ( inputdata.size()<index+1 )
		    inputdata += 0;
		inputdata.replace( index, data );
		if ( dosteer )
		    steeridx += getSteeringIndex(
			    	(BinID) (localpos.inl/ bidstep.inl,
					localpos.crl/ bidstep.crl) );
		index++;
	    }
	}
    }
    else
    {
	BinID truepos0; BinID truepos1;
	truepos0.inl = pos0.inl * bidstep.inl; 
	truepos1.inl = pos1.inl * bidstep.inl;
	truepos0.crl = pos0.crl * bidstep.crl; 
	truepos1.crl = pos1.crl * bidstep.crl;
	const DataHolder* p0 = inputs[0]->getData( relpos+truepos0, idx );
	if ( !p0 ) { return false; }
	inputdata.replace( 0, p0 );
	steeridx += getSteeringIndex(pos0);

	const DataHolder* p1 = inputs[0]->getData( relpos+truepos1, idx );
	if ( !p1 ) { return false; }
	inputdata.replace( 1, p1 );
	steeridx += getSteeringIndex(pos1);

	if ( extension==mExtensionRot90 )
	{
	    while ( inputdata.size() < 4 )
		inputdata += 0;
	    p0 = inputs[0]->getData( relpos+BinID(pos0.crl,-pos0.inl), idx );
	    if ( !p0 ) { return false; }
	    inputdata.replace( 2, p0 );
	    steeridx += getSteeringIndex(BinID(pos0.crl,-pos0.inl));

	    p1 = inputs[0]->getData( relpos+BinID(pos1.crl,-pos1.inl), idx );
	    if ( !p1 ) { inputdata.erase(); return false; }
	    inputdata.replace( 3, p1 );
	    steeridx += getSteeringIndex(BinID(pos1.crl,-pos1.inl));
	}
	else if ( extension==mExtensionRot180 )
	{
	    while ( inputdata.size() < 4 )
		inputdata += 0;
	    p0 = inputs[0]->getData( relpos+BinID(-pos0.inl,-pos0.crl), idx );
	    if ( !p0 ) { inputdata.erase(); return false; }
	    inputdata.replace( 2, p0 );
	    steeridx += getSteeringIndex(BinID(-pos0.inl,-pos0.crl));

	    p1 = inputs[0]->getData( relpos+BinID(-pos1.inl,-pos1.crl), idx );
	    if ( !p1 ) { inputdata.erase(); return false; }
	    inputdata.replace( 3, p1 );
	    steeridx += getSteeringIndex(BinID(-pos1.inl,-pos1.crl));
	}
    }

    return true;
}


bool Similarity::computeData( const DataHolder& output, 
				const BinID& relpos, 
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
	    float s0 = firstsample + idx + samplegate.start;
	    float s1 = s0;

	     if ( !inputdata[pair*2] || ! inputdata[pair*2 + 1])
		 continue;
	     
	     if ( dosteer )
	     {
	         if ( steeringdata->item(steeridx[pair*2]) )
		     s0 += steeringdata->item(steeridx[pair*2])->value(idx);

		 if ( steeringdata->item(steeridx[pair*2+1]) )
		     s1 += steeringdata->item(steeridx[pair*2+1])->value(idx);
	     }

	     SimiFunc vals0(*(inputdata[pair*2]->item(0)));
	     SimiFunc vals1(*(inputdata[pair*2+1]->item(0)));
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

	    if ( nrpairs>1 )
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

