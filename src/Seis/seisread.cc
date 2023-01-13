/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisread.h"

#include "seis2dlineio.h"
#include "seisbounds.h"
#include "seisbuf.h"
#include "seispacketinfo.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seis2ddata.h"

#include "binidvalset.h"
#include "executor.h"
#include "file.h"
#include "iopar.h"
#include "iostrm.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "scaler.h"
#include "streamconn.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "trckeyzsampling.h"


TrcKeySampling& getUdfTks()
{
    static PtrMan<TrcKeySampling> udftks = new TrcKeySampling( false );
    return *udftks.ptr();
}


SeisTrcReader::SeisTrcReader( const MultiID& dbkey, Seis::GeomType gt )
    : SeisStoreAccess(dbkey,gt)
    , outer(&getUdfTks())
{
    init();
}


SeisTrcReader::SeisTrcReader( const IOObj& ioobj, const Seis::GeomType* gt )
    : SeisStoreAccess(&ioobj,gt)
    , outer(&getUdfTks())
{
    init();
}


SeisTrcReader::SeisTrcReader( const IOObj& ioobj, Pos::GeomID geomid,
			      const Seis::GeomType* gt )
    : SeisStoreAccess(&ioobj,geomid,gt)
    , outer(&getUdfTks())
{
    init();
}


SeisTrcReader::SeisTrcReader( const SeisStoreAccess::Setup& su )
    : SeisStoreAccess(su)
    , outer(&getUdfTks())
{
    init();
}


SeisTrcReader::SeisTrcReader( const IOObj* ioobj )
    : SeisStoreAccess(ioobj,nullptr)
    , outer(&getUdfTks())
{
    init();
}


SeisTrcReader::SeisTrcReader( const char* fname )
    : SeisTrcReader(SeisStoreAccess::getTmp(fname,false,false))
{
}


SeisTrcReader::~SeisTrcReader()
{
    if ( outer != &getUdfTks() )
	delete outer;

    if ( tbuf_ )
	tbuf_->deepErase();

    delete tbuf_;
    delete fetcher;
    delete psrdr2d_;
    delete psrdr3d_;
    delete pscditer_;
    delete pslditer_;
}


void SeisTrcReader::init()
{
    if ( ioobj_ )
	entryis2d = SeisTrcTranslator::is2D( *ioobj_ );
}


bool SeisTrcReader::prepareWork( Seis::ReadMode rm )
{
    if ( !isOK() )
	return false;
    else if ( !ioobj_ )
    {
	errmsg_ = tr("Info for input seismic data not found in Object Manager");
	return false;
    }
    else if ( psioprov_ )
    {
	if ( is2d_ )
	{
	    const Pos::GeomID geomid = SeisStoreAccess::geomID();
	    if ( !Survey::is2DGeom(geomid) )
		{ errmsg_ = tr("No line geometry ID set"); return false; }
	    psrdr2d_ = psioprov_->get2DReader( *ioobj_, geomid );
	}
	else
	    psrdr3d_ = psioprov_->get3DReader( *ioobj_ );
    }

    const bool is3dfail = !is2d_ && !entryis2d && !trl_;
    const bool is2dfail = (is2d_ || entryis2d) && !psioprov_ && !dataset_;
    const bool ispsfail = psioprov_ && !psrdr2d_ && !psrdr3d_;
    if ( is3dfail && is2dfail && ispsfail )
    {
	if ( errmsg_.isEmpty() )
	    errmsg_ = tr("No data interpreter available for '%1'")
		    .arg(ioobj_->name());
	return false;
    }

    readmode = rm;
    if ( is2d_ || psioprov_ )
	return (prepared_ = true);

    Conn* conn = openFirst();
    if ( !conn )
    {
	errmsg_ = tr("Cannot open data files for '%1'")
		.arg(ioobj_->name());
	return false;
    }

    if ( !initRead(conn) )
	return false;

    return (prepared_ = true);
}


int SeisTrcReader::expectedNrTraces() const
{
    if ( !isPrepared() &&
	    !const_cast<SeisTrcReader*>(this)->prepareWork(Seis::Prod) )
	return -1;

    int totnr = 0;
    const Seis::SelData* sd = selData();
    if ( sd && !sd->isAll() )
    {
	totnr += sd->expectedNrTraces( entryis2d  );
	if ( isPS() )
	{
	    const int nroffsets = getNrOffsets();
	    if ( !mIsUdf(nroffsets) )
		totnr *= nroffsets;
	}

	return totnr;
    }

    const SeisTrcTranslator* strl = SeisStoreAccess::strl();
    if ( strl && entryis2d )
    {
	const SeisTrcTranslator* strl2d =
			const_cast<SeisTrcReader*>( this )->seis2Dtranslator();
	if ( strl2d )
	    strl = strl2d;
    }

    if ( strl )
	totnr += strl->estimatedNrTraces();
    else if ( isPS() )
    {
	TrcKeySampling tks( true );
	if ( entryis2d )
	    tks.init( geomID() );

	totnr = tks.totalNr();
	if ( isPS() )
	{
	    const int nroffsets = getNrOffsets();
	    if ( !mIsUdf(nroffsets) )
		totnr *= nroffsets;
	}
    }
    else
	totnr = -1;

    return totnr;
}


bool SeisTrcReader::startWork()
{
    if ( outer && outer != &getUdfTks() )
	delete outer;

    outer = nullptr;
    if ( psioprov_ )
    {
	if ( !psrdr2d_ && !psrdr3d_ && !prepareWork(Seis::Prod) )
	    { pErrMsg("Huh"); return false; }

	if ( psrdr3d_ )
	{
	    pscditer_ = new PosInfo::CubeDataIterator( psrdr3d_->posData() );
	    if ( !pscditer_->next(curpsbid_) )
	    {
		errmsg_ = tr("3D Prestack Data storage is empty");
		return false;
	    }
	    pscditer_->reset();
	}
	else if ( psrdr2d_ )
	{
	    pslditer_ = new PosInfo::Line2DDataIterator( psrdr2d_->posData() );
	    if ( !pslditer_->next() )
	    {
		errmsg_ = tr("2D Prestack Data storage is empty");
		return false;
	    }
	    pslditer_->reset();
	}
	else
	    { pErrMsg("Huh"); return false; }

	return true;
    }
    else if ( is2d_ )
	{ tbuf_ = new SeisTrcBuf( false ); return true; }

    // 3D cubes from here
    if ( !trl_ )
	return false;

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
	outer = new TrcKeySampling;
	outer->set( seldata_->inlRange(), seldata_->crlRange() );
    }

    if ( !sttrl.commitSelections() )
	{ errmsg_ = sttrl.errMsg(); return false; }

    return true;
}


bool SeisTrcReader::isMultiConn() const
{
    return !psioprov_ && !is2d_ && !entryis2d
	&& ioobj_ && ioobj_->hasConnType(StreamConn::sType())
	&& ((IOStream*)ioobj_)->isMultiConn();
}


Conn* SeisTrcReader::openFirst()
{
    mDynamicCastGet(IOStream*,iostrm,ioobj_)
    if ( iostrm )
	iostrm->resetConnIdx();

    Conn* conn = ioobj_->getConn( Conn::Read );
    const char* fnm = ioobj_->fullUserExpr( Conn::Read );
    if ( !conn || (conn->isBad() && !File::isDirectory(fnm)) )
    {
	delete conn; conn = 0;
	if ( iostrm && isMultiConn() )
	{
	    while ( !conn || conn->isBad() )
	    {
		delete conn; conn = 0;
		if ( !iostrm->toNextConnIdx() ) break;

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

    SeisTrcTranslator* sttrl = strl();
    if ( !sttrl )
    {
	errmsg_ = tr("%1 found where seismic cube was expected")
		.arg(trl_->userName());
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
	errmsg_ = toUiString("Internal: no data components found");
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
    if ( !prepared_ && !prepareWork(readmode) )
	return -1;
    else if ( outer == &getUdfTks() && !startWork() )
	return -1;

    if ( psioprov_ )
	return getPS(ti);
    else if ( is2d_ )
	return get2D(ti);


    // 3D post-stack

    SeisTrcTranslator& sttrl = *strl();
    bool needsk = needskip; needskip = false;
    if ( needsk && !sttrl.skip() )
	return nextConn( ti );

    if ( !sttrl.readInfo(ti) )
    {
	const uiString emsg = sttrl.errMsg();
	if ( emsg.isSet() )
	    { errmsg_ = emsg; return -1; }
	return nextConn( ti );
    }

    ti.new_packet = false;

    if ( mIsUdf(prev_inl) )
	prev_inl = ti.inl();
    else if ( prev_inl != ti.inl() )
    {
	foundvalidcrl = false;
	prev_inl = ti.inl();
	if ( !entryis2d )
	    ti.new_packet = true;
    }

    int selres = 0;
    if ( seldata_ )
    {
	if ( entryis2d )
	    selres = seldata_->selRes( ti.geomID(), ti.trcNr() );
	else
	    selres = seldata_->selRes( ti.binID() );
    }

    if ( selres / 256 == 0 )
	foundvalidcrl = true;
    if ( selres % 256 == 0 )
	foundvalidinl = true;

    if ( selres )
    {
	if ( !entryis2d && sttrl.inlCrlSorted() )
	{
	    bool neednewinl = outer && !outer->includes(ti.binID());
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


bool SeisTrcReader::getData( TraceData& data )
{
    needskip = false;
    if ( !prepared_ && !prepareWork(readmode) )
	return false;
    else if ( outer == &getUdfTks() && !startWork() )
	return false;

    if ( psioprov_ )
	return false;
    else if ( is2d_ )
	return get2DData( data );

    if ( !strl()->readData(&data) )
    {
	errmsg_ = strl()->errMsg();
	strl()->skip();
	return false;
    }

    return true;
}


bool SeisTrcReader::getDataPack( RegularSeisDataPack& sdp, TaskRunner* taskr )
{
    needskip = false;
    if ( !prepared_ && !prepareWork(readmode) )
	return false;
    else if ( outer == &getUdfTks() && !startWork() )
	return false;

    if ( psioprov_ || is2D() )
	return false;

    if ( !strl()->readDataPack(sdp,taskr) )
    {
	errmsg_ = strl()->errMsg();
	return false;
    }

    return true;
}


bool SeisTrcReader::get( SeisTrc& trc )
{
    needskip = false;
    if ( !prepared_ && !prepareWork(readmode) )
	return false;
    else if ( outer == &getUdfTks() && !startWork() )
	return false;

    if ( psioprov_ )
	return getPS(trc);
    if ( is2d_ )
	return get2D(trc);

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
    if ( !psrdr2d_ && !psrdr3d_ )
	return 0;

    if ( !tbuf_ )
	tbuf_ = new SeisTrcBuf( false );

    if ( tbuf_->isEmpty() )
    {
	if ( psrdr3d_ )
	{
	    int selres = 2;
	    while ( selres % 256 == 2 )
	    {
		if ( !pscditer_->next(curpsbid_) )
		{
		    deleteAndNullPtr( psrdr3d_ );
		    return 0;
		}
		selres = seldata_ ? seldata_->selRes( curpsbid_ ) : 0;
	    }

	    if ( seldata_ && !seldata_->isOK(curpsbid_) )
		return 2;

	    if ( !psrdr3d_->getGather(curpsbid_,*tbuf_) )
		{ errmsg_ = psrdr3d_->errMsg(); return -1; }
	}
	else
	{
	    bool isok = false;
	    const Pos::GeomID gid = geomID();
	    int trcnr = 0;
	    while ( !isok )
	    {
		if ( !pslditer_->next() )
		{
		    deleteAndNullPtr( psrdr2d_ );
		    return 0;
		}

		trcnr = pslditer_->trcNr();
		isok = !seldata_ || seldata_->isOK(gid,trcnr);
	    }

	    if ( !psrdr2d_->getGath(trcnr,*tbuf_) )
		{ errmsg_ = psrdr2d_->errMsg(); return -1; }
	}
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


const Scaler* SeisTrcReader::getTraceScaler() const
{
    if ( psioprov_ || is2d_ )
	return nullptr;

    return strl() ? strl()->curtrcscalebase_ : nullptr;
}


Pos::GeomID SeisTrcReader::geomID() const
{
    if ( dataset_ )
    {
	if ( curlineidx >= 0 && dataset_->nrLines() > curlineidx )
	    return dataset_->geomID( curlineidx );
    }

    return SeisStoreAccess::geomID();
}


class SeisTrcReaderLKProv : public GeomIDProvider
{
public:
    SeisTrcReaderLKProv( const SeisTrcReader& r )
				: rdr(r) {}
    Pos::GeomID geomID() const override		{ return rdr.geomID(); }
    const SeisTrcReader&	rdr;
};


GeomIDProvider* SeisTrcReader::geomIDProvider() const
{
    return new SeisTrcReaderLKProv( *this );
}


bool SeisTrcReader::mkNextFetcher()
{
    curlineidx++;
    if ( tbuf_ )
	tbuf_->deepErase();

    const Pos::GeomID geomid = SeisStoreAccess::geomID();
    const bool islinesel = geomid.isValid() && geomid.is2D();
    const bool istable = seldata_ && seldata_->type() == Seis::Table;
    const int nrlines = dataset_->nrLines();

    if ( !islinesel )
    {
	if ( istable )
	{
	    // Chances are we do not need to go through this line at all
	    mDynamicCastGet(Seis::TableSelData*,tsd,seldata_)
	    Pos::GeomID curgeomid = dataset_->geomID( curlineidx );
	    while ( !dataset_->haveMatch(curgeomid,tsd->binidValueSet()) )
	    {
		curlineidx++;
		curgeomid = dataset_->geomID( curlineidx );
		if ( curlineidx >= nrlines )
		    return false;
	    }
	}
    }
    else
    {
	if ( nrfetchers > 0 )
	{ errmsg_ = uiString::emptyString(); return false; }

	bool found = false;
	for ( ; curlineidx<nrlines; curlineidx++ )
	{
	    if ( geomid == dataset_->geomID(curlineidx) )
		{ found = true; break; }
	}
	if ( !found )
	{
	    errmsg_ = tr("Data not available for the selected line");
	    return false;
	}
    }

    StepInterval<float> zrg;
    dataset_->getRanges( dataset_->geomID(curlineidx), curtrcnrrg, zrg );
    if ( seldata_ && !seldata_->isAll() && seldata_->type() == Seis::Range )
    {
	if ( seldata_->crlRange().start > curtrcnrrg.start )
	    curtrcnrrg.start = seldata_->crlRange().start;
	if ( seldata_->crlRange().stop < curtrcnrrg.stop )
	    curtrcnrrg.stop = seldata_->crlRange().stop;
    }

    if ( !tbuf_ && !startWork() )
	return false;

    prev_inl = mUdf(int);
    delete fetcher;
    fetcher = dataset_->lineFetcher( dataset_->geomID(curlineidx),
				     *tbuf_, 1, seldata_ );
    nrfetchers++;
    return fetcher;
}


bool SeisTrcReader::readNext2D()
{
    if ( !tbuf_->isEmpty() )
	tbuf_->deepErase();

    const int res = fetcher->doStep();
    if ( res == SequentialTask::ErrorOccurred() )
    {
	errmsg_ = fetcher->uiMessage();
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


const SeisTrcTranslator* SeisTrcReader::seis2Dtranslator()
{
    if ( !fetcher && !mkNextFetcher() )
	return nullptr;

    mDynamicCastGet(const Seis2DLineGetter*,getter2d,fetcher)
    if ( !getter2d )
	return nullptr;

    return getter2d->translator();
}


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
	    isincl = tsd->binidValueSet().includes(trcti.binID());
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


bool SeisTrcReader::get2DData( TraceData& data )
{
    SeisTrcInfo trcinfo;
    if ( !inforead && get2D(trcinfo)<=0 )
	return false;

    inforead = false;
    const SeisTrc* buftrc = tbuf_->get( 0 );
    if ( !buftrc )
	{ pErrMsg("Huh"); return false; }

    data.copyFrom( buftrc->data() );

    delete tbuf_->remove(0);
    nrtrcs_++;
    return true;
}


int SeisTrcReader::nextConn( SeisTrcInfo& ti )
{
    new_packet = false;
    if ( !isMultiConn() ) return 0;

    // Multiconn is only used for multi-machine data collection nowadays
    strl()->cleanUp();
    setSelData( nullptr );
    IOStream* iostrm = (IOStream*)ioobj_;
    if ( !iostrm->toNextConnIdx() )
	return 0;

    Conn* conn = iostrm->getConn( Conn::Read );

    while ( !conn || conn->isBad() )
    {
	delete conn; conn = 0;
	if ( !iostrm->toNextConnIdx() ) return 0;

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
    b3d->tkzs_.hsamp_.start_.inl() = inlrg.start;
    b3d->tkzs_.hsamp_.start_.crl() = crlrg.start;
    b3d->tkzs_.hsamp_.stop_.inl() = inlrg.stop;
    b3d->tkzs_.hsamp_.stop_.crl() = crlrg.stop;
    b3d->tkzs_.hsamp_.step_.inl() = inlrg.step;
    b3d->tkzs_.hsamp_.step_.crl() = crlrg.step;

    if ( b3d->tkzs_.hsamp_.step_.inl() < 0 )
    {
	pErrMsg("Poss prob: negative inl step from transl");
	b3d->tkzs_.hsamp_.step_.inl() = -b3d->tkzs_.hsamp_.step_.inl();
    }
    if ( b3d->tkzs_.hsamp_.step_.crl() < 0 )
    {
	pErrMsg("Poss prob: negative crl step from transl");
	b3d->tkzs_.hsamp_.step_.crl() = -b3d->tkzs_.hsamp_.step_.crl();
    }
    b3d->tkzs_.zsamp_ = zrg;

    if ( !seldata_ || seldata_->isAll() )
	return b3d;

#define mChkRg(dir) \
    const Interval<int> dir##rng( seldata_->dir##Range() ); \
    if ( b3d->tkzs_.hsamp_.start_.dir() < dir##rng.start ) \
	b3d->tkzs_.hsamp_.start_.dir() = dir##rng.start; \
    if ( b3d->tkzs_.hsamp_.stop_.dir() > dir##rng.stop ) \
	b3d->tkzs_.hsamp_.stop_.dir() = dir##rng.stop;

    mChkRg(inl)
    mChkRg(crl)

    const Interval<float> zrng( seldata_->zRange() );
    if ( b3d->tkzs_.zsamp_.start < zrng.start )
	b3d->tkzs_.zsamp_.start = zrng.start;
    if ( b3d->tkzs_.zsamp_.stop > zrng.stop )
	b3d->tkzs_.zsamp_.stop = zrng.stop;

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
	if ( !ioobj_ || is2D() ) // TODO PS 2D
	    return nullptr;

	SeisPSReader* psrdr = SPSIOPF().get3DReader( *ioobj_ );
	mDynamicCastGet(SeisPS3DReader*,rdr3d,psrdr)
	if ( !rdr3d )
	    return nullptr;

	const PosInfo::CubeData& cd = rdr3d->posData();
	StepInterval<int> inlrg, crlrg;
	cd.getInlRange( inlrg ); cd.getInlRange( crlrg );
	return get3DBounds( inlrg, crlrg, SI().sampling(false).zsamp_ );
    }
    else if ( !is2D() )
    {
	if ( !trl_ )
	    return nullptr;
	if ( !isPrepared() &&
		!const_cast<SeisTrcReader*>(this)->prepareWork(Seis::Prod) )
	    return nullptr;
	return get3DBounds( strl()->packetInfo().inlrg,
			strl()->packetInfo().crlrg, strl()->packetInfo().zrg );
    }

    // From here post-stack 2D

    if ( !dataset_ || dataset_->nrLines() < 1 )
	return nullptr;

    auto* b2d = new Seis::Bounds2D;
    for ( int iiter=0; iiter<2; iiter++ ) // iiter == 0 is initialization
    {
	for ( int iln=0; iln<dataset_->nrLines(); iln++ )
	{
	    const Pos::GeomID selgeomid = SeisStoreAccess::geomID();
	    if ( !mIsUdfGeomID(selgeomid) && selgeomid != dataset_->geomID(iln))
		continue;

	    Pos::GeomID geomid =
		mIsUdfGeomID(selgeomid) ? dataset_->geomID( iln ) : selgeomid;
	    mDynamicCastGet(const Survey::Geometry2D*,geom2d,
			    Survey::GM().getGeometry(geomid))
	    if ( !geom2d )
		continue;

	    const PosInfo::Line2DData& l2dd = geom2d->data();
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


int SeisTrcReader::getNrOffsets( int maxnrpostobechecked ) const
{
    if ( !isPS() )
	return mUdf(int);

    int nroffsets = 0;
    if ( !is2D() )
    {
	PtrMan<SeisPSReader> psrdr = SPSIOPF().get3DReader( *ioobj_ );
	mDynamicCastGet(SeisPS3DReader*,rdr3d,psrdr.ptr())
	if ( !rdr3d )
	    return mUdf(int);

	PtrMan<PosInfo::CubeDataIterator> pscditer =
	    new PosInfo::CubeDataIterator( rdr3d->posData() );
	BinID psbid;
	int nrchecked = 0;
	while ( pscditer->next(psbid) )
	{
	    if ( nrchecked>=maxnrpostobechecked )
		break;
	    SeisTrcBuf tmptbuf( true );
	    psrdr->getGather( psbid, tmptbuf );
	    if ( nroffsets<tmptbuf.size() )
		nroffsets=tmptbuf.size();

	    nrchecked++;
	}
    }
    else
    {
	PtrMan<SeisPSReader> psrdr = SPSIOPF().get2DReader( *ioobj_, geomID() );
	mDynamicCastGet(SeisPS2DReader*,rdr2d,psrdr.ptr())
	if ( !rdr2d )
	    return mUdf(int);

	const PosInfo::Line2DData& l2d = rdr2d->posData();
	const TypeSet<PosInfo::Line2DPos>& allpos = l2d.positions();
	const int nrpos = allpos.size();
	const int step = float(nrpos) / (maxnrpostobechecked-1);
	for ( int posidx=0; posidx<nrpos; posidx+=step )
	{
	    const int trcnr = l2d.positions()[posidx].nr_;
	    SeisTrcBuf tmptbuf( true );
	    rdr2d->getGath( trcnr, tmptbuf );
	    if ( nroffsets<tmptbuf.size() )
		nroffsets = tmptbuf.size();
	}
    }

    return nroffsets;
}


bool SeisTrcReader::get3DGeometryInfo( PosInfo::CubeData& cd ) const
{
    if ( !isPS() )
    {
	if ( !isPrepared() &&
		!const_cast<SeisTrcReader*>(this)->prepareWork(Seis::Prod) )
	    return false;

	return strl() ? strl()->getGeometryInfo( cd ) : false;
    }
    else if ( !ioobj_ )
	return false;

    SeisPSReader* psrdr = SPSIOPF().get3DReader( *ioobj_ );
    mDynamicCastGet(SeisPS3DReader*,rdr3d,psrdr)
    if ( !rdr3d )
	return false;

    cd = rdr3d->posData();
    delete rdr3d;
    return true;
}
