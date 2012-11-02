/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R. K. Singh
 * DATE     : Oct 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

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


SeisPSMerger::SeisPSMerger( const ObjectSet<IOObj>& inobjs, const IOObj& out,
       			    bool dostack, const Seis::SelData* sd )
  	: Executor("Merging Pre-Stack data")
	, writer_(0)
	, dostack_(dostack)
	, sd_(sd && !sd->isAll() ? sd->clone() : 0)
	, offsrg_(0,mUdf(float))
	, msg_("Handling gathers")
	, totnr_(-1)
	, nrdone_(0)
{
    CubeSampling cs; bool havecs = false;
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
	    cs.zrg.start = zrg.start;
	    cs.zrg.stop = zrg.stop;
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
	    cs.hrg.set( inlrg, crlrg );
	else
	{
	    cs.hrg.include( BinID(inlrg.start,crlrg.start) );
	    cs.hrg.include( BinID(inlrg.stop,crlrg.stop) );
	}
    }
    if ( readers_.isEmpty() )
	{ msg_ = "No valid inputs specified"; return; }

    totnr_ = mCast( int, sd_ ? sd_->expectedNrTraces() : cs.hrg.totalNr() );
    iter_ = new HorSamplingIterator( cs.hrg );
    resampler_ = new SeisResampler( cs );

    writer_ = new SeisTrcWriter( &out );
    if ( !writer_ )
	{ deepErase(readers_); msg_ = "Cannot create output writer"; return; }
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
	return Executor::ErrorOccurred();

    static const float offseps = 1e-6;
    SeisTrcBuf* gather = 0;
    while ( true )
    {
	if ( !iter_->next(curbid_) )
	    return Executor::Finished();

	if ( sd_ && !sd_->isOK(curbid_) )
	    continue;

	nrdone_ ++;

	ManagedObjectSet<SeisTrcBuf> gatherset( false );
	for ( int idx=0; idx<readers_.size(); idx++ )
	{
	    gather = new SeisTrcBuf( true );
	    if ( !readers_[idx]->getGather(curbid_,*gather) )
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
	    const float offs = gathtrc.info().offset;
	    if ( offs < offsrg_.start - offseps
	      || offs > offsrg_.stop + offseps )
		continue;

	    const SeisTrc* wrtrc = resampler_->get( gathtrc );
	    if ( wrtrc && !writer_->put(*wrtrc) )
		return Executor::ErrorOccurred();
	}

	return Executor::MoreToDo();
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
