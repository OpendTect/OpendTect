/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribprocessor.cc,v 1.15 2005-08-19 14:24:35 cvsnanne Exp $";

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
    , outputindex_(0)
{
    if ( !provider ) return;
    provider->ref();
    desc_.ref();

    is2d_ = lk_ != "";
    if ( is2d_ )
    {
	provider->setCurLineKey( lk_ );
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


void Processor::addOutput( Output* output )
{
    if ( !output ) return;
    output->ref();
    outputs += output;
}


int Processor::nextStep()
{
    if ( !provider || !outputs.size() ) return ErrorOccurred;

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

	    if ( !idx )
		globalcs = cs;
	    else
	    {
		globalcs.hrg.include(cs.hrg.start);
		globalcs.hrg.include(cs.hrg.stop);
		globalcs.zrg.include(cs.zrg);
	    }

	    for ( int idy=0; idy<outpinterest_.size(); idy++ )
	    {
		if ( globaloutputinterest.indexOf(outpinterest_[idy])==-1 )
		    globaloutputinterest += outpinterest_[idy];
	    }
	    outputs[idx]->setDesiredOutputs( outpinterest_ );
	}

	SeisSelData sd;
	if ( outputs[0]->getSelData().type_==Seis::Table )
	{
	    for ( int idx=0; idx<outputs.size(); idx++ )
	    {
		if ( !idx ) sd = outputs[0]->getSelData();
		else sd.include( outputs[idx]->getSelData() );
	    }
	}

	if ( sd.type_ == Seis::Table )
	    provider->setSelData( sd );

	for ( int idx=0; idx<globaloutputinterest.size(); idx++ )
	    provider->enableOutput(globaloutputinterest[idx], true );

	provider->setDesiredVolume( globalcs );
	if ( !provider->getInputs().size() && !provider->getDesc().isStored() )
	    provider->setPossibleVolume( globalcs );
	else
	    provider->getPossibleVolume( -1, globalcs );
    }

    const int res = provider->moveToNextTrace();
    if ( !nriter )
    {
	errmsg = provider->errMsg().buf();
	if ( errmsg.size() )
	    return -1;
    }

    if ( res )
    {
	BinID curbid = provider->getCurrentPosition();
	if ( is2d_ )
	{
	    curbid.inl = 0;
	    curbid.crl = provider->getCurrentTrcNr();
	}
	
	//TODO: Smarter way if output's intervals don't intersect
	TypeSet< Interval<int> > localintervals;
	bool isset = false;
	for ( int idx=0; idx<outputs.size(); idx++ )
	{
	    if ( !outputs[idx]->wantsOutput(curbid) ) 
		continue;

	    //just assume that intervals.size() will be != 0 only in
	    //case of arbitrary shapes which will require only 
	    //one output.
	    if ( isset )
		localintervals[0].include(
				outputs[idx]->getLocalZRange(curbid)[0]);
	    else
	    {
		localintervals = outputs[idx]->getLocalZRange(curbid);
		isset = true;
	    }
	}

	if ( isset ) 
	    provider->addLocalCompZIntervals( localintervals );

	for ( int idi=0; idi<localintervals.size(); idi++ )
	{
	    const DataHolder* data = isset ? 
				    provider->getData( BinID(0,0), idi ) : 0;
	    if ( data )
	    {
		for ( int idx=0; idx<outputs.size(); idx++ )
		{
		    outputs[idx]->setReqs(curbid);
		    if ( is2d_ )
			curbid = provider->getCurrentPosition();
		    outputs[idx]->collectData( curbid, *data,
						provider->getRefStep(),
						provider->getCurrentTrcNr(),
						outputindex_ );
		}
	    }
	}

	if ( provider->getPossibleVolume()->hrg.includes(curbid) )
	    nrdone++;
    }

    provider->resetMoved();
    provider->resetZIntervals();
    nriter++;
    return res;
}


int Processor::totalNr() const
{
    return provider ? provider->getTotalNrPos(is2d_) : 0;
}


const char* Processor::getAttribName()
{
    return desc_.attribName();
}


void Processor::setOutputIndex( int& index )
{
    outputindex_ = index;
    index += outpinterest_.size();
}

}; // namespace Attrib
