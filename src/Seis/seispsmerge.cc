/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seispsmerge.h"
#include "seisresampler.h"
#include "seisselectionimpl.h"
#include "posinfo.h"
#include "seisbuf.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "ioobj.h"


SeisPSCopier::SeisPSCopier( const IOObj& in, const IOObj& out,
			    const Seis::SelData* sd )
    : SeisPSMerger( mkObjs(in), out, false, sd )
{
    setName( "Copying Prestack data" );
}


SeisPSCopier::~SeisPSCopier()
{
    delete objs_;
}


ObjectSet<const IOObj>& SeisPSCopier::mkObjs( const IOObj& in )
{
    objs_ = new ObjectSet<const IOObj>;
    *objs_ += &in;
    return *objs_;
}


SeisPSMerger::SeisPSMerger( const ObjectSet<const IOObj>& inobjs,
			    const IOObj& out, bool dostack,
			    const Seis::SelData* sd )
    : Executor("Merging Prestack data")
    , writer_(nullptr)
    , dostack_(dostack)
    , sd_(sd && !sd->isAll() ? sd->clone() : nullptr)
    , offsrg_(0,mUdf(float))
    , msg_(tr("Handling gathers"))
    , totnr_(-1)
    , nrdone_(0)
{
    TrcKeyZSampling cs; bool havecs = false;
    if ( sd_ )
    {
	mDynamicCastGet(Seis::RangeSelData*,rsd,sd_)
	if ( rsd )
	{
	    cs = rsd->cubeSampling();
	    havecs = true;
	}
	else
	{
	    Interval<float> zrg = sd_->zRange();
	    cs.zsamp_.start_ = zrg.start_;
	    cs.zsamp_.stop_ = zrg.stop_;
	}
    }

    for ( int idx=0; idx<inobjs.size(); idx++ )
    {
	SeisPS3DReader* rdr = SPSIOPF().get3DReader( *inobjs[idx] );
	if ( !rdr ) continue;
	readers_ += rdr;
	if ( havecs ) continue;

	Interval<int> inlrg, crlrg; StepInterval<int> rg;
	rdr->posData().getInlRange( rg ); assign( inlrg, rg );
	rdr->posData().getCrlRange( rg ); assign( crlrg, rg );
	if ( idx == 0 )
	    cs.hsamp_.set( inlrg, crlrg );
	else
	{
	    cs.hsamp_.include( BinID(inlrg.start_,crlrg.start_) );
	    cs.hsamp_.include( BinID(inlrg.stop_,crlrg.stop_) );
	}
    }
    if ( readers_.isEmpty() )
    { msg_ = tr("No valid inputs specified"); return; }

    totnr_ = mCast( int, sd_ ? sd_->expectedNrTraces() : cs.hsamp_.totalNr() );
    iter_ = new TrcKeySamplingIterator( cs.hsamp_ );
    resampler_ = new SeisResampler( cs );

    const Seis::GeomType gt = Seis::VolPS;
    writer_ = new SeisTrcWriter( out, &gt );
    if ( !writer_ )
    {
	deepErase(readers_);
	msg_ = toUiString("Cannot create output writer");
	return;
    }
}


SeisPSMerger::~SeisPSMerger()
{
    delete iter_;
    delete writer_;
    delete sd_;
    delete resampler_;
    deepErase( readers_ );
}


int SeisPSMerger::nextStep()
{
    if ( readers_.isEmpty() )
	return ErrorOccurred();

    const float offseps = 1e-6;
    SeisTrcBuf* gather = nullptr;
    while ( true )
    {
	BinID curbid;
	if ( !iter_->next(curbid) )
	    return Finished();

	if ( sd_ && !sd_->isOK(curbid) )
	    continue;

	nrdone_ ++;

	ManagedObjectSet<SeisTrcBuf> gatherset;
	for ( int idx=0; idx<readers_.size(); idx++ )
	{
	    gather = new SeisTrcBuf( true );
	    if ( !readers_[idx]->getGather(curbid,*gather) )
	    {
		delete gather; gather = 0;
		continue;
	    }

	    gatherset += gather;
	    if ( !dostack_ )
		break;
	}

	if ( gatherset.isEmpty() ) continue;

	gather = gatherset[0];
	if ( dostack_ && gatherset.size() > 1 )
	    stackGathers( *gather, gatherset );

	for ( int tdx=0; tdx<gather->size(); tdx++ )
	{
	    const SeisTrc& gathtrc = *gather->get( tdx );
	    const float offs = gathtrc.info().offset_;
	    if ( offs < offsrg_.start_ - offseps
		 || offs > offsrg_.stop_ + offseps )
		continue;

	    const SeisTrc* wrtrc = resampler_->get( gathtrc );
	    if ( wrtrc && !writer_->put(*wrtrc) )
	    {
		msg_ = writer_->errMsg();
		return ErrorOccurred();
	    }
	}

	return MoreToDo();
    }
}


void SeisPSMerger::stackGathers( SeisTrcBuf& resgather,
				 const ObjectSet<SeisTrcBuf>& duplgathers )
{
    const int sz = resgather.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	SeisTrc& trc = *resgather.get( idx );
	SeisTrcPropChg stckr( trc );
	for ( int gdx=1; gdx<duplgathers.size(); gdx++ )
	{
	    const SeisTrcBuf& dupgather = *duplgathers[gdx];
	    const SeisTrc& duptrc = *dupgather.get( idx );
	    stckr.stack( duptrc, false, (float) 1/gdx );
	}
    }
}
