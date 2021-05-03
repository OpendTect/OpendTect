/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          July 2013
________________________________________________________________________

-*/


#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "prestackgather.h"
#include "seistrc.h"
#include "seisbufadapters.h"
#include "seiswrite.h"
#include "seisselectionimpl.h"
#include "seispsioprov.h"
#include "seis2dlineio.h"
#include "stratsynthexp.h"
#include "syntheticdataimpl.h"
#include "posinfo2d.h"
#include "survinfo.h"
#include "survgeom.h"
#include "separstr.h"
#include "transl.h"

StratSynthExporter::StratSynthExporter(
	const ObjectSet<const SyntheticData>& sds,
	PosInfo::Line2DData* newgeom, const SeparString& prepostfix )
    : Executor( "Exporting syntheic data" )
    , sds_(sds)
    , linegeom_(newgeom)
    , cursdidx_(0)
    , posdone_(0)
    , postobedone_(0)
    , prefixstr_(prepostfix[0].str())
    , postfixstr_(prepostfix[1].str())
    , writer_(0)
{
    int synthmodelsz = 0;
    mDynamicCastGet(const PreStackSyntheticData*,presd,sds_[0]);
    mDynamicCastGet(const PostStackSyntheticData*,postsd,sds_[0]);
    if ( presd )
	synthmodelsz = presd->preStackPack().getGathers().size();
    else
	synthmodelsz = postsd->postStackPack().trcBuf().size();

    postobedone_ = linegeom_->positions().size() < synthmodelsz
			    ? linegeom_->positions().size() : synthmodelsz;
}


StratSynthExporter::~StratSynthExporter()
{
    delete writer_;
}




od_int64 StratSynthExporter::nrDone() const
{
    return (cursdidx_*postobedone_) + posdone_;
}


od_int64 StratSynthExporter::totalNr() const
{
    return sds_.size()*postobedone_;
}

#define mSkipInitialBlanks( str ) \
{ \
BufferString copystr = str.buf(); \
char* strptr = copystr.getCStr(); \
mSkipBlanks( strptr ); \
str = strptr; \
}

bool StratSynthExporter::prepareWriter()
{
    const bool isps = sds_[cursdidx_]->isPS();
    BufferString synthnm;
    mSkipInitialBlanks( prefixstr_ )
    if ( !prefixstr_.isEmpty() )
    {
	synthnm += prefixstr_.buf();
	synthnm += "_";
    }

    synthnm += sds_[cursdidx_]->name();

    mSkipInitialBlanks( postfixstr_ );
    if ( !postfixstr_.isEmpty() )
    {
	synthnm += "_";
	synthnm += postfixstr_.buf();
    }

    PtrMan<CtxtIOObj> ctxt;
    if ( isps )
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
    NotifyStopper stopaddentrynot( IOM().entryAdded );
    IOM().getEntry( *ctxt, false );
    delete writer_;
    writer_ = new SeisTrcWriter( ctxt->ioobj_ );
    Seis::SelData* seldata = Seis::SelData::get( Seis::Range );
    Survey::Geometry::ID newgeomid =
	Survey::GM().getGeomID( linegeom_->lineName() );
    seldata->setGeomID( newgeomid );
    writer_->setSelData( seldata );
    writer_->setAttrib( synthnm );
    return true;
}


int StratSynthExporter::nextStep()
{
    if ( !sds_.validIdx(cursdidx_) )
	return Executor::Finished();

    const bool isps = sds_[cursdidx_]->isPS();
    if ( !posdone_ && !prepareWriter() )
	return ErrorOccurred();

    return !isps ? writePostStackTrace() : writePreStackTraces();
}


uiString StratSynthExporter::uiMessage() const
{
    return errmsg_.isEmpty() ? tr("Exporting syntheic data") : errmsg_;
}

#define mErrRetPErr( msg ) \
{ pErrMsg( msg ); return ErrorOccurred(); }

int StratSynthExporter::writePostStackTrace()
{
    const SyntheticData* sd = sds_[cursdidx_];
    mDynamicCastGet(const PostStackSyntheticData*,postsd,sd);
    if ( !postsd )
	mErrRetPErr( "Wrong type (not PostStackSyntheticData)" )

    const SeisTrcBuf& seisbuf = postsd->postStackPack().trcBuf();
    const TypeSet<PosInfo::Line2DPos>& positions = linegeom_->positions();
    if ( posdone_ >= postobedone_ )
    {
	cursdidx_++;
	posdone_ = 0;
	return Executor::MoreToDo();
    }

    const PosInfo::Line2DPos& linepos = positions[posdone_];
    const SeisTrc* synthrc = seisbuf.get( posdone_ );
    if ( !synthrc )
	mErrRetPErr( "Cannot find the trace in the required position" )

    SeisTrc trc( *synthrc );
    trc.info().nr = linepos.nr_;
    trc.info().binid = SI().transform( linepos.coord_ );
    trc.info().coord = linepos.coord_;
    if ( !writer_->put(trc) )
    {
	errmsg_ = writer_->errMsg();
	return ErrorOccurred();
    }

    posdone_++;
    return Executor::MoreToDo();
}


int StratSynthExporter::writePreStackTraces()
{
    const SyntheticData* sd = sds_[cursdidx_];
    mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
    if ( !presd )
	mErrRetPErr( "Wrong type (not PreStackSyntheticData)" )
    const PreStack::GatherSetDataPack& gsdp = presd->preStackPack();
    const ObjectSet<PreStack::Gather>& gathers = gsdp.getGathers();
    const TypeSet<PosInfo::Line2DPos>& positions = linegeom_->positions();
    if ( posdone_ >= postobedone_ ) 
    {
	cursdidx_++;
	posdone_ = 0;
	return Executor::MoreToDo();
    }

    const PosInfo::Line2DPos& linepos = positions[posdone_];

    if ( !gathers.validIdx(posdone_) )
	mErrRetPErr( "Cannot find the gather in the required position" );
    const PreStack::Gather* gather = gathers[posdone_];
    for ( int offsidx=0; offsidx<gather->size(true); offsidx++ )
    {
	const float offset = gather->getOffset( offsidx );
	SeisTrc trc( *gsdp.getTrace(posdone_,offsidx) );
	trc.info().nr = linepos.nr_;
	trc.info().binid = SI().transform( linepos.coord_ );
	trc.info().coord = linepos.coord_;
	trc.info().offset = offset;
	if ( !writer_->put(trc) )
	{
	    errmsg_ = writer_->errMsg();
	    return ErrorOccurred();
	}
    }

    posdone_++;
    return Executor::MoreToDo();
}
