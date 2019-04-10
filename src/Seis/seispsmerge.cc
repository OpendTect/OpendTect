/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R. K. Singh
 * DATE     : Oct 2007
-*/


#include "seispsmerge.h"
#include "ioobj.h"
#include "posinfo.h"
#include "seisbuf.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seisresampler.h"
#include "seisseldata.h"
#include "seisstorer.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "survinfo.h"


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
	, storer_(0)
	, dostack_(dostack)
	, sd_(sd && !sd->is2D() && !sd->isAll() ? sd->clone() : 0)
	, offsrg_(0,mUdf(float))
	, msg_(tr("Handling gathers"))
{
    for ( int idx=0; idx<inobjs.size(); idx++ )
    {
	SeisPS3DReader* rdr = SPSIOPF().get3DReader( *inobjs[idx] );
	if ( rdr )
	    readers_ += rdr;
    }
    if ( readers_.isEmpty() )
	{ msg_ = tr("No valid inputs specified"); return; }

    if ( sd_ )
	totnr_ = sd_->expectedNrTraces();
    else
	totnr_ = SI().maxNrTraces();

    if ( sd_ && sd_->isRange() )
	resampler_ = new SeisResampler( *sd_->asRange() );

    storer_ = new Seis::Storer( out );
    if ( storer_->isUsable() )
	{ deepErase( readers_ ); msg_ = storer_->errNotUsable(); }
}


SeisPSMerger::~SeisPSMerger()
{
    delete iter_;
    delete storer_;
    delete sd_;
    delete resampler_;
    deepErase( readers_ );
}


int SeisPSMerger::nextStep()
{
    if ( readers_.isEmpty() )
	return ErrorOccurred();

    nrdone_ ++;

    const float offseps = 1e-6;
    SeisTrcBuf* gather = 0;
    const BinID curbid( iter_->curBinID() );
    if ( sd_ && !sd_->isOK(curbid) )
	return iter_->next() ? MoreToDo() : Finished();

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

    if ( gatherset.isEmpty() )
	return iter_->next() ? MoreToDo() : Finished();

    gather = gatherset[0];
    if ( dostack_ && gatherset.size() > 1 )
	stackGathers( *gather, gatherset );

    for ( int tdx=0; tdx<gather->size(); tdx++ )
    {
	const SeisTrc& gathtrc = *gather->get( tdx );
	const float offs = gathtrc.info().offset_;
	if ( offs < offsrg_.start - offseps
	  || offs > offsrg_.stop + offseps )
	    continue;

	const SeisTrc* wrtrc = resampler_ ? resampler_->get(gathtrc) : &gathtrc;
	if ( wrtrc )
	{
	    auto uirv = storer_->put( *wrtrc );
	    if ( !uirv.isOK() )
		{ msg_ = uirv; return ErrorOccurred(); }
	}
    }

    return iter_->next() ? MoreToDo() : Finished();
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
