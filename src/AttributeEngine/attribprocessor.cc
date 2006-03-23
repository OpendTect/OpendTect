/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribprocessor.cc,v 1.40 2006-03-23 12:47:17 cvshelene Exp $";

#include "attribprocessor.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attriboutput.h"
#include "attribprovider.h"
#include "cubesampling.h"
#include "linekey.h"
#include "seisinfo.h"
#include "seistrcsel.h"


namespace Attrib
{

Processor::Processor( Desc& desc , const char* lk, BufferString& err )
    : Executor("Attribute Processor")
    , desc_(desc)
    , provider(Provider::create(desc,err))
    , nriter(0)
    , nrdone(0)
    , isinited(false)
    , moveonly(this)
    , prevbid_(BinID(-1,-1))
    , sd_(0)
{
    if ( !provider ) return;
    provider->ref();
    desc_.ref();

    is2d_ = desc_.descSet()->is2D();
    if ( is2d_ )
    {
	provider->setCurLineKey( lk );
	provider->adjust2DLineStoredVolume( true );
    }
}


Processor::~Processor()
{
    if ( provider )  { provider->unRef(); desc_.unRef(); }
    deepUnRef( outputs );

    if (sd_) delete sd_;
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

    if ( !isinited )
	init();

    int res;
    res = provider->moveToNextTrace();
    
    if ( res < 0 || !nriter )
    {
	errmsg = provider->errMsg().buf();
	if ( errmsg.size() )
	    return ErrorOccurred;
	else if ( res < 0 )
	{
	    BinID firstpos;

	    if ( sd_ && sd_->type_ == Seis::Table )
		firstpos = sd_->table_.firstPos();
	    else
	    {
		const BinID step = provider->getStepoutStep();
		firstpos.inl = step.inl/abs(step.inl)>0 ? 
			       provider->getDesiredVolume()->hrg.start.inl : 
			       provider->getDesiredVolume()->hrg.stop.inl;
		firstpos.crl = step.crl/abs(step.crl)>0 ?
			       provider->getDesiredVolume()->hrg.start.crl :
			       provider->getDesiredVolume()->hrg.stop.crl;
	    }
	    provider->resetMoved();
	    res = provider->moveToNextTrace( firstpos, true );
	    
	    if ( res < 0 )
	    {
		errmsg = "Error during data read";
		return ErrorOccurred;
	    }
	}
    }

    provider->updateCurrentInfo();
    const SeisTrcInfo* curtrcinfo = provider->getCurrentTrcInfo();
    const bool needsinput = !provider->getDesc().isStored() && 
			    provider->getDesc().nrInputs();
    if ( !curtrcinfo && needsinput )
    {
	errmsg = "No trace info available";
	return ErrorOccurred;
    }

    if ( res == 0 && !nrdone )
    {
	errmsg = "No position to process.\n";
	errmsg += "You may not be in the possible volume,\n";
	errmsg += "mind the stepout...";
	return ErrorOccurred;
    }
    else if ( res != 0 )
    {
	BinID curbid = provider->getCurrentPosition();
	if ( is2d_ && curtrcinfo )
	{
	    mDynamicCastGet( LocationOutput*, locoutp, outputs[0] );
	    if ( locoutp ) 
		curbid = curtrcinfo->binid;
	    else
	    {
		curbid.inl = 0;
		curbid.crl = curtrcinfo->nr;
	    }
	}

	SeisTrcInfo mytrcinfo;
	if ( !curtrcinfo )
	{
	    mytrcinfo.binid = curbid;
	    if ( is2d_ ) mytrcinfo.nr = curbid.crl;

	    curtrcinfo = &mytrcinfo;
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
			    		       *curtrcinfo );
		if ( isset )
		    nrdone++;
	    }
	}

	prevbid_ = curbid;
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

    if ( outputs.size() && outputs[0]->getSelData().type_==Seis::Table )
    {
	for ( int idx=0; idx<outputs.size(); idx++ )
	{
	    if ( !idx ) sd_ = new SeisSelData(outputs[0]->getSelData());
	    else sd_->include( outputs[idx]->getSelData() );
	}
    }

    if ( sd_ && sd_->type_ == Seis::Table )
    {
	provider->setSelData( sd_ );
	mDynamicCastGet( LocationOutput*, locoutp, outputs[0] );
	if ( locoutp )
	{
	    Interval<float> extraz( -2*provider->getRefStep(), 
		    		    2*provider->getRefStep() );
	    provider->setExtraZ( extraz );
	    provider->setNeedInterpol(true);
	}
    }

    for ( int idx=0; idx<globaloutputinterest.size(); idx++ )
	provider->enableOutput(globaloutputinterest[idx], true );

    if ( !provider->getInputs().size() && !provider->getDesc().isStored() )
    {
	provider->setDesiredVolume( globalcs );
	provider->setPossibleVolume( globalcs );
    }
    else
    {
	CubeSampling possvol;
	if ( !possvol.includes(globalcs) )
	    possvol = globalcs;
	provider->setDesiredVolume( possvol );
	provider->getPossibleVolume( -1, possvol );
	provider->resetDesiredVolume();

#       define mAdjustIf(v1,op,v2) \
	if ( !mIsUdf(v1) && !mIsUdf(v2) && v1 op v2 ) v1 = v2;
	
	mAdjustIf(globalcs.hrg.start.inl,<,possvol.hrg.start.inl);
	mAdjustIf(globalcs.hrg.start.crl,<,possvol.hrg.start.crl);
	mAdjustIf(globalcs.zrg.start,<,possvol.zrg.start);
	mAdjustIf(globalcs.hrg.stop.inl,>,possvol.hrg.stop.inl);
	mAdjustIf(globalcs.hrg.stop.crl,>,possvol.hrg.stop.crl);
	mAdjustIf(globalcs.zrg.stop,>,possvol.zrg.stop);
	
	provider->setDesiredVolume( globalcs );
    }

    isinited = true;
}


bool Processor::setZIntervals( TypeSet< Interval<int> >& localintervals, 
			       BinID curbid )
{
    //TODO: Smarter way if output's intervals don't intersect
    bool isset = false;
    for ( int idx=0; idx<outputs.size(); idx++ )
    {
	if ( !outputs[idx]->wantsOutput(curbid) || curbid == prevbid_ ) 
	    continue;

	//just assume that intervals.size() will be != 0 only in
	//case of arbitrary shapes or picks which will require only 
	//one output.
	if ( isset )
	    localintervals[0].include( outputs[idx]->
		    	getLocalZRange( curbid, provider->getRefStep() )[0] );
	else
	{
	    localintervals = outputs[idx]->
			    getLocalZRange( curbid, provider->getRefStep() );
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


}; // namespace Attrib
