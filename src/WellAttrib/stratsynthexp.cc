/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          July 2013
________________________________________________________________________

-*/


#include "ctxtioobj.h"
#include "ioobj.h"
#include "dbman.h"
#include "prestackgather.h"
#include "seistrc.h"
#include "seisbufadapters.h"
#include "seisstorer.h"
#include "seisseldata.h"
#include "seispsioprov.h"
#include "seis2dlineio.h"
#include "stratsynthexp.h"
#include "synthseisdataset.h"
#include "trckey.h"
#include "posinfo2d.h"
#include "survinfo.h"
#include "survgeom.h"
#include "separstr.h"
#include "transl.h"

#include "prestacksynthdataset.h"

StratSynthExporter::StratSynthExporter( const Setup& setup,
	const ObjectSet<const SynthSeis::DataSet>& sds,
	const PosInfo::Line2DData& newgeom )
    : Executor( "Exporting synthetic data" )
    , sds_(sds)
    , setup_(setup)
    , linegeom_(newgeom)
    , cursdsidx_(0)
    , posdone_(0)
    , postobedone_(0)
    , storer_(0)
{
    int synthmodelsz = 0;
    ConstRefMan<SynthSeis::DataSet> sd = sds_[0];
    mDynamicCastGet(const SynthSeis::PreStackDataSet*,presd,sd.ptr());
    mDynamicCastGet(const SynthSeis::PostStackDataSet*,postsd,sd.ptr());
    if ( presd )
	synthmodelsz = presd->preStackPack().getGathers().size();
    else
	synthmodelsz = postsd->postStackPack().trcBuf().size();

    postobedone_ = linegeom_.positions().size() < synthmodelsz
			    ? linegeom_.positions().size() : synthmodelsz;
}


StratSynthExporter::~StratSynthExporter()
{
    delete storer_;
}

od_int64 StratSynthExporter::nrDone() const
{ return (cursdsidx_*postobedone_) + posdone_; }

od_int64 StratSynthExporter::totalNr() const
{ return sds_.size()*postobedone_; }


bool StratSynthExporter::prepareStorer()
{
    BufferString synthnm;
    if ( !setup_.prefix_.isEmpty() )
	synthnm.add( setup_.prefix_ ).add( "_" );
    synthnm += sds_[cursdsidx_]->name();
    if ( !setup_.postfix_.isEmpty() )
	synthnm.add( "_" ).add( setup_.postfix_ ).add( "_" );

    PtrMan<CtxtIOObj> ctxt;
    if ( sds_[cursdsidx_]->isPS() )
    {
	ctxt = mMkCtxtIOObj( SeisPS2D );
	ctxt->ctxt_.deftransl_ = CBVSSeisPS2DTranslator::translKey();
    }
    else
    {
	ctxt = mMkCtxtIOObj( SeisTrc2D );
	ctxt->ctxt_.deftransl_ = CBVSSeisTrc2DTranslator::translKey();
    }

    ctxt->setName( synthnm.buf() );
    NotifyStopper stopaddentrynot( DBM().entryAdded );
    DBM().getEntry( *ctxt, false );
    if ( !ctxt->ioobj_ )
	return false;

    delete storer_;
    storer_ = new Seis::Storer( *ctxt->ioobj_ );
    storer_->setFixedGeomID( setup_.geomid_ );

    return true;
}


int StratSynthExporter::nextStep()
{
    if ( !sds_.validIdx(cursdsidx_) )
	return Executor::Finished();

    if ( !posdone_ && !prepareStorer() )
	return ErrorOccurred();

    return !sds_[cursdsidx_]->isPS() ? writePreStackTraces()
				    : writePostStackTrace();
}


uiString StratSynthExporter::message() const
{
    return errmsg_.isEmpty() ? tr("Exporting syntheic data") : errmsg_;
}


void StratSynthExporter::prepTrc4Store( SeisTrc& trc ) const
{
    if ( setup_.replaceudfs_ )
	trc.ensureNoUndefs( mUdf(float) );
}


#define mErrRetPErr( msg ) \
{ pErrMsg( msg ); return ErrorOccurred(); }

int StratSynthExporter::writePostStackTrace()
{
    mDynamicCastGet( const SynthSeis::PostStackDataSet*, postsds,
		     sds_[cursdsidx_] );
    if ( !postsds )
	mErrRetPErr( "Wrong type (not PostStack)" )

    const auto& seisbuf = postsds->postStackPack().trcBuf();
    const auto& positions = linegeom_.positions();
    if ( posdone_ >= postobedone_ )
    {
	cursdsidx_++;
	posdone_ = 0;
	return Executor::MoreToDo();
    }

    const auto& linepos = positions[posdone_];
    const auto* synthrc = seisbuf.get( posdone_ );
    if ( !synthrc )
	mErrRetPErr( "Cannot find the trace in the required position" )

    SeisTrc trc( *synthrc );
    trc.info().setPos( Bin2D(setup_.geomid_,linepos.nr_) );
    trc.info().coord_ = linepos.coord_;
    prepTrc4Store( trc );
    errmsg_ = storer_->put( trc );
    if ( !errmsg_.isEmpty() )
	return ErrorOccurred();

    posdone_++;
    return Executor::MoreToDo();
}


int StratSynthExporter::writePreStackTraces()
{
    mDynamicCastGet( const SynthSeis::PreStackDataSet*, presds,
		     sds_[cursdsidx_] );
    if ( !presds )
	mErrRetPErr( "Wrong type (not PreStack)" )

    const auto& gsdp = presds->preStackPack();
    const auto& gathers = gsdp.getGathers();
    const auto& positions = linegeom_.positions();

    if ( posdone_ >= postobedone_ )
    {
	cursdsidx_++;
	posdone_ = 0;
	return Executor::MoreToDo();
    }
    if ( !gathers.validIdx(posdone_) )
	mErrRetPErr( "Cannot find the gather in the required position" );

    const auto& linepos = positions[posdone_];
    const auto& gather = *gathers[posdone_];
    for ( int offsidx=0; offsidx<gather.size(true); offsidx++ )
    {
	const float offset = gather.getOffset( offsidx );
	PtrMan<SeisTrc> trc = gsdp.createTrace( posdone_, offsidx );
	auto& ti = trc->info();
	ti.setPos( Bin2D(setup_.geomid_,linepos.nr_) );
	ti.coord_ = linepos.coord_;
	ti.offset_ = offset;
	prepTrc4Store( *trc );
	errmsg_ = storer_->put( *trc );
	if ( !errmsg_.isEmpty() )
	    return ErrorOccurred();
    }

    posdone_++;
    return Executor::MoreToDo();
}
