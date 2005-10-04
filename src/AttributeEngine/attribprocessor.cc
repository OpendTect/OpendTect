/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribprocessor.cc,v 1.21 2005-10-04 13:31:16 cvshelene Exp $";

#include "attribprocessor.h"

#include "attribdesc.h"
#include "attriboutput.h"
#include "attribprovider.h"
#include "cubesampling.h"
#include "linekey.h"
#include "seisinfo.h"
#include "seistrcsel.h"


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
	provider->computeRefZStep( provider->allexistingprov );
	provider->propagateZRefStep( provider->allexistingprov );
    }
}


Processor::~Processor()
{
    if ( provider )  { provider->unRef(); desc_.unRef(); }
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

    const int res = provider->moveToNextTrace();
    if ( !nriter )
    {
	errmsg = provider->errMsg().buf();
	if ( errmsg.size() ) return ErrorOccurred;
    }

    const SeisTrcInfo* curtrcinfo = provider->getCurrentTrcInfo();
    const bool needsinput = !provider->getDesc().isStored() && 
			    provider->getDesc().nrInputs();
    if ( !curtrcinfo && needsinput && !res )
    {
	if ( !res )
	{
	    errmsg = "no position to process\n";
	    errmsg += "you may not be in the possible volume\n";
	    errmsg += "mind the stepout...";
	}
	else
	    errmsg = "no Trace info available";

	return ErrorOccurred;
    }

    if ( res )
    {
	BinID curbid = provider->getCurrentPosition();
	if ( is2d_ && curtrcinfo )
	{
	    curbid.inl = 0;
	    curbid.crl = curtrcinfo->nr;
	}
	
	if ( !curtrcinfo )
	{
	    SeisTrcInfo trcinfo;
	    trcinfo.binid = curbid;
	    if ( is2d_ ) trcinfo.nr = curbid.crl;

	    curtrcinfo = &trcinfo;
	}

	TypeSet< Interval<int> > localintervals;
	bool isset = setZIntervals( localintervals, curbid );

	for ( int idi=0; idi<localintervals.size(); idi++ )
	{
	    const DataHolder* data = isset ? 
				    provider->getData( BinID(0,0), idi ) : 0;
	    if ( data )
	    {
		for ( int idx=0; idx<outputs.size(); idx++ )
		    outputs[idx]->collectData( *data, provider->getRefStep(),
			    		       *curtrcinfo, outputindex_ );
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


void Processor::init()
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


bool Processor::setZIntervals( TypeSet< Interval<int> >& localintervals, 
			       BinID curbid )
{
    //TODO: Smarter way if output's intervals don't intersect
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

    return isset;
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
