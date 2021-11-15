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
#include "survgeom2d.h"
#include "separstr.h"
#include "transl.h"

StratSynthExporter::StratSynthExporter(
			const ObjectSet<const SyntheticData>& sds,
			Pos::GeomID geomid, const SeparString& prepostfix )
    : Executor( "Exporting syntheic data" )
    , sds_(sds)
    , geomid_(geomid)
    , prefixstr_(prepostfix[0].str())
    , postfixstr_(prepostfix[1].str())
{
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid_ );
    const Survey::Geometry2D* geom2d = geom ? geom->as2D() : nullptr;
    if ( geom2d )
	linepos_ = &geom2d->data().positions();
    else
	return;

    int synthmodelsz = 0;
    mDynamicCastGet(const PreStackSyntheticData*,presd,sds_[0]);
    mDynamicCastGet(const PostStackSyntheticData*,postsd,sds_[0]);
    if ( presd )
	synthmodelsz = presd->preStackPack().getGathers().size();
    else
	synthmodelsz = postsd->postStackPack().trcBuf().size();

    postobedone_ = linepos_->size() < synthmodelsz
		 ? linepos_->size() : synthmodelsz;
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
    if ( !ctxt->ioobj_ )
	return false;

    const Seis::GeomType gt = Seis::Line;
    delete writer_;
    writer_ = new SeisTrcWriter( *ctxt->ioobj_, geomid_, &gt );
    writer_->setAttrib( synthnm );
    return true;
}


int StratSynthExporter::nextStep()
{
    if ( !sds_.validIdx(cursdidx_) )
	return Finished();

    const bool isps = sds_[cursdidx_]->isPS();
    if ( !posdone_ && !prepareWriter() )
	return ErrorOccurred();

    return isps ? writePreStackTraces() : writePostStackTrace();
}


uiString StratSynthExporter::uiMessage() const
{
    return errmsg_.isEmpty() ? tr("Exporting synthetic data") : errmsg_;
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
    if ( posdone_ >= postobedone_ )
    {
	cursdidx_++;
	posdone_ = 0;
	return MoreToDo();
    }

    const SeisTrc* synthrc = seisbuf.get( posdone_ );
    if ( !synthrc )
	mErrRetPErr( "Cannot find the trace in the required position" )

    const PosInfo::Line2DPos& linepos = (*linepos_)[posdone_];
    SeisTrc trc( *synthrc );
    SeisTrcInfo& trcinfo = trc.info();
    trcinfo.setGeomID( geomid_ ).setTrcNr( linepos.nr_ ).calcCoord();
    trcinfo.seqnr_ = ++posdone_;
    if ( !writer_->put(trc) )
    {
	errmsg_ = writer_->errMsg();
	return ErrorOccurred();
    }

    return MoreToDo();
}


int StratSynthExporter::writePreStackTraces()
{
    const SyntheticData* sd = sds_[cursdidx_];
    mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
    if ( !presd )
	mErrRetPErr( "Wrong type (not PreStackSyntheticData)" )

    const PreStack::GatherSetDataPack& gsdp = presd->preStackPack();
    const ObjectSet<PreStack::Gather>& gathers = gsdp.getGathers();
    if ( posdone_ >= postobedone_ )
    {
	cursdidx_++;
	posdone_ = 0;
	return MoreToDo();
    }

    if ( !gathers.validIdx(posdone_) )
	mErrRetPErr( "Cannot find the gather in the required position" );

    const PosInfo::Line2DPos& linepos = (*linepos_)[posdone_];
    const PreStack::Gather* gather = gathers[posdone_];
    for ( int offsidx=0; offsidx<gather->size(true); offsidx++ )
    {
	const float offset = gather->getOffset( offsidx );
	SeisTrc trc( *gsdp.getTrace(posdone_,offsidx) );
	SeisTrcInfo& trcinfo = trc.info();
	trcinfo.setGeomID( geomid_ ).setTrcNr( linepos.nr_ ).calcCoord();
	trcinfo.seqnr_ = posdone_+1;
	trcinfo.offset = offset;
	if ( !writer_->put(trc) )
	{
	    errmsg_ = writer_->errMsg();
	    return ErrorOccurred();
	}
    }

    posdone_++;
    return MoreToDo();
}
