/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribprocessor.cc,v 1.11 2005-08-01 08:54:34 cvshelene Exp $";

#include "attribprocessor.h"

#include "attribdesc.h"
#include "attriboutput.h"
#include "attribprovider.h"
#include "cubesampling.h"
#include "linekey.h"


namespace Attrib
{

Processor::Processor( Desc& desc , const char* lk )
    : Executor("Attribute Processor")
    , desc_(desc)
    , provider(Provider::create(desc))
    , nriter(0)
    , nrdone(0)
    , lk_(lk)
    , moveonly(this)
{
    if ( !provider ) return;
    provider->ref();
    desc_.ref();

    is2d_ = strcmp( (const char*)lk_,"" );
    if ( is2d_ )
    {
	provider->setCurLineKey( (const char*)lk_ );
	provider->adjust2DLineStoredVolume();
    }
}


Processor::~Processor()
{
    desc_.unRef();
    if ( provider ) provider->unRef();
    deepUnRef( outputs );
}


bool Processor::isOK() const { return provider; }


void Processor::addOutput( Output* o )
{
    o->ref();
    outputs += o;
}


int Processor::nextStep()
{    
    if ( !nriter )
    {
	TypeSet<int> globaloutputinterest;
	CubeSampling globalcs;
	for ( int idx=0; idx<outputs.size(); idx++ )
	{
	    provider->setSelData(outputs[idx]->getSelData());

	    CubeSampling cs;
	    if ( !outputs[idx]->getDesiredVolume(cs) )
	    {
		outputs[idx]->unRef();
		outputs.remove(idx);
		idx--;
		continue;
	    }

	    if ( !idx ) globalcs = cs;
	    else
	    {
		globalcs.hrg.include(cs.hrg.start);
		globalcs.hrg.include(cs.hrg.stop);
		globalcs.zrg.include(cs.zrg);
	    }

	    for ( int idy=0; idy<outpinterest.size(); idy++ )
	    {
		if ( globaloutputinterest.indexOf(outpinterest[idy])==-1 )
		    globaloutputinterest += outpinterest[idy];
	    }
	    outputs[idx]->setDesiredOutputs( outpinterest );
	}

	if ( !outputs.size() )
	    return 0;

	for ( int idx=0; idx<globaloutputinterest.size(); idx++ )
	    provider->enableOutput(globaloutputinterest[idx], true );

	provider->setDesiredVolume( globalcs );
	if ( !provider->getInputs().size() && !provider->getDesc().isStored() )
	    provider->setPossibleVolume(globalcs);
	else
	    provider->getPossibleVolume( -1, globalcs );

    }

    const int res = provider->moveToNextTrace();
    if ( res )
    {
	BinID curbid = provider->getCurrentPosition();
	if ( is2d_ )
	{
	    curbid.inl = 0;
	    curbid.crl = provider-> getCurrentTrcNr();
	}
	
	//TODO: Smarter way if output's intervals don't intersect
	TypeSet< Interval<int> > localintervals;
	bool isset = false;
	for ( int idx=0; idx<outputs.size(); idx++ )
	{
	    if ( !outputs[idx]->wantsOutput(curbid) ) 
		continue;

	    if ( isset )//just assume that intervals.size() will be != 0 only in 			//case of arbitrary shapes which will require only 
			//one output.
		localintervals[0].include(
				outputs[idx]->getLocalZRange(curbid)[0]);
	    else
	    {
		localintervals = outputs[idx]->getLocalZRange(curbid);
		isset = true;
	    }
	}

	if ( isset ) provider->addLocalCompZIntervals(localintervals);
	for ( int idi=0; idi<localintervals.size(); idi++ )
	{
	    const DataHolder* data = isset ? 
				    provider->getData(BinID(0,0), idi): 0;
	    if ( data )
	    {
		for ( int idx=0; idx<outputs.size(); idx++ )
		{
		    outputs[idx]->setReqs(curbid);
		    if ( is2d_ )
			curbid = provider->getCurrentPosition();
		    outputs[idx]->collectData( curbid, *data,
						provider->getRefStep(),
						provider->getCurrentTrcNr() );
		}
	    }
	}
    if ( provider->getPossibleVolume()->hrg.includes(curbid) ) nrdone++;
    }

    provider->resetMoved();
    provider->resetZIntervals();
    nriter++;
    return res;
}

int Processor::totalNr() const
{
    return provider->getTotalNrPos(is2d_);
}


const char* Processor::getAttribName()
{
    return desc_.attribName();
}

}; //namespace
