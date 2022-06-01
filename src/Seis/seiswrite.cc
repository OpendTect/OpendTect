/*+
* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* AUTHOR   : A.H. Bril
* DATE     : 28-1-1998
-*/

#include "seiswrite.h"

#include "executor.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "posinfo2dsurv.h"
#include "seispsioprov.h"
#include "seispscubetr.h"
#include "seispswrite.h"
#include "seisselection.h"
#include "seisstor.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seis2ddata.h"
#include "seis2dlineio.h"
#include "separstr.h"
#include "survgeom2d.h"
#include "thread.h"
#include "threadwork.h"
#include "uistrings.h"


#define mCurGeomID (gidp_ \
    ? gidp_->geomID() \
    : (seldata_ ? seldata_->geomID():Survey::GM().cUndefGeomID()))


SeisTrcWriter::SeisTrcWriter( const IOObj* ioob, const GeomIDProvider* l )
	: SeisStoreAccess(ioob)
	, auxpars_(*new IOPar)
	, gidp_(l)
	, worktrc_(*new SeisTrc)
	, linedata_(0)
{
    init();
}


SeisTrcWriter::SeisTrcWriter( const char* fnm, bool is2d, bool isps )
	: SeisStoreAccess(fnm,is2d,isps)
	, auxpars_(*new IOPar)
	, gidp_(0)
	, worktrc_(*new SeisTrc)
	, linedata_(0)
{
    init();
}


void SeisTrcWriter::init()
{
    putter_ = 0; pswriter_ = 0;
    nrtrcs_ = nrwritten_ = 0;
    prepared_ = false;
    firstns_ = mUdf(int);
    firstsampling_.start = mUdf(float);
}


SeisTrcWriter::~SeisTrcWriter()
{
    close();
    delete linedata_;
    delete &auxpars_;
    delete &worktrc_;
}


bool SeisTrcWriter::close()
{
    bool ret = true;
    if ( putter_ )
	{ ret = putter_->close(); if ( !ret ) errmsg_ = putter_->errMsg(); }

    if ( is2D() )
    {
	Pos::GeomID geomid = mCurGeomID;
	Survey::Geometry* geom = Survey::GMAdmin().getGeometry( geomid );
	mDynamicCastGet(Survey::Geometry2D*,geom2d,geom);

	if ( geom2d && geom2d->data().isEmpty() )
	{
	    uiString st;
	    geom2d->dataAdmin() = *linedata_;
	    geom2d->spnrs() = spnrs_;
	    geom2d->touch();
	    Survey::GMAdmin().write( *geom2d, st );
	}
    }

    deleteAndZeroPtr( putter_ );
    deleteAndZeroPtr( pswriter_ );
    psioprov_ = nullptr;
    ret &= SeisStoreAccess::close();

    return ret;
}


void SeisTrcWriter::setGeomIDProvider( const GeomIDProvider* prov )
{
    gidp_ = prov;
    delete linedata_;
    linedata_ = new PosInfo::Line2DData( Survey::GM().getName(mCurGeomID) );
    spnrs_.erase();
}


void SeisTrcWriter::setSelData( Seis::SelData* tsel )
{
    SeisStoreAccess::setSelData( tsel );
    delete linedata_;
    spnrs_.erase();

    if ( !is2D() )
	return;

    linedata_ = new PosInfo::Line2DData( Survey::GM().getName(mCurGeomID) );
}


bool SeisTrcWriter::prepareWork( const SeisTrc& trc )
{
    if ( !ioobj_ )
    {
	errmsg_ = tr("Info for output seismic data "
		     "not found in Object Manager");
	return false;
    }

    if ( !psioprov_ && !is2d_ && !trl_ )
    {
	errmsg_ = tr("No data storer available for '%1'")
		.arg( ioobj_->name() );
	return false;
    }
    if ( is2d_ && !gidp_ && ( !seldata_ || (seldata_->geomID() < 0) ) )
    {
	errmsg_ = tr("Internal: 2D seismic can only "
		     "be stored if line key known");
	return false;
    }

    if ( is2d_ && !psioprov_ )
    {
	if ( !linedata_ )
	    return false;
	if ( !next2DLine() )
	    return false;

	SamplingData<float> sd = trc.info().sampling;
	StepInterval<float> zrg( sd.start, 0, sd.step );
	zrg.stop = sd.start + sd.step * (trc.size()-1);
	linedata_->setZRange( zrg );
    }
    else if ( psioprov_ )
    {
	if ( !is2d_ )
	    pswriter_ = SPSIOPF().get3DWriter( *ioobj_ );
	else
	{
	    const Pos::GeomID geomid = mCurGeomID;
	    const char* lnm = is2d_ ? Survey::GM().getName(geomid) : 0;
	    pswriter_ = SPSIOPF().get2DWriter( *ioobj_, lnm );
	    SamplingData<float> sd = trc.info().sampling;
	    StepInterval<float> zrg( sd.start, 0, sd.step );
	    if ( linedata_ ) linedata_->setZRange( zrg );
	}
	if ( !pswriter_ )
	{
	    errmsg_ = tr("Cannot open Data store for write");
	    return false;
	}

	if ( !is2d_ )
	    SPSIOPF().mk3DPostStackProxy( *ioobj_ );
    }
    else
    {
	mDynamicCastGet(const IOStream*,strm,ioobj_)
	if ( !strm || !strm->isMulti() )
	    IOM().implRemove( *ioobj_ );

	if ( !ensureRightConn(trc,true) )
	    return false;
    }

    prepared_ = true;

    if ( !ioobj_->isTmp() && !ioobj_->isProcTmp() )
    {
	ioobj_->pars().update( sKey::CrFrom(), crfrom_ );
	ioobj_->pars().update( sKey::CrInfo(), crusrinfo_ );
	ioobj_->updateCreationPars();
	IOM().commitChanges( *ioobj_ );
    }

    return prepared_;
}


Conn* SeisTrcWriter::crConn( int inl, bool first )
{
    if ( !ioobj_ )
    { errmsg_ = tr("No data from object manager"); return 0; }

    if ( isMultiConn() )
    {
	mDynamicCastGet(IOStream*,iostrm,ioobj_)
	if ( iostrm->fileSpec().isRangeMulti() )
	    inl = iostrm->connIdxFor( inl );
	iostrm->setConnIdx( inl );
    }

    return ioobj_->getConn( Conn::Write );
}


bool SeisTrcWriter::start3DWrite( Conn* conn, const SeisTrc& trc )
{
    if ( !conn || conn->isBad() || !trl_ )
    {
	errmsg_ = tr("Cannot write to %1")
		.arg( ioobj_->fullUserExpr(false) );
	delete conn;
	return false;
    }

    strl()->cleanUp();
    if ( !strl()->initWrite(conn,trc) )
    {
	errmsg_ = strl()->errMsg();
	return false;
    }

    return true;
}


bool SeisTrcWriter::ensureRightConn( const SeisTrc& trc, bool first )
{
    bool neednewconn = !curConn3D();

    if ( !neednewconn && isMultiConn() )
    {
	mDynamicCastGet(IOStream*,iostrm,ioobj_)
	if ( iostrm->fileSpec().isRangeMulti() && trc.info().new_packet )
	{
	    const int connidx = iostrm->connIdxFor( trc.info().binid.inl() );
	    neednewconn = connidx != iostrm->curConnIdx();
	}
    }

    if ( neednewconn )
    {
	Conn* conn = crConn( trc.info().binid.inl(), first );
	if ( !conn )
	{
	    errmsg_ = tr("Cannot create output stream");
	    if ( ioobj_ && FixedString(ioobj_->translator()) == "Blocks" )
		errmsg_ = uiStrings::phrNotImplInThisVersion( "7.X" );
	    return false;
	}
	if ( !conn || !start3DWrite(conn,trc) )
	    return false;
    }

    return true;
}


bool SeisTrcWriter::next2DLine()
{
    Pos::GeomID geomid = mCurGeomID;
    BufferString lnm = Survey::GM().getName( geomid );
    if ( lnm.isEmpty() )
    {
	errmsg_ = tr("Invalid 2D Geometry");
	return false;
    }

    prevgeomid_ = geomid;
    delete putter_;

    putter_ = dataset_->linePutter( geomid );

    if ( !putter_ )
    {
	errmsg_ = toUiString("Cannot create 2D line writer");
	return false;
    }

    linePutter()->setComponentNames( compnames_ );

    return true;
}


bool SeisTrcWriter::put2D( const SeisTrc& trc )
{
    if ( !putter_ || !linedata_ ) return false;

    if ( mCurGeomID != prevgeomid_ )
    {
	if ( !next2DLine() )
	    return false;
    }

    bool res = putter_->put( trc );
    if ( !res )
	errmsg_ = putter_->errMsg();

    PosInfo::Line2DPos pos( trc.info().nr );
    pos.coord_ = trc.info().coord;
    linedata_->add( pos );
    spnrs_ += mIsUdf(trc.info().refnr) ? Survey::Geometry2D::udfSPNr()
					: trc.info().refnr;

    return res;
}


bool SeisTrcWriter::put( const SeisTrc& trc )
{
    if ( !prepared_ && !prepareWork(trc) )
	return false;

    nrtrcs_++;
    if ( seldata_ )
    {
	BinID selbid = trc.info().binid;
	if ( is2d_ )
	    selbid = BinID( seldata_->inlRange().start, trc.info().nr );

	if ( seldata_->selRes(selbid) )
	    return true;
    }

    if ( psioprov_ )
    {
	if ( !pswriter_ )
	    return false;
	if ( !pswriter_->put(trc) )
	{
	    errmsg_ = pswriter_->errMsg();
	    return false;
	}

	if ( is2d_ && linedata_ && linedata_->indexOf(trc.info().nr) < 0 )
	{
	    PosInfo::Line2DPos pos( trc.info().nr );
	    pos.coord_ = trc.info().coord;
	    linedata_->add( pos );
	    spnrs_ += mIsUdf(trc.info().refnr) ? Survey::Geometry2D::udfSPNr()
						: trc.info().refnr;
	}
    }
    else if ( is2d_ )
    {
	if ( !put2D(trc) )
	    return false;
    }
    else
    {
	if ( !ensureRightConn(trc,false) )
	    return false;
	else if ( !strl()->write(trc) )
	{
	    errmsg_ = strl()->errMsg();
	    strl()->close(); delete trl_; trl_ = 0;
	    return false;
	}
    }

    nrwritten_++;
    return true;
}


bool SeisTrcWriter::isMultiConn() const
{
    mDynamicCastGet(IOStream*,iostrm,ioobj_)
    return iostrm && iostrm->isMulti();
}


void SeisTrcWriter::setComponentNames( const BufferStringSet& compname )
{
    compnames_ = compname;
    if ( strl() ) strl()->setComponentNames( compnames_ );
    if ( putter_ ) linePutter()->setComponentNames( compnames_ );
}


//SeisSequentialWriter
SeisSequentialWriter::SeisSequentialWriter( SeisTrcWriter* writer,
					    int buffsize )
    : writer_( writer )
    , lock_(*new Threads::ConditionVar)
    , maxbuffersize_( buffsize<1 ? Threads::getNrProcessors()*2 : buffsize )
    , latestbid_( -1, -1 )
{
    queueid_ = Threads::WorkManager::twm().addQueue(
				    Threads::WorkManager::SingleThread,
				    "SequentalWriter");
}


bool SeisSequentialWriter::finishWrite()
{
    if ( outputs_.size() )
    {
	pErrMsg( "Buffer is not empty" );
	deepErase( outputs_ );
    }

    Threads::WorkManager::twm().emptyQueue( queueid_, true );

    return errmsg_.isEmpty();
}


SeisSequentialWriter::~SeisSequentialWriter()
{
    if ( outputs_.size() )
	{ pErrMsg( "Buffer is not empty" ); deepErase( outputs_ ); }
    if ( Threads::WorkManager::twm().queueSize( queueid_ ) )
	{ pErrMsg("finishWrite was not called"); }
    Threads::WorkManager::twm().removeQueue( queueid_, false );

    delete &lock_;
}


bool SeisSequentialWriter::announceTrace( const BinID& bid )
{
    Threads::MutexLocker lock( lock_ );
    if ( bid.inl()<latestbid_.inl() ||
	    (bid.inl()==latestbid_.inl() && bid.crl()<latestbid_.crl() ) )
    {
	errmsg_ = tr("Announced trace is out of sequence");
	return false;
    }

    announcedtraces_ += bid;
    return true;
}


class SeisSequentialWriterTask : public Task
{
public:
    SeisSequentialWriterTask( SeisSequentialWriter& imp, SeisTrcWriter& writer,
			      SeisTrc* trc )
	: ssw_( imp )
	, writer_( writer )
	, trc_( trc )
    {}

    bool execute()
    {
	BufferString errmsg;
	if ( !writer_.put(*trc_) )
	{
	    errmsg = writer_.errMsg().getFullString();
	    if ( errmsg.isEmpty() )
	    {
		pErrMsg( "Need an error message from writer" );
		errmsg = "Cannot write trace";
	    }
	}

	delete trc_;

	ssw_.reportWrite( errmsg.str() );
	return !errmsg.str();
    }


protected:

    SeisSequentialWriter&       ssw_;
    SeisTrcWriter&		writer_;
    SeisTrc*	trc_;
};



bool SeisSequentialWriter::submitTrace( SeisTrc* inptrc, bool waitforbuffer )
{
    Threads::MutexLocker lock( lock_ );
    outputs_ += inptrc;
    return iterateBuffer( waitforbuffer );
}


bool SeisSequentialWriter::submitGather( ObjectSet<SeisTrc>& gather,
					 bool waitforbuffer )
{
    pErrMsg("Not implemented yet. Talk to Bruno" );
    return false;
/*
    Threads::MutexLocker lock( lock_ );
    for ( int idx=0; idx<gather.size(); idx++ )
	outputs_ += gather[idx];

    return iterateBuffer( waitforbuffer );
*/
}


bool SeisSequentialWriter::iterateBuffer( bool waitforbuffer )
{
    if ( !writer_ )
	return false;

    bool found = true;
    while ( found )
    {
	found = false;
	int idx = 0;

	while ( idx<announcedtraces_.size() )
	{
	    const BinID& bid = announcedtraces_[idx];

	    ObjectSet<SeisTrc> trcs;
	    for ( int idy=0; idy<outputs_.size(); idy++ )
	    {
		const bool samepos = writer_->is2D()
		    ? bid.crl() == outputs_[idy]->info().nr
		    : outputs_[idy]->info().binid == bid;
		if ( samepos )
		{
		    trcs += outputs_.removeSingle( idy );
		    idy--;
		}
		else if ( trcs.size() )
		    break;
	    }

	    if ( !trcs.size() )
	    {
		idx--;
		break;
	    }

	    for ( int idy=0; idy<trcs.size(); idy++ )
	    {
		Task* task =
		    new SeisSequentialWriterTask( *this, *writer_, trcs[idy] );
		Threads::WorkManager::twm().addWork( Threads::Work(*task,true),
						     0, queueid_, false, false,
						     true );
	    }

	    found = true;

	    idx++;
	}

	if ( idx>=0 )
	    announcedtraces_.removeRange( 0, idx );
    }

    if ( found )
	lock_.signal(true);

    if ( waitforbuffer )
    {
	int bufsize = outputs_.size() +
	    Threads::WorkManager::twm().queueSize( queueid_ );
	while ( bufsize>=maxbuffersize_ && !errmsg_ )
	{
	    lock_.wait();
	    bufsize = outputs_.size() +
			Threads::WorkManager::twm().queueSize( queueid_ );
	}
    }

    return errmsg_.isEmpty();
}


void SeisSequentialWriter::reportWrite( const char* errmsg )
{
    Threads::MutexLocker lock( lock_ );
    if ( errmsg )
    {
	errmsg_ = mToUiStringTodo(errmsg);
	Threads::WorkManager::twm().emptyQueue( queueid_, false );
	lock_.signal( true );
	return;
    }

    const int bufsize = Threads::WorkManager::twm().queueSize( queueid_ ) +
			outputs_.size();
    if ( bufsize<maxbuffersize_ )
	lock_.signal( true );
}
