/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 28-1-1998
 * FUNCTION : Seismic data reader
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seisread.h"
#include "seispsread.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seisbuf.h"
#include "seisbounds.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seisselectionimpl.h"
#include "executor.h"
#include "iostrm.h"
#include "streamconn.h"
#include "survinfo.h"
#include "surv2dgeom.h"
#include "keystrs.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "cubesampling.h"
#include "binidvalset.h"
#include "errh.h"
#include "file.h"
#include "iopar.h"


#define mUndefPtr(clss) ((clss*)0xdeadbeef) // Like on AIX. Nothing special.


SeisTrcReader::SeisTrcReader( const IOObj* ioob )
	: SeisStoreAccess(ioob)
    	, outer(mUndefPtr(HorSampling))
    	, fetcher(0)
    	, psrdr_(0)
    	, tbuf_(0)
    	, pscditer_(0)
    	, selcomp_(-1)
{
    init();
    if ( ioobj_ )
	entryis2d = SeisTrcTranslator::is2D( *ioob, false );
}



SeisTrcReader::SeisTrcReader( const char* fname )
	: SeisStoreAccess(fname,false,false)
    	, outer(mUndefPtr(HorSampling))
    	, fetcher(0)
    	, psrdr_(0)
	, pscditer_(0)
    	, tbuf_(0)
    	, selcomp_(-1)
{
    init();
}


#define mDelOuter if ( outer != mUndefPtr(HorSampling) ) delete outer

SeisTrcReader::~SeisTrcReader()
{
    mDelOuter; outer = 0;
    init();
    delete tbuf_;
}


void SeisTrcReader::init()
{
    foundvalidinl = foundvalidcrl = entryis2d =
    new_packet = inforead = needskip = prepared = forcefloats = false;
    prev_inl = mUdf(int);
    readmode = Seis::Prod;
    if ( tbuf_ ) tbuf_->deepErase();
    mDelOuter; outer = mUndefPtr(HorSampling);
    delete fetcher; fetcher = 0;
    delete psrdr_; psrdr_ = 0;
    delete pscditer_; pscditer_ = 0;
    nrfetchers = 0; curlineidx = -1;
}


bool SeisTrcReader::prepareWork( Seis::ReadMode rm )
{
    if ( !ioobj_ )
    {
	errmsg_ = "Info for input seismic data not found in Object Manager";
	return false;
    }
    else if ( psioprov_ )
    {
	const char* fnm = ioobj_->fullUserExpr(Conn::Read);
	if ( is2d_ )
	{
	    errmsg_ = "SeisTrcReader cannot read from 2D Pre-Stack data store";
	    return false;
	}
	psrdr_ = psioprov_->make3DReader( fnm );
    }
    if ( (is2d_ && !lset_) || (!is2d_ && !trl_) || (psioprov_ && !psrdr_) )
    {
	errmsg_ = "No data interpreter available for '";
	errmsg_ += ioobj_->name(); errmsg_ += "'";
	return false;
    }

    readmode = rm;
    if ( is2d_ || psioprov_ )
	return (prepared = true);

    Conn* conn = openFirst();
    if ( !conn )
    {
	errmsg_ = "Cannot open data files for '";
	errmsg_ += ioobj_->name(); errmsg_ += "'";
	return false;
    }

    if ( !initRead(conn) )
	return false;

    return (prepared = true);
}



void SeisTrcReader::startWork()
{
    outer = 0;
    if ( psioprov_ )
    {
	if ( !psrdr_ && !prepareWork(Seis::Prod) )
	    { pErrMsg("Huh"); return; }

	pscditer_ = new PosInfo::CubeDataIterator( psrdr_->posData() );
	if ( !pscditer_->next(curpsbid_) )
	    { errmsg_ = "Pre-stack data storage is empty"; return; }
	pscditer_->reset();
	return;
    }
    else if ( is2d_ )
	{ tbuf_ = new SeisTrcBuf( false ); return; }

    if ( !trl_ ) return;

    SeisTrcTranslator& sttrl = *strl();
    if ( forcefloats )
    {
	for ( int idx=0; idx<sttrl.componentInfo().size(); idx++ )
	    sttrl.componentInfo()[idx]->datachar = DataCharacteristics();
    }
    if ( selcomp_ >= 0 )
    {
	for ( int idx=0; idx<sttrl.componentInfo().size(); idx++ )
	    sttrl.componentInfo()[idx]->destidx = idx == selcomp_ ? 0 : -1;
    }

    sttrl.setSelData( seldata_ );
    if ( sttrl.inlCrlSorted() && seldata_ && !seldata_->isAll() )
    {
	outer = new HorSampling;
	outer->set( seldata_->inlRange(), seldata_->crlRange() );
    }

    if ( !sttrl.commitSelections() )
	{ errmsg_ = sttrl.errMsg(); return; }
}


bool SeisTrcReader::isMultiConn() const
{
    return !psioprov_ && !is2d_ && !entryis2d
	&& ioobj_ && ioobj_->hasConnType(StreamConn::sType())
	&& ((IOStream*)ioobj_)->multiConn();
}


Conn* SeisTrcReader::openFirst()
{
    mDynamicCastGet(IOStream*,iostrm,ioobj_)
    if ( iostrm )
	iostrm->setConnNr( iostrm->fileNumbers().start );

    Conn* conn = ioobj_->getConn( Conn::Read );
    const char* fnm = ioobj_->fullUserExpr( Conn::Read );
    if ( !conn || (conn->bad() && !File::isDirectory(fnm)) )
    {
	delete conn; conn = 0;
	if ( iostrm && isMultiConn() )
	{
	    while ( !conn || conn->bad() )
	    {
		delete conn; conn = 0;
		if ( !iostrm->toNextConnNr() ) break;

		conn = ioobj_->getConn( Conn::Read );
	    }
	}
    }
    return conn;
}


bool SeisTrcReader::initRead( Conn* conn )
{
    if ( !trl_ )
	{ pErrMsg("Should be a translator there"); return false; }

    mDynamicCastGet(SeisTrcTranslator*,sttrl,trl_)
    if ( !sttrl )
    {
	errmsg_ = trl_->userName();
	errmsg_ +=  "found where seismic cube was expected";
	cleanUp(); return false;
    }

    if ( !sttrl->initRead(conn,readmode) )
    {
	errmsg_ = sttrl->errMsg();
	cleanUp(); return false;
    }
    const int nrcomp = sttrl->componentInfo().size();
    if ( nrcomp < 1 )
    {
	// Why didn't the translator return false?
	errmsg_ = "Internal: no data components found";
	cleanUp(); return false;
    }

    // Make sure the right component(s) are selected.
    // If all else fails, take component 0.
    bool foundone = false;
    for ( int idx=0; idx<nrcomp; idx++ )
    {
	if ( sttrl->componentInfo()[idx]->destidx >= 0 )
	    { foundone = true; break; }
    }
    if ( !foundone )
    {
	for ( int idx=0; idx<nrcomp; idx++ )
	{
	    if ( selcomp_ == -1 )
		sttrl->componentInfo()[idx]->destidx = idx;
	    else
		sttrl->componentInfo()[idx]->destidx = selcomp_ == idx ? 0 : 1;
	    if ( sttrl->componentInfo()[idx]->destidx >= 0 )
		foundone = true;
	}
	if ( !foundone )
	    sttrl->componentInfo()[0]->destidx = 0;
    }

    needskip = false;
    return true;
}


int SeisTrcReader::get( SeisTrcInfo& ti )
{
    if ( !prepared && !prepareWork(readmode) )
	return -1;
    else if ( outer == mUndefPtr(HorSampling) )
	startWork();

    if ( is2d_ )
	return get2D(ti);
    if ( psioprov_ )
	return getPS(ti);

    SeisTrcTranslator& sttrl = *strl();
    bool needsk = needskip; needskip = false;
    if ( needsk && !sttrl.skip() )
	return nextConn( ti );

    if ( !sttrl.readInfo(ti) )
    {
	const char* emsg = sttrl.errMsg();
	if ( emsg && *emsg )
	    { errmsg_ = emsg; return -1; }
	return nextConn( ti );
    }

    ti.new_packet = false;

    if ( mIsUdf(prev_inl) )
	prev_inl = ti.binid.inl;
    else if ( prev_inl != ti.binid.inl )
    {
	foundvalidcrl = false;
	prev_inl = ti.binid.inl;
	if ( !entryis2d )
	    ti.new_packet = true;
    }

    int selres = 0;
    if ( seldata_ )
    {
	if ( !entryis2d )
	    selres = seldata_->selRes(ti.binid);
	else
	{
	    BinID bid( seldata_->inlRange().start, ti.nr );
	    selres = seldata_->selRes( bid );
	}
    }

    if ( selres / 256 == 0 )
	foundvalidcrl = true;
    if ( selres % 256 == 0 )
	foundvalidinl = true;

    if ( selres )
    {
	if ( !entryis2d && sttrl.inlCrlSorted() )
	{
	    bool neednewinl = outer && !outer->includes(ti.binid);
	    if ( neednewinl )
	    {
		mDynamicCastGet(IOStream*,iostrm,ioobj_)
		if ( iostrm && iostrm->isMulti() )
		    return nextConn(ti);
	    }
	}

	return sttrl.skip() ? 2 : nextConn( ti );
    }

    nrtrcs_++;
    if ( new_packet )
    {
	ti.new_packet = true;
	new_packet = false;
    }
    needskip = true;
    return 1;
}


static void reduceComps( SeisTrc& trc, int selcomp )
{
    const int orgnrcomps = trc.nrComponents();
    if ( selcomp < 0 || orgnrcomps < 2 ) return;
    if ( selcomp >= orgnrcomps )
	selcomp = orgnrcomps-1;

    TraceData& td( trc.data() );
    for ( int idx=0; idx<selcomp; idx++ )
	td.delComponent( 0 );
    for ( int idx=selcomp+1; idx<orgnrcomps; idx++ )
	td.delComponent( 1 );
}


bool SeisTrcReader::get( SeisTrc& trc )
{
    needskip = false;
    if ( !prepared && !prepareWork(readmode) )
	return false;
    else if ( outer == mUndefPtr(HorSampling) )
	startWork();
    if ( is2d_ )
	return get2D(trc);
    if ( psioprov_ )
	return getPS(trc);

    if ( !strl()->read(trc) )
    {
	errmsg_ = strl()->errMsg();
	strl()->skip();
	return false;
    }

    reduceComps( trc, selcomp_ );
    return true;
}


int SeisTrcReader::getPS( SeisTrcInfo& ti )
{
    if ( !psrdr_ ) return 0;

    if ( !tbuf_ )
	tbuf_ = new SeisTrcBuf( false );

    if ( tbuf_->isEmpty() )
    {
	int selres = 2;
	while ( selres % 256 == 2 )
	{
	    if ( !pscditer_->next(curpsbid_) )
	    {
		delete psrdr_; psrdr_ = 0;
		return 0;
	    }
	    selres = seldata_ ? seldata_->selRes( curpsbid_ ) : 0;
	}

	if ( seldata_ && !seldata_->isOK(curpsbid_) )
	    return 2;

	if ( !psrdr_->getGather(curpsbid_,*tbuf_) )
	    { errmsg_ = psrdr_->errMsg(); return -1; }
    }

    ti = tbuf_->get(0)->info();
    inforead = true;
    return 1;
}


bool SeisTrcReader::getPS( SeisTrc& trc )
{
    if ( !inforead && getPS(trc.info()) <= 0 )
	return false;

    if ( tbuf_->isEmpty() )
    {
	while ( true )
	{
	    int rdres = getPS(trc.info());
	    if ( rdres == 1 )
		break;
	    if ( rdres > 1 )
		continue;
	    return rdres == 0;
	}
    }

    inforead = false;
    const SeisTrc* buftrc = tbuf_->get( 0 );
    if ( !buftrc )
	{ pErrMsg("Huh"); return false; }
    trc.info() = buftrc->info();
    trc.copyDataFrom( *buftrc, -1, forcefloats );

    delete tbuf_->remove(0);
    reduceComps( trc, selcomp_ );
    nrtrcs_++;
    return true;
}


LineKey SeisTrcReader::lineKey() const
{
    if ( lset_ )
    {
	if ( curlineidx >= 0 && lset_->nrLines() > curlineidx )
	    return lset_->lineKey( curlineidx );
    }
    if ( seldata_ )
	return seldata_->lineKey();
    else if ( ioobj_ )
	return LineKey(ioobj_->name(),ioobj_->pars().find(sKey::Attribute()));

    return LineKey(0,0);
}


class SeisTrcReaderLKProv : public LineKeyProvider
{
public:
    SeisTrcReaderLKProv( const SeisTrcReader& r )
				: rdr(r) {}
    LineKey lineKey() const	{ return rdr.lineKey(); }
    const SeisTrcReader&	rdr;
};


LineKeyProvider* SeisTrcReader::lineKeyProvider() const
{
    return new SeisTrcReaderLKProv( *this );
}


bool SeisTrcReader::ensureCurLineAttribOK( const BufferString& attrnm )
{
    const int nrlines = lset_->nrLines();
    while ( curlineidx < nrlines )
    {
	if ( lset_->lineKey(curlineidx).attrName() == attrnm )
	    break;
	curlineidx++;
    }

    bool ret = curlineidx < nrlines;
    if ( !ret && nrfetchers < 1 )
	errmsg_ = "No line found matching selection";
    return ret;
}


bool SeisTrcReader::mkNextFetcher()
{
    curlineidx++; tbuf_->deepErase();
    LineKey lk( seldata_ ? seldata_->lineKey() : "" );
    const BufferString attrnm = lk.attrName();
    const bool islinesel = !lk.lineName().isEmpty();
    const bool istable = seldata_ && seldata_->type() == Seis::Table;
    const int nrlines = lset_->nrLines();

    if ( !islinesel )
    {
	if ( !ensureCurLineAttribOK(attrnm) )
	    return false;

	if ( istable )
	{
	    // Chances are we do not need to go through this line at all
	    mDynamicCastGet(Seis::TableSelData*,tsd,seldata_)
	    while ( !lset_->haveMatch(curlineidx,tsd->binidValueSet()) )
	    {
	    	curlineidx++;
		if ( !ensureCurLineAttribOK(attrnm) )
		    return false;
	    }
	}
    }
    else
    {
	if ( nrfetchers > 0 )
	{ errmsg_ = ""; return false; }

	bool found = false;
	for ( ; curlineidx<nrlines; curlineidx++ )
	{
	    if ( lk == lset_->lineKey(curlineidx) )
		{ found = true; break; }
	}
	if ( !found )
	{
	    errmsg_ = "Line key not found in line set: ";
	    errmsg_ += seldata_->lineKey();
	    return false;
	}
    }

    StepInterval<float> zrg;
    lset_->getRanges( curlineidx, curtrcnrrg, zrg );
    if ( seldata_ && !seldata_->isAll() && seldata_->type() == Seis::Range )
    {
	if ( seldata_->crlRange().start > curtrcnrrg.start )
	    curtrcnrrg.start = seldata_->crlRange().start;
	if ( seldata_->crlRange().stop < curtrcnrrg.stop )
	    curtrcnrrg.stop = seldata_->crlRange().stop;
    }

    prev_inl = mUdf(int);
    fetcher = lset_->lineFetcher( curlineidx, *tbuf_, 1, seldata_ );
    nrfetchers++;
    return fetcher;
}


bool SeisTrcReader::readNext2D()
{
    if ( tbuf_->size() )
	tbuf_->deepErase();

    int res = fetcher->doStep();
    if ( res == Executor::ErrorOccurred() )
    {
	errmsg_ = fetcher->message();
	return false;
    }
    else if ( res == 0 )
    {
	if ( !mkNextFetcher() )
	    return false;
	return readNext2D();
    }

    return tbuf_->size();
}


#define mNeedNextFetcher() (tbuf_->size() == 0 && !fetcher)


int SeisTrcReader::get2D( SeisTrcInfo& ti )
{
    if ( !fetcher && !mkNextFetcher() )
	return errmsg_.isEmpty() ? 0 : -1;

    if ( !readNext2D() )
	return errmsg_.isEmpty() ? 0 : -1;

    inforead = true;
    SeisTrcInfo& trcti = tbuf_->get( 0 )->info();
    trcti.new_packet = mIsUdf(prev_inl);
    ti = trcti;
    prev_inl = 0;

    bool isincl = true;
    if ( seldata_ )
    {
	if ( seldata_->type() == Seis::Table && !seldata_->isAll() )
	    // Not handled by fetcher
	{
	    mDynamicCastGet(Seis::TableSelData*,tsd,seldata_)
	    isincl = tsd->binidValueSet().includes(trcti.binid);
	}
    }
    return isincl ? 1 : 2;
}


bool SeisTrcReader::get2D( SeisTrc& trc )
{
    if ( !inforead && get2D(trc.info())<=0 )
	return false;

    inforead = false;
    const SeisTrc* buftrc = tbuf_->get( 0 );
    if ( !buftrc )
	{ pErrMsg("Huh"); return false; }
    trc.info() = buftrc->info();
    trc.copyDataFrom( *buftrc, -1, forcefloats );

    delete tbuf_->remove(0);
    reduceComps( trc, selcomp_ );
    nrtrcs_++;
    return true;
}


int SeisTrcReader::nextConn( SeisTrcInfo& ti )
{
    new_packet = false;
    if ( !isMultiConn() ) return 0;

    // Multiconn is only used for multi-machine data collection nowadays
    strl()->cleanUp(); setSelData( 0 );
    IOStream* iostrm = (IOStream*)ioobj_;
    if ( !iostrm->toNextConnNr() )
	return 0;

    Conn* conn = iostrm->getConn( Conn::Read );

    while ( !conn || conn->bad() )
    {
	delete conn; conn = 0;
	if ( !iostrm->toNextConnNr() ) return 0;

	conn = iostrm->getConn( Conn::Read );
    }

    if ( !strl()->initRead(conn) )
    {
	errmsg_ = strl()->errMsg();
	return -1;
    }

    const int rv = get( ti );
    if ( rv < 1 )	return rv;
    else if ( rv == 2 )	new_packet = true;
    else		ti.new_packet = true;
    return rv;
}



void SeisTrcReader::fillPar( IOPar& iopar ) const
{
    SeisStoreAccess::fillPar( iopar );
    if ( seldata_ )	seldata_->fillPar( iopar );
    else		Seis::SelData::removeFromPar( iopar );
}


Seis::Bounds* SeisTrcReader::get3DBounds( const StepInterval<int>& inlrg,
					  const StepInterval<int>& crlrg,
					  const StepInterval<float>& zrg ) const
{
    Seis::Bounds3D* b3d = new Seis::Bounds3D;
    b3d->cs_.hrg.start.inl = inlrg.start;
    b3d->cs_.hrg.start.crl = crlrg.start;
    b3d->cs_.hrg.stop.inl = inlrg.stop;
    b3d->cs_.hrg.stop.crl = crlrg.stop;
    b3d->cs_.hrg.step.inl = inlrg.step;
    b3d->cs_.hrg.step.crl = crlrg.step;

    if ( b3d->cs_.hrg.step.inl < 0 )
    {
	pErrMsg("Poss prob: negative inl step from transl");
	b3d->cs_.hrg.step.inl = -b3d->cs_.hrg.step.inl;
    }
    if ( b3d->cs_.hrg.step.crl < 0 )
    {
	pErrMsg("Poss prob: negative crl step from transl");
	b3d->cs_.hrg.step.crl = -b3d->cs_.hrg.step.crl;
    }
    b3d->cs_.zrg = zrg;

    if ( !seldata_ || seldata_->isAll() )
	return b3d;

#define mChkRg(dir) \
    const Interval<int> dir##rng( seldata_->dir##Range() ); \
    if ( b3d->cs_.hrg.start.dir < dir##rng.start ) \
	b3d->cs_.hrg.start.dir = dir##rng.start; \
    if ( b3d->cs_.hrg.stop.dir > dir##rng.stop ) \
	b3d->cs_.hrg.stop.dir = dir##rng.stop;

    mChkRg(inl)
    mChkRg(crl)

    const Interval<float> zrng( seldata_->zRange() );
    if ( b3d->cs_.zrg.start < zrng.start ) b3d->cs_.zrg.start = zrng.start;
    if ( b3d->cs_.zrg.stop > zrng.stop ) b3d->cs_.zrg.stop = zrng.stop;

    return b3d;
}


bool SeisTrcReader::initBounds2D( const PosInfo::Line2DData& l2dd,
				  Seis::Bounds2D& b2d ) const
{
    b2d.zrg_ = l2dd.zRange();

    const TypeSet<PosInfo::Line2DPos>& posns = l2dd.positions();
    int prevnr = posns[0].nr_;
    bool havefoundaselected = false;
    b2d.nrrg_.step = mUdf(int);

    for ( int idx=0; idx<posns.size(); idx++ )
    {
	const int curnr = posns[idx].nr_;
	if ( idx != 0 )
	{
	    const int step = abs( curnr - prevnr );
	    if ( step > 0 && step < b2d.nrrg_.step )
		b2d.nrrg_.step = step;
	}

	if ( !havefoundaselected )
	{
	    b2d.nrrg_.start = b2d.nrrg_.stop = curnr;
	    b2d.mincoord_ = b2d.maxcoord_ = posns[idx].coord_;
	    havefoundaselected = true;
	}

	if ( b2d.nrrg_.step == 1 && havefoundaselected )
	    return true;
    }

    return havefoundaselected;
}


Seis::Bounds* SeisTrcReader::getBounds() const
{
    if ( isPS() )
    {
	if ( !ioobj_ ) return 0;
	if ( is2D() ) return 0; //TODO 2D pre-stack
	SeisPSReader* r = SPSIOPF().get3DReader( *ioobj_ );
	mDynamicCastGet(SeisPS3DReader*,rdr,r)
	if ( !rdr ) return 0;
	const PosInfo::CubeData& cd = rdr->posData();
	StepInterval<int> inlrg, crlrg;
	cd.getInlRange( inlrg ); cd.getInlRange( crlrg );
	return get3DBounds( inlrg, crlrg, SI().sampling(false).zrg );
    }
    if ( !is2D() )
    {
	if ( !trl_ ) return 0;
	return get3DBounds( strl()->packetInfo().inlrg,
			strl()->packetInfo().crlrg, strl()->packetInfo().zrg );
    }

    if ( !lset_ || lset_->nrLines() < 1 )
	return 0;

    Seis::Bounds2D* b2d = new Seis::Bounds2D;

    for ( int iiter=0; iiter<2; iiter++ ) // iiter == 0 is initialisation
    {
    
    S2DPOS().setCurLineSet( lset_->name() );
    for ( int iln=0; iln<lset_->nrLines(); iln++ )
    {
	if ( seldata_ && !seldata_->lineKey().isEmpty()
	  && seldata_->lineKey() != lset_->lineKey(iln) )
	    continue;

	LineKey lk = seldata_ ? seldata_->lineKey() :  lset_->lineKey( iln );
	PosInfo::Line2DData l2dd( lk.lineName() );
	if ( !S2DPOS().getGeometry(l2dd) )
	    continue;
	const TypeSet<PosInfo::Line2DPos>& posns = l2dd.positions();
	if ( posns.size() < 2 )
	    continue;

	if ( iiter == 0 )
	{
	    if ( initBounds2D(l2dd,*b2d) )
		break;
	    else
		continue;
	}

	for ( int idx=0; idx<posns.size(); idx++ )
	{
	    const int nr = posns[idx].nr_;
	    if ( b2d->nrrg_.start > nr ) b2d->nrrg_.start = nr;
	    else if ( b2d->nrrg_.stop < nr ) b2d->nrrg_.stop = nr;
	    const Coord c( posns[idx].coord_ );
	    if ( b2d->mincoord_.x > c.x ) b2d->mincoord_.x = c.x;
	    else if ( b2d->maxcoord_.x < c.x ) b2d->maxcoord_.x = c.x;
	    if ( b2d->mincoord_.y > c.y ) b2d->mincoord_.y = c.y;
	    else if ( b2d->maxcoord_.y < c.y ) b2d->maxcoord_.y = c.y;
	} // each position
    } // each line
    } // iiter = 0 or 1

    return b2d;
}
