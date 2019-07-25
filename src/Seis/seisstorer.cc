/*+
* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* AUTHOR   : Bert
* DATE     : Feb 2019
-*/

#include "seisstorer.h"

#include "dbman.h"
#include "executor.h"
#include "filepath.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "scaler.h"
#include "seispsioprov.h"
#include "seispswrite.h"
#include "seisstatscollector.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seis2ddata.h"
#include "seis2dlineio.h"
#include "seisioobjinfo.h"
#include "survgeom2d.h"
#include "survgeommgr.h"
#include "thread.h"
#include "threadwork.h"
#include "trckey.h"
#include "uistrings.h"


Seis::Storer::Storer()
    : statscollector_(*new StatsCollector)
    , prevtrcky_(*new TrcKey)
{
}

Seis::Storer::Storer( const DBKey& dbky )
    : Storer()
{
    setIOObj( getIOObj(dbky) );
}

Seis::Storer::Storer( const IOObj& ioobj )
    : Storer()
{
    setIOObj( ioobj.clone() );
}


Seis::Storer::~Storer()
{
    if ( !prevtrcky_.isUdf() )
	{ pErrMsg( "Use close() and if !isOK() present to user" ); close(); }
    delete &statscollector_;
    delete &prevtrcky_;
}


bool Seis::Storer::is2D() const
{
    return is2DLine() || is2dps_;
}


uiString Seis::Storer::errNotUsable() const
{
    // no idea what would be a good message
    return tr("Cannot store to selected output");
}


uiRetVal Seis::Storer::close()
{
    uiRetVal uirv;

    if ( pswriter_ )
	pswriter_->close();
    else if ( lineputter_ )
    {
	if ( !lineputter_->close() )
	    uirv = lineputter_->errMsg();
    }
    else if ( trl_ )
    {
	if ( !trl_->close() )
	    uirv = trl_->errMsg();
    }

    writeCollectedLineGeometry( uirv );
    writeCollectedStats();

    psioprov_ = 0;
    deleteAndZeroPtr( worktrc_ );
    deleteAndZeroPtr( dataset2d_ );
    deleteAndZeroPtr( linedata_ );
    deleteAndZeroPtr( pswriter_ );
    deleteAndZeroPtr( trl_ );
    deleteAndZeroPtr( ioobj_ );
    statscollector_.setEmpty();
    nrwritten_ = 0;
    prepared_ = is2dps_ = false;
    prevtrcky_.setUdf();

    return uirv;
}


void Seis::Storer::setIOObj( IOObj* ioobj )
{
    close();
    ioobj_ = ioobj;
    if ( !ioobj_ )
	return;

    const SeisIOObjInfo info( *ioobj_ );
    const bool is2d = info.is2D();
    const bool isps = info.isPS();

    if ( is2d )
	dataset2d_ = new Seis2DDataSet( *ioobj_ );

    if ( isps )
    {
	psioprov_ = SPSIOPF().provider( ioobj_->translator() );
	is2dps_ = is2d;
    }
    else if ( !is2d )
    {
	auto* trl = ioobj_->createTranslator();
	mDynamicCast( SeisTrcTranslator*, trl_, trl );
    }
}


bool Seis::Storer::setOutput( const DBKey& dbky )
{
    setIOObj( getIOObj(dbky) );
    return isUsable();
}


bool Seis::Storer::setOutput( const IOObj& ioobj )
{
    setIOObj( ioobj.clone() );
    return isUsable();
}


void Seis::Storer::setScaler( Scaler* sclr )
{
    if ( scaler_ != sclr )
	{ delete scaler_; scaler_ = sclr; }
}


bool Seis::Storer::isUsable() const
{
    return trl_ || psioprov_ || dataset2d_;
}


bool Seis::Storer::writeCollectedLineGeometry( uiRetVal& uirv ) const
{
    if ( linedata_ && !linedata_->isEmpty() )
    {
	auto& geom2d = SurvGeom2D::get4Edit( lastGeomID() );
	geom2d.data() = *linedata_;
	geom2d.commitChanges();
	uiString errmsg;
	if ( !Survey::GMAdmin().save(geom2d,errmsg) )
	    { uirv.add( errmsg ); return false; }
    }

    deleteAndZeroPtr( mNonConst(linedata_) );
    return true;
}


void Seis::Storer::writeCollectedStats() const
{
    IOPar statspar;
    statspar.set( sKey::Source(), "Full Scan" );
    if ( !statscollector_.fillPar(statspar) )
	return;

    File::Path fp( ioobj_->mainFileName() );
    fp.setExtension( sStatsFileExtension() );
    statspar.write( fp.fullPath(), sKey::Stats() );
}


Pos::GeomID Seis::Storer::lastGeomID() const
{
    return prevtrcky_.geomID();
}


Pos::GeomID Seis::Storer::geomID( const SeisTrc& trc ) const
{
    if ( fixedgeomid_.isValid() )
	return fixedgeomid_;

    GeomID ret = trc.info().trcKey().geomID();
    if ( !ret.isValid() || ret.isSynthetic() )
	ret = GeomID::get3D();

    return ret;
}


void Seis::Storer::setPrevTrcKey( const SeisTrc& trc )
{
    prevtrcky_ = trc.info().trcKey();
    prevtrcky_.setGeomID( geomID(trc) );
}


static BufferString getLineName( Pos::GeomID geomid )
{
    BufferString lnm = geomid.name();
    if ( lnm.isEmpty() )
	{ pFreeFnErrMsg("Empty line name"); lnm.set( "ID=" ).add( geomid ); }
    return lnm;
}


#define mErrRet(msg) { uirv.add( msg ); return uirv; }
#define mErrRetInternal(msg) { pErrMsg(msg); mErrRet( mINTERNAL(msg) ); }

uiRetVal Seis::Storer::prepareWork( const SeisTrc& trc )
{
    uiRetVal uirv;

    if ( !ioobj_ )
	mErrRet( uiStrings::phrCannotFindObjInDB() )
    else if ( !isUsable() )
	mErrRet( tr("No data storer available for '%1'").arg(ioobj_->name()) )

    setPrevTrcKey( trc );
    const GeomID geomid = geomID( trc );
    const bool is2d = is2D();
    if ( geomid.is2D() != is2d )
	mErrRetInternal( "Submitted trace is not 2D/3D correct" )

    if ( is2d && !selectLine(trc,uirv) )
	return uirv;

    const bool isps = isPS();
    if ( isps )
    {
	if ( !is2d )
	    pswriter_ = SPSIOPF().get3DWriter( *ioobj_ );
	else if ( !pswriter_ )
	    uirv.set( mINTERNAL("pswriter_ shld be created by selectLine") );

	if ( !pswriter_ )
	    { uirv.set( tr("Cannot open Data store for write") ); return uirv; }

	if ( !is2d )
	    SPSIOPF().mk3DPostStackProxy( *ioobj_ );
    }
    else if ( !is2d )
    {
	mDynamicCastGet(const IOStream*,strm,ioobj_)
	if ( !strm || !strm->isMulti() )
	    fullImplRemove( *ioobj_ );
	if ( !ensureRightConn(trc,true,uirv) )
	    return uirv;
    }

    prepared_ = true;

    if ( !(ioobj_->isTmp() || ioobj_->isProcTmp()) )
    {
	ioobj_->pars().update( sKey::CrFrom(), crfrom_ );
	ioobj_->pars().update( sKey::CrInfo(), crusrinfo_ );
	ioobj_->updateCreationPars();
	ioobj_->commitChanges();
    }

    return uirv;
}


Conn* Seis::Storer::crConn( int inl, bool first, uiRetVal& uirv )
{
    if ( !ioobj_ )
	{ uirv.set( tr("No data from object manager") ); return 0; }

    if ( isMultiConn() )
    {
	mDynamicCastGet(IOStream*,iostrm,ioobj_)
	if ( iostrm->fileSpec().isRangeMulti() )
	    inl = iostrm->connIdxFor( inl );
	iostrm->setConnIdx( inl );
    }

    Conn* ret = ioobj_->getConn( Conn::Write );
    if ( !ret )
	uirv.set( uiStrings::phrCannotOpen(ioobj_->mainFileName(),false) );

    return ret;
}


bool Seis::Storer::start3DWrite( Conn* conn, const SeisTrc& trc,
				 uiRetVal& uirv )
{
    if ( !conn || conn->isBad() || !trl_ )
    {
	uirv.set( tr("Cannot write to %1").arg(ioobj_->mainFileName()) );
	delete conn; return false;
    }

    trl_->cleanUp();
    if ( !trl_->initWrite(conn,trc) )
	{ uirv.set( trl_->errMsg() ); return false; }

    return true;
}


bool Seis::Storer::ensureRightConn( const SeisTrc& trc, bool first,
				    uiRetVal& uirv )
{
    bool neednewconn = !trl_->curConn();

    if ( !neednewconn && isMultiConn() )
    {
	mDynamicCastGet(IOStream*,iostrm,ioobj_)
	if ( iostrm->fileSpec().isRangeMulti() )
	{
	    const int connidx = iostrm->connIdxFor( trc.info().lineNr() );
	    neednewconn = connidx != iostrm->curConnIdx();
	}
    }

    if ( neednewconn )
    {
	Conn* conn = crConn( trc.info().lineNr(), first, uirv );
	if ( !conn || !start3DWrite(conn,trc,uirv) )
	    return false;
    }

    return true;
}

#undef mErrRet
#define mErrRet(msg) { uirv.add( msg ); return false; }
#undef mErrRetInternal
#define mErrRetInternal(msg) { pErrMsg(msg); mErrRet( mINTERNAL(msg) ); }


bool Seis::Storer::selectLine( const SeisTrc& trc, uiRetVal& uirv )
{
    const bool isps = isPS();
    if ( isps )
	deleteAndZeroPtr( pswriter_ );
    else
    {
	deleteAndZeroPtr( lineputter_ );
	if ( !writeCollectedLineGeometry(uirv) )
	    return false;
    }

    const GeomID geomid = geomID( trc );
    if ( !SurvGeom2D::isPresent(geomid) )
    {
	const BufferString msg( "GeomID not found: ", geomid.getI() );
	mErrRetInternal( msg )
    }

    auto& geom = SurvGeom2D::get( geomid );
    const BufferString lnm( getLineName(geomid) );
    if ( geom.isEmpty() )
    {
	linedata_ = new PosInfo::Line2DData( lnm );
	SamplingData<float> sd = trc.info().sampling_;
	StepInterval<float> zrg( sd.start, 0, sd.step );
	zrg.stop = sd.start + sd.step * (trc.size()-1);
	linedata_->setZRange( zrg );
    }

    if ( isps )
    {
	pswriter_ = SPSIOPF().get2DWriter( *ioobj_, lnm );
	if ( !pswriter_ )
	{
	    uirv.set( tr("Cannot open data store for %1").arg(lnm) );
	    return false;
	}
    }
    else
    {
	lineputter_ = dataset2d_->linePutter( geomid, uirv );
	if ( !uirv.isOK() )
	{
	    deleteAndZeroPtr( lineputter_ );
	    return false;
	}
    }

    return true;
}


bool Seis::Storer::putLineTrc( const SeisTrc& trc, uiRetVal& uirv )
{
    if ( !lineputter_ )
	mErrRetInternal( "No lineputter_" )

    if ( !lineputter_->put(trc) )
    {
	uirv.add( lineputter_->errMsg() );
	deleteAndZeroPtr( lineputter_ );
    }

    return uirv.isOK();
}


bool Seis::Storer::putPSTrc( const SeisTrc& trc, uiRetVal& uirv )
{
    if ( !pswriter_ )
	mErrRetInternal( "No pswriter_" )
    if ( !pswriter_->put(trc) )
    {
	uirv = pswriter_->errMsg();
	pswriter_->close();
	deleteAndZeroPtr( pswriter_ );
    }
    return uirv.isOK();
}


bool Seis::Storer::putCubeTrc( const SeisTrc& trc, uiRetVal& uirv )
{
    if ( !trl_ )
	mErrRetInternal( "No trl_" )

    if ( !trl_->write(trc) )
    {
	uirv.set( trl_->errMsg() );
	deleteAndZeroPtr( trl_ );
    }

    return uirv.isOK();
}


uiRetVal Seis::Storer::put( const SeisTrc& inptrc )
{
    if ( !worktrc_ )
	worktrc_ = new SeisTrc( inptrc );

    if ( inptrc.size() != worktrc_->size() )
	{ return mINTERNAL("Writing variable length traces not supported"); }
    else if ( inptrc.nrComponents() != worktrc_->nrComponents() )
	{ return mINTERNAL("Number of components change not supported"); }

    const SeisTrc* trc = &inptrc;
    if ( scaler_ )
    {
	const auto sz = inptrc.size();
	const auto nrcomps = inptrc.nrComponents();
	for ( int icomp=0; icomp<nrcomps; icomp++ )
	{
	    for ( int isamp=0; isamp<sz; isamp++ )
	    {
		float val = inptrc.get( isamp, icomp );
		if ( !mIsUdf(val) )
		    val = (float)scaler_->scale( val );
		worktrc_->set( isamp, val, icomp );
	    }
	}
	worktrc_->info() = inptrc.info();
	trc = worktrc_;
    }

    uiRetVal uirv;
    if ( !prepared_ )
    {
	uirv = prepareWork( *trc );
	if ( !uirv.isOK() )
	    return uirv;
    }

    const bool isps = isPS();
    const bool is2d = is2D();

    if ( is2d )
    {
	if ( geomID(*trc) != lastGeomID() && !selectLine(*trc,uirv) )
	    return uirv;

	if ( (isps && !putPSTrc(*trc,uirv))
	  || (!isps && !putLineTrc(*trc,uirv)) )
	    return uirv;

	if ( linedata_ && !linedata_->isPresent(trc->info().trcNr()) )
	{
	    PosInfo::Line2DPos pos( trc->info().trcNr() );
	    pos.coord_ = trc->info().coord_;
	    linedata_->add( pos );
	}
    }
    else if ( isps )
    {
	if ( !putPSTrc(*trc,uirv) )
	    return uirv;
    }
    else
    {
	if ( !ensureRightConn(*trc,false,uirv) )
	    { deleteAndZeroPtr( trl_ ); return uirv; }
	if ( !putCubeTrc(*trc,uirv) )
	    return uirv;
    }

    statscollector_.useTrace( *trc );
    nrwritten_++;
    setPrevTrcKey( *trc );
    return uirv;
}


bool Seis::Storer::isMultiConn() const
{
    mDynamicCastGet(IOStream*,iostrm,ioobj_)
    return iostrm && iostrm->isMulti();
}


void Seis::Storer::fillPar( IOPar& iopar ) const
{
    if ( ioobj_ )
	iopar.set( sKey::ID(), ioobj_->key() );
    else
	iopar.removeWithKey( sKey::ID() );
    if ( fixedgeomid_.isValid() )
	iopar.set( sKey::GeomID(), fixedgeomid_ );
    else
	iopar.removeWithKey( sKey::GeomID() );
}


void Seis::Storer::usePar( const IOPar& iopar )
{
    DBKey dbky( ioobj_ ? ioobj_->key() : DBKey() );
    iopar.get( sKey::ID(), dbky );
    if ( dbky.isValid() )
	setOutput( dbky );
    else
    {
	const char* res = iopar.find( sKey::Name() );
	if ( res && *res )
	{
	    IOObj* tryioobj = DBM().getByName( IOObjContext::Seis, res );
	    if ( tryioobj )
		setIOObj( tryioobj );
	}
    }
    iopar.get( sKey::GeomID(), fixedgeomid_ );
}


Seis::SequentialStorer::SequentialStorer( Storer& strr, int maxqsz )
    : storer_(strr)
    , lock_(*new Threads::ConditionVar)
    , maxqueuesize_( maxqsz<1 ? Threads::getNrProcessors()*2 : maxqsz )
    , latestbid_( -1, -1 )
{
    queueid_ = Threads::WorkManager::twm().addQueue(
				    Threads::WorkManager::SingleThread,
				    "SequentalStorer");
}


bool Seis::SequentialStorer::finishWrite()
{
    if ( outputs_.size() )
	{ pErrMsg( "Buffer is not empty" ); deepErase( outputs_ ); }

    Threads::WorkManager::twm().emptyQueue( queueid_, true );

    return uirv_.isOK();
}


Seis::SequentialStorer::~SequentialStorer()
{
    if ( outputs_.size() )
	{ pErrMsg( "Buffer is not empty" ); deepErase( outputs_ ); }
    if ( Threads::WorkManager::twm().queueSize( queueid_ ) )
	{ pErrMsg("finishWrite was not called"); }
    Threads::WorkManager::twm().removeQueue( queueid_, false );

    delete &lock_;
}


bool Seis::SequentialStorer::announceTrace( const TrcKey& tk )
{
    return tk.is2D() ? announceTrace( BinID(tk.geomID().getI(),tk.trcNr()) )
		     : announceTrace( tk.position() );
}


bool Seis::SequentialStorer::announceTrace( Pos::GeomID gid,
					    Pos::TraceNr_Type tnr )
{
    return announceTrace( BinID(gid.lineNr(),tnr) );
}


bool Seis::SequentialStorer::announceTrace( const BinID& bid )
{
    Threads::MutexLocker lock( lock_ );
    if ( bid.inl()<latestbid_.inl() ||
	    (bid.inl()==latestbid_.inl() && bid.crl()<latestbid_.crl() ) )
	{ uirv_.set( tr("Announced trace is out of sequence") ); return false; }

    announcedtraces_ += bid;
    return true;
}


namespace Seis
{

class SingleTraceStorer : public ::Task
{
public:

SingleTraceStorer( SequentialStorer& ss, SeisTrc* trc )
    : ss_( ss ), trc_( trc )	{}

bool execute()
{
    const uiRetVal uirv = ss_.storer_.put( *trc_ );
    delete trc_;
    ss_.reportWrite( uirv );
    return uirv.isOK();
}

protected:

    SequentialStorer&	ss_;
    SeisTrc*		trc_;

};

} // namespace Seis


bool Seis::SequentialStorer::submitTrace( SeisTrc* inptrc, bool waitforbuffer )
{
    Threads::MutexLocker lock( lock_ );
    outputs_ += inptrc;
    return iterateBuffer( waitforbuffer );
}


bool Seis::SequentialStorer::iterateBuffer( bool waitforbuffer )
{
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
		const auto& ti = outputs_[idy]->info();
		const bool samepos = ti.lineNr() == bid.lineNr()
				  && ti.trcNr() == bid.trcNr();
		if ( samepos )
		{
		    trcs += outputs_.removeSingle( idy );
		    idy--;
		}
		else if ( !trcs.isEmpty() )
		    break;
	    }

	    if ( trcs.isEmpty() )
		{ idx--; break; }

	    for ( int idy=0; idy<trcs.size(); idy++ )
	    {
		Task* task = new SingleTraceStorer( *this, trcs[idy] );
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
	lock_.signal( true );

    if ( waitforbuffer )
    {
	int queuesz = outputs_.size() +
	    Threads::WorkManager::twm().queueSize( queueid_ );
	while ( queuesz>=maxqueuesize_ && uirv_.isOK() )
	{
	    lock_.wait();
	    queuesz = outputs_.size() +
			Threads::WorkManager::twm().queueSize( queueid_ );
	}
    }

    return uirv_.isOK();
}


void Seis::SequentialStorer::reportWrite( const uiRetVal& uirv )
{
    Threads::MutexLocker lock( lock_ );
    if ( !uirv.isOK() )
    {
	uirv_ = uirv;
        Threads::WorkManager::twm().emptyQueue( queueid_, false );
        lock_.signal( true );
        return;
    }

    const int queuesz = Threads::WorkManager::twm().queueSize( queueid_ ) +
                        outputs_.size();
    if ( queuesz<maxqueuesize_ )
        lock_.signal( true );
}
