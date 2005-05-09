/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribprocessor.cc,v 1.3 2005-05-09 14:40:41 cvshelene Exp $";

#include "attribprocessor.h"

#include "attribdesc.h"
#include "attriboutput.h"
#include "attribprovider.h"
#include "cubesampling.h"


namespace Attrib
{

Processor::Processor( Desc& desc , const char* lk )
    : Executor("Attribute Processor")
    , desc_( desc )
    , provider( Provider::create(desc) )
    , nriter( 0 )
    , lk_ (lk)
    , moveonly(this)
{
    if ( !provider ) return;
    provider->ref();
    desc_.ref();

    if ( desc_.selectedOutput()!=-1 )
	provider->enableOutput( desc_.selectedOutput(), true );

    is2d_ = strcmp( (const char*)lk_,"" );
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
    }

    const int res = provider->moveToNextTrace();
    if ( res )
    {
	const BinID curbid = provider->getCurrentPosition();

	//TODO: Smarter way if output's intervals don't intersect
	Interval<int> localinterval;
	bool isset = false;
	for ( int idx=0; idx<outputs.size(); idx++ )
	{
	    if ( !outputs[idx]->wantsOutput(curbid) ) continue;

	    if ( isset )
		localinterval.include(outputs[idx]->getLocalZRange(curbid));
	    else
	    {
		localinterval = outputs[idx]->getLocalZRange(curbid);
		isset = true;
	    }
	}

	if ( isset ) provider->addLocalCompZInterval(localinterval);
	const DataHolder* data = isset ? provider->getData() : 0;
	if ( data )
	{
	    for ( int idx=0; idx<outputs.size(); idx++ )
	    {
		outputs[idx]->setReqs(curbid);
		outputs[idx]->collectData( curbid, *data,
					    provider->getRefStep(),
					    provider->getTraceNr() );
	    }
	}
    }

    nriter++;
    return res;
}

int Processor::totalNr()
{
    return provider->getTotalNrPos(is2d_);
}

}; //namespace
