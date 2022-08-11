/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : June 2005
-*/


#include "seisioobjinfo.h"

#include "bufstringset.h"
#include "cbvsreadmgr.h"
#include "conn.h"
#include "datadistributiontools.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "globexpr.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "linekey.h"
#include "posinfo2d.h"
#include "ptrman.h"
#include "seis2ddata.h"
#include "seisbuf.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seispsioprov.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seisstatscollector.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "statrand.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "trckeyzsampling.h"
#include "uistrings.h"
#include "zdomain.h"


Seis::ObjectSummary::ObjectSummary( const MultiID& mid )
    : ioobjinfo_(*new SeisIOObjInfo(mid))
{
    init();
}


Seis::ObjectSummary::ObjectSummary( const DBKey& dbkey )
    : ioobjinfo_(*new SeisIOObjInfo(dbkey))
{
    init();
}


Seis::ObjectSummary::ObjectSummary( const IOObj& ioobj )
    : ioobjinfo_(*new SeisIOObjInfo(ioobj))
{
    init();
}


Seis::ObjectSummary::ObjectSummary( const IOObj& ioobj, Pos::GeomID geomid )
    : ioobjinfo_(*new SeisIOObjInfo(ioobj))
{
    ioobjinfo_.is2D() ? init2D(geomid) : init();
}


Seis::ObjectSummary::ObjectSummary( const Seis::ObjectSummary& oth )
    : ioobjinfo_(*new SeisIOObjInfo(oth.ioobjinfo_))
{
    init();
}


Seis::ObjectSummary::~ObjectSummary()
{
    delete &ioobjinfo_;
}


Seis::ObjectSummary&
	Seis::ObjectSummary::operator =( const Seis::ObjectSummary& oth )
{
    if ( &oth == this ) return *this;

    const_cast<SeisIOObjInfo&>( ioobjinfo_ ) = oth.ioobjinfo_;
    init();

    return *this;
}


void Seis::ObjectSummary::init()
{
    bad_ = !ioobjinfo_.isOK() || ioobjinfo_.is2D();
    if ( bad_ ) return;

    geomtype_ = ioobjinfo_.geomType();
    TrcKeyZSampling tkzs( false );
    ioobjinfo_.getRanges( tkzs );
    zsamp_ = tkzs.zsamp_;

    SeisTrcReader rdr( *ioobjinfo_.ioObj(), &geomtype_ );
    if ( !rdr.prepareWork(Seis::PreScan) || !rdr.seisTranslator() )
	{ pErrMsg("Translator not SeisTrcTranslator!"); bad_ = true; return; }

    refreshCache( *rdr.seisTranslator() );
}


void Seis::ObjectSummary::init2D( Pos::GeomID geomid )
{
    bad_ = !ioobjinfo_.isOK();
    if ( bad_ ) return;

    geomtype_ = ioobjinfo_.geomType();
    StepInterval<int> trcrg;
    ioobjinfo_.getRanges( geomid, trcrg, zsamp_ );

    SeisTrcReader rdr( *ioobjinfo_.ioObj(), geomid, &geomtype_ );
    if ( !rdr.prepareWork(Seis::PreScan) || !rdr.seis2Dtranslator() )
	{ pErrMsg("Translator not SeisTrcTranslator!"); bad_ = true; return; }

    refreshCache( *rdr.seis2Dtranslator() );
}


void Seis::ObjectSummary::refreshCache( const SeisTrcTranslator& trl )
{
    if ( !trl.inputComponentData().isEmpty() )
	datachar_ = trl.componentInfo().first()->org.datachar;

    trl.getComponentNames( compnms_ );
    nrcomp_ = compnms_.size();
    nrsamppertrc_ = zsamp_.nrSteps()+1;
    nrbytespersamp_ = datachar_.nrBytes();
    nrdatabytespespercomptrc_ = nrbytespersamp_ * nrsamppertrc_;
    nrdatabytespertrc_ = nrdatabytespespercomptrc_ * nrcomp_;
    nrbytestrcheader_ = trl.bytesOverheadPerTrace();
    nrbytespertrc_ = nrbytestrcheader_ + nrdatabytespertrc_;
}


bool Seis::ObjectSummary::hasSameFormatAs( const BinDataDesc& desc ) const
{
    return datachar_ == desc;
}


#define mGetDataSet(nm,rv) \
    if ( !isOK() || !is2D() || isPS() ) return rv; \
 \
    PtrMan<Seis2DDataSet> nm \
	= new Seis2DDataSet( *ioobj_ ); \
    if ( nm->nrLines() == 0 ) \
	return rv


SeisIOObjInfo::SeisIOObjInfo( const IOObj* ioobj )
    : ioobj_(ioobj ? ioobj->clone() : nullptr)
{
    setType();
}


SeisIOObjInfo::SeisIOObjInfo( const IOObj& ioobj )
    : ioobj_(ioobj.clone())
{
    setType();
}


SeisIOObjInfo::SeisIOObjInfo( const MultiID& id )
    : ioobj_(IOM().get(id))
{
    setType();
}


SeisIOObjInfo::SeisIOObjInfo( const DBKey& dbkey )
{
    surveychanger_ = new SurveyChanger( dbkey.surveyDiskLocation() );
    ioobj_ = IOM().get( dbkey );
    setType();
}


SeisIOObjInfo::SeisIOObjInfo( const char* ioobjnm, Seis::GeomType geomtype )
	: geomtype_(geomtype)
	, ioobj_(nullptr)
{
    IOM().to( IOObjContext::Seis );
    switch ( geomtype_ )
    {
	case Seis::Vol:
	ioobj_ = IOM().getLocal( ioobjnm, mTranslGroupName(SeisTrc) );
	break;

	case Seis::VolPS:
	ioobj_ = IOM().getLocal( ioobjnm, mTranslGroupName(SeisPS3D) );
	break;

	case Seis::Line:
	ioobj_ = IOM().getLocal( ioobjnm, mTranslGroupName(SeisTrc2D) );
	break;

	case Seis::LinePS:
	ioobj_ = IOM().getLocal( ioobjnm, mTranslGroupName(SeisPS2D) );
	break;
    }

    setType();
}


SeisIOObjInfo::SeisIOObjInfo( const char* ioobjnm )
    : ioobj_(nullptr)
{
    IOM().to( IOObjContext::Seis );
    ioobj_ = IOM().getLocal( ioobjnm, nullptr );
    setType();
}


SeisIOObjInfo::SeisIOObjInfo( const SeisIOObjInfo& sii )
{
    *this = sii;
}


SeisIOObjInfo::~SeisIOObjInfo()
{
    delete surveychanger_;
    delete ioobj_;
}


SeisIOObjInfo& SeisIOObjInfo::operator =( const SeisIOObjInfo& sii )
{
    if ( &sii != this )
    {
	delete ioobj_;
	ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : nullptr;
	geomtype_ = sii.geomtype_;
	bad_ = sii.bad_;
	if ( surveychanger_ && surveychanger_->hasChanged() )
	{
	    SurveyDiskLocation sdl = surveychanger_->changedToSurvey();
	    delete surveychanger_;
	    surveychanger_ = new SurveyChanger( sdl );
	}
    }

    return *this;
}


void SeisIOObjInfo::setType()
{
    bad_ = !ioobj_;
    if ( bad_ ) return;

    const BufferString trgrpnm( ioobj_->group() );
    bool isps = false;
    if ( SeisTrcTranslator::isPS(*ioobj_) )
	isps = true;
    ioobj_->pars().getYN( SeisTrcTranslator::sKeyIsPS(), isps );

    if ( !isps && ioobj_->group()!=mTranslGroupName(SeisTrc) &&
	    ioobj_->group()!=mTranslGroupName(SeisTrc2D) )
	{ bad_ = true; return; }

    const bool is2d = SeisTrcTranslator::is2D( *ioobj_ );
    geomtype_ = isps ? (is2d ? Seis::LinePS : Seis::VolPS)
		     : (is2d ? Seis::Line : Seis::Vol);
}


SeisIOObjInfo::SpaceInfo::SpaceInfo( int ns, int ntr, int bps )
	: expectednrsamps(ns)
	, expectednrtrcs(ntr)
	, maxbytespsamp(bps)
{
    if ( expectednrsamps < 0 )
	expectednrsamps = SI().zRange(false).nrSteps() + 1;
    if ( expectednrtrcs < 0 )
	expectednrtrcs = sCast(int,SI().sampling(false).hsamp_.totalNr());
}


#define mChk(ret) if ( bad_ ) return ret

bool SeisIOObjInfo::getDefSpaceInfo( SpaceInfo& spinf ) const
{
    mChk(false);

    if ( Seis::isPS(geomtype_) )
    {
	if ( is2D() )
	    return false;
	else
	{
	    SeisPS3DReader* rdr = SPSIOPF().get3DReader( *ioobj_ );
	    if ( !rdr )
		return false;

	    const PosInfo::CubeData& cd = rdr->posData();
	    spinf.expectednrtrcs = cd.totalSize();
	    delete rdr;
	}
	spinf.expectednrsamps = SI().zRange(false).nrSteps() + 1;
	return true;
    }

    if ( is2D() )
    {
	mGetDataSet(dset,false);
	StepInterval<int> trcrg; StepInterval<float> zrg;
	TypeSet<Pos::GeomID> seen;
	spinf.expectednrtrcs = 0;
	for ( int idx=0; idx<dset->nrLines(); idx++ )
	{
	    const Pos::GeomID geomid = dset->geomID( idx );
	    if ( !seen.isPresent(geomid) )
	    {
		seen.add( geomid );
		dset->getRanges( geomid, trcrg, zrg );
		spinf.expectednrtrcs += trcrg.nrSteps() + 1;
	    }
	}
	spinf.expectednrsamps = zrg.nrSteps() + 1;
	spinf.maxbytespsamp = 4;
	return true;
    }

    TrcKeyZSampling cs;
    if ( !getRanges(cs) )
	return false;

    PosInfo::CubeData cd;
    SeisTrcReader rdr( *ioobj_, &geomtype_ );
    if ( rdr.prepareWork(Seis::Prod) && rdr.seisTranslator() &&
	 rdr.get3DGeometryInfo(cd) )
    {
	spinf.expectednrtrcs = cd.totalSize();
    }
    else
    {
	spinf.expectednrtrcs = sCast(int,cs.hsamp_.totalNr());
    }

    spinf.expectednrsamps = cs.zsamp_.nrSteps() + 1;
    getBPS( spinf.maxbytespsamp, -1 );
    return true;
}


bool SeisIOObjInfo::isTime() const
{
    const bool siistime = SI().zIsTime();
    mChk(siistime);
    return ZDomain::isTime( ioobj_->pars() );
}


bool SeisIOObjInfo::isDepth() const
{
    const bool siisdepth = !SI().zIsTime();
    mChk(siisdepth);
    return ZDomain::isDepth( ioobj_->pars() );
}


const ZDomain::Def& SeisIOObjInfo::zDomainDef() const
{
    mChk(ZDomain::SI());
    return ZDomain::Def::get( ioobj_->pars() );
}


int SeisIOObjInfo::SpaceInfo::expectedMBs() const
{
    if ( expectednrsamps<0 || expectednrtrcs<0 )
	return -1;
    od_int64 totnrbytes = expectednrsamps;
    totnrbytes *= expectednrtrcs;
    totnrbytes *= maxbytespsamp;
    return sCast(int,(totnrbytes / 1048576));
}


int SeisIOObjInfo::expectedMBs( const SpaceInfo& si ) const
{
    mChk(-1);

    int nrbytes = si.expectedMBs();
    if ( nrbytes < 0 || isPS() )
	return nrbytes;

    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		 ioobj_->createTranslator())
    if ( !sttr )
	return -1;

    int overhead = sttr->bytesOverheadPerTrace();
    double sz = si.expectednrsamps;
    sz *= si.maxbytespsamp;
    sz = (sz + overhead) * si.expectednrtrcs;

    const double bytes2mb = 9.53674e-7;
    return sCast(int,((sz * bytes2mb) + .5));
}


od_int64 SeisIOObjInfo::getFileSize( const char* filenm, int& nrfiles )
{
    if ( !File::isDirectory(filenm) && File::isEmpty(filenm) ) return -1;

    od_int64 totalsz = 0;
    nrfiles = 0;
    if ( File::isDirectory(filenm) )
    {
	DirList dl( filenm, File::FilesInDir );
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    FilePath filepath = dl.fullPath( idx );
	    StringView ext = filepath.extension();
	    if ( ext != "cbvs" )
		continue;

	    totalsz += File::getKbSize( filepath.fullPath() );
	    nrfiles++;
	}
    }
    else
    {
	while ( true )
	{
	    BufferString fullnm( CBVSIOMgr::getFileName(filenm,nrfiles) );
	    if ( !File::exists(fullnm) ) break;

	    totalsz += File::getKbSize( fullnm );
	    nrfiles++;
	}
    }

    return totalsz;
}


od_int64 SeisIOObjInfo::getFileSize() const
{
    const char* fnm = ioobj_->fullUserExpr();
    int nrfiles;
    return getFileSize( fnm, nrfiles );
}


bool SeisIOObjInfo::getRanges( TrcKeyZSampling& cs ) const
{
    mChk(false);
    mDynamicCastGet(IOStream*,iostrm,ioobj_)
    if ( is2D() )
    {
	StepInterval<int> trcrg;
	if ( (iostrm && iostrm->isMulti()) ||
	     !getRanges(cs.hsamp_.getGeomID(),trcrg,cs.zsamp_) )
	    return false;

	cs.hsamp_.setTrcRange( trcrg );
	return true;
    }

    cs.init( true );
    if ( !isPS() )
	return SeisTrcTranslator::getRanges( *ioobj_, cs );

    PtrMan<SeisPS3DReader> rdr = SPSIOPF().get3DReader( *ioobj_ );
    if ( !rdr )
	return false;

    cs.zsamp_ = rdr->getZRange();
    const PosInfo::CubeData& cd = rdr->posData();
    StepInterval<int> rg;
    cd.getInlRange( rg ); cs.hsamp_.setInlRange( rg );
    cd.getCrlRange( rg ); cs.hsamp_.setCrlRange( rg );
    return true;
}


bool SeisIOObjInfo::getDataChar( DataCharacteristics& dc ) const
{
    mChk(false);
    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		 ioobj_->createTranslator())
    if ( !sttr )
	{ pErrMsg("No Translator!"); return false; }

    Conn* conn = ioobj_->getConn( Conn::Read );
    if ( !sttr->initRead(conn,Seis::PreScan) )
	return false;

    ObjectSet<SeisTrcTranslator::TargetComponentData>& comps
		= sttr->componentInfo();
    if ( comps.isEmpty() )
	return false;

    dc = comps.first()->org.datachar;
    return true;
}


bool SeisIOObjInfo::haveAux( const char* ext ) const
{
    mChk(false);
    FilePath fp( ioobj_->mainFileName() );
    fp.setExtension( ext );
    return File::exists( fp.fullPath() );
}


bool SeisIOObjInfo::havePars() const
{ return haveAux( sParFileExtension() ); }

bool SeisIOObjInfo::haveStats() const
{ return haveAux( sStatsFileExtension() ); }


bool SeisIOObjInfo::getAux( const char* ext, const char* filetyp,
			    IOPar& iop ) const
{
    mChk(false);
    FilePath fp( ioobj_->mainFileName() );
    fp.setExtension( ext );
    return iop.read( fp.fullPath(), filetyp );
}


bool SeisIOObjInfo::getPars( IOPar& iop ) const
{ return getAux( sParFileExtension(), sKey::Pars(), iop ); }

bool SeisIOObjInfo::getStats( IOPar& iop ) const
{ return getAux( sStatsFileExtension(), sKey::Stats(), iop ); }

bool SeisIOObjInfo::isAvailableIn( const TrcKeySampling& tks ) const
{
    if ( !isOK() )
	return false;

    PosInfo::CubeData cd;
    SeisTrcReader rdr( *ioobj_, &geomtype_ );
    return rdr.prepareWork(Seis::Prod) && rdr.seisTranslator() &&
	   rdr.get3DGeometryInfo(cd) && cd.totalSizeInside(tks) > 0;
}


RefMan<FloatDistrib> SeisIOObjInfo::getDataDistribution() const
{
    mChk(nullptr);
    IOPar iop;
    RefMan<FloatDistrib> ret;
    if ( haveStats() && getStats(iop) )
    {
	PtrMan<IOPar> dpar = iop.subselect( sKey::Distribution() );
	if ( dpar && !dpar->isEmpty() )
	{
	    ret = new FloatDistrib;
	    DataDistributionChanger<float> chgr( *ret );
	    chgr.usePar( *dpar );
	    return ret;
	}
    }

    // No .stats file. Extract stats right now
    SeisTrcReader rdr( *ioobj_ );
    if ( !rdr.prepareWork() )
	return nullptr;

    SeisTrcTranslator* trl = rdr.seisTranslator();
    if ( !trl )
	return nullptr;

    PosInfo::CubeData cd;
    Seis::StatsCollector ssc;
    if ( trl->getGeometryInfo(cd) && (cd.size() > 0) )
    {
	Stats::RandGen randgen;
	PosInfo::CubeDataPos cdp;
	while ( true )
	{
	    cdp.lidx_ = randgen.getIndex( cd.size() );
	    const auto& segs = cd.get( cdp.lidx_ )->segments_;
	    cdp.segnr_ = randgen.getIndex( segs.size() );
	    cdp.sidx_ = randgen.getIndex( segs.get(cdp.segnr_).nrSteps()+1 );

	    const BinID bid = cd.binID( cdp );
	    bool res = true;
	    if ( trl->supportsGoTo() )
		res = trl->goTo( bid );

	    if ( !res )
		continue;

	    SeisTrc trc;
	    res = rdr.get( trc );
	    if ( !res || trc.isNull() )
		continue;

	    ssc.useTrace( trc );
	    if ( ssc.nrSamplesUsed() > 100000 )
		break;
	}
    }

    ret = &ssc.distribution();
    if ( !ret->isEmpty() )
    {
	iop.set( sKey::Source(), "Partial Scan" );
	ssc.fillPar( iop );
	FilePath fp( ioobj_->mainFileName() );
	fp.setExtension( sStatsFileExtension() );
	iop.write( fp.fullPath(), sKey::Stats() );
    }

    return ret;
}


bool SeisIOObjInfo::getBPS( int& bps, int icomp ) const
{
    mChk(false);
    if ( is2D() )
	return 4;

    if ( isPS() )
    {
	pErrMsg("TODO: no BPS for PS");
	return false;
    }

    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		 ioobj_->createTranslator())
    if ( !sttr )
	{ pErrMsg("No Translator!"); return false; }

    Conn* conn = ioobj_->getConn( Conn::Read );
    bool isgood = sttr->initRead(conn);
    bps = 0;
    if ( isgood )
    {
	ObjectSet<SeisTrcTranslator::TargetComponentData>& comps
		= sttr->componentInfo();
	for ( int idx=0; idx<comps.size(); idx++ )
	{
	    int thisbps = sCast(int,comps[idx]->datachar.nrBytes());
	    if ( icomp < 0 )
		bps += thisbps;
	    else if ( icomp == idx )
		bps = thisbps;
	}
    }

    if ( bps == 0 ) bps = 4;
    return isgood;
}


void SeisIOObjInfo::getGeomIDs( TypeSet<Pos::GeomID>& geomids ) const
{
    if ( !isOK() )
	return;

    if ( isPS() )
    {
	SPSIOPF().getGeomIDs( *ioobj_, geomids );
	return;
    }

    if ( !is2D() ) return;

    PtrMan<Seis2DDataSet> dset = new Seis2DDataSet( *ioobj_ );
    dset->getGeomIDs( geomids );
}


bool SeisIOObjInfo::isCompatibleType( const char* typestr1,
				      const char* typestr2 )
{
    const StringView typ1( typestr1 ); const StringView typ2( typestr2 );
    if ( typ1 == typ2 )
	return true;

    // One extra chance: 'Attribute' is compatible with 'no type'.
    const bool isnorm1 = typ1.isEmpty() || typ1 == sKey::Attribute();
    const bool isnorm2 = typ2.isEmpty() || typ2 == sKey::Attribute();
    return isnorm1 && isnorm2;
}


void SeisIOObjInfo::getNms( BufferStringSet& bss,
			    const SeisIOObjInfo::Opts2D& o2d ) const
{
    if ( !isOK() )
	return;

    if ( isPS() )
    {
	SPSIOPF().getLineNames( *ioobj_, bss );
	return;
    }

   if ( !isOK() || !is2D() || isPS() ) return;

    PtrMan<Seis2DDataSet> dset
	= new Seis2DDataSet( *ioobj_ );
    if ( dset->nrLines() == 0 )
	return;

    BufferStringSet rejected;
    for ( int idx=0; idx<dset->nrLines(); idx++ )
    {
	const char* nm = dset->lineName(idx);
	if ( bss.isPresent(nm) )
	    continue;

	if ( o2d.bvs_ )
	{
	    if ( rejected.isPresent(nm) )
		continue;
	}

	bss.add( nm );
    }

    bss.sort();
}


bool SeisIOObjInfo::getRanges( const Pos::GeomID geomid,
			       StepInterval<int>& trcrg,
			       StepInterval<float>& zrg ) const
{
    mChk(false);
    if ( !isPS() )
    {
	ConstPtrMan<Seis2DDataSet> dataset = new Seis2DDataSet( *ioobj_ );
	return dataset->getRanges( geomid, trcrg, zrg );
    }

    ConstPtrMan<SeisPS2DReader> rdr = SPSIOPF().get2DReader( *ioobj_, geomid );
    if ( !rdr )
	return false;

    zrg = rdr->getZRange();
    const PosInfo::Line2DData& l2dd = rdr->posData();
    trcrg = l2dd.trcNrRange();
    return true;
}


static BufferStringSet& getTypes()
{
    mDefineStaticLocalObject( BufferStringSet, types, );
    return types;
}

static TypeSet<MultiID>& getIDs()
{
    mDefineStaticLocalObject( TypeSet<MultiID>, ids, );
    return ids;
}


void SeisIOObjInfo::initDefault( const char* typ )
{
    BufferStringSet& typs = getTypes();
    if ( typs.isPresent(typ) ) return;

    IOObjContext ctxt( SeisTrcTranslatorGroup::ioContext() );
    ctxt.toselect_.require_.set( sKey::Type(), typ );
    int nrpresent = 0;
    PtrMan<IOObj> ioobj = IOM().getFirst( ctxt, &nrpresent );
    if ( !ioobj || nrpresent > 1 )
	return;

    typs.add( typ );
    getIDs() += ioobj->key();
}


const MultiID& SeisIOObjInfo::getDefault( const char* typ )
{
    mDefineStaticLocalObject( const MultiID, noid, ("") );
    const int typidx = getTypes().indexOf( typ );
    return typidx < 0 ? noid : getIDs()[typidx];
}


void SeisIOObjInfo::setDefault( const MultiID& id, const char* typ )
{
    BufferStringSet& typs = getTypes();
    TypeSet<MultiID>& ids = getIDs();

    const int typidx = typs.indexOf( typ );
    if ( typidx > -1 )
	ids[typidx] = id;
    else
    {
	typs.add( typ );
	ids += id;
    }
}


int SeisIOObjInfo::nrComponents( Pos::GeomID geomid ) const
{
    return getComponentInfo( geomid, nullptr );
}


void SeisIOObjInfo::getComponentNames( BufferStringSet& nms,
				       Pos::GeomID geomid ) const
{
    getComponentInfo( geomid, &nms );
}


void SeisIOObjInfo::getCompNames( const MultiID& mid, BufferStringSet& nms )
{
    SeisIOObjInfo ioobjinf( mid );
    ioobjinf.getComponentNames( nms, Survey::GM().cUndefGeomID() );
}


int SeisIOObjInfo::getComponentInfo( Pos::GeomID geomid,
				     BufferStringSet* nms ) const
{
    int ret = 0;
    if ( nms )
	nms->erase();

    mChk(ret);
    if ( isPS() )
	return 0;

    if ( !is2D() )
    {
	mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		     ioobj_->createTranslator())
	if ( !sttr )
	    { pErrMsg("No Translator!"); return 0; }
	Conn* conn = ioobj_->getConn( Conn::Read );
	if ( sttr->initRead(conn) )
	{
	    ret = sttr->componentInfo().size();
	    if ( nms )
	    {
		for ( int icomp=0; icomp<ret; icomp++ )
		    nms->add( sttr->componentInfo()[icomp]->name() );
	    }
	}
    }
    else
    {
	PtrMan<Seis2DDataSet> dataset = new Seis2DDataSet( *ioobj_ );
	if ( !dataset || dataset->nrLines() == 0 )
	    return 0;

	int lidx = dataset->indexOf( geomid );
	if ( lidx < 0 ) lidx = 0;
	SeisTrcBuf tbuf( true );
	Executor* ex = dataset->lineFetcher( dataset->geomID(lidx), tbuf, 1 );
	if ( ex ) ex->doStep();
	ret = tbuf.isEmpty() ? 0 : tbuf.get(0)->nrComponents();
	if ( nms )
	{
	    mDynamicCastGet(Seis2DLineGetter*,lg,ex)
	    if ( !lg )
	    {
		for ( int icomp=0; icomp<ret; icomp++ )
		    nms->add( BufferString("[",icomp+1,"]") );
	    }
	    else
	    {
		ret = lg->translator() ?
		      lg->translator()->componentInfo().size() : 0;
		if ( nms )
		{
		    for ( int icomp=0; icomp<ret; icomp++ )
			nms->add(
			    lg->translator()->componentInfo()[icomp]->name() );
		}
	    }
	}
	delete ex;
    }

    return ret;
}


bool SeisIOObjInfo::hasData( Pos::GeomID geomid )
{
    const char* linenm = Survey::GM().getName( geomid );
    const IODir iodir( IOObjContext::getStdDirData(IOObjContext::Seis)->id_ );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( SeisTrcTranslator::isPS(ioobj) )
	{
	    BufferStringSet linenames;
	    SPSIOPF().getLineNames( ioobj, linenames );
	    if ( linenames.isPresent(linenm) )
		return true;
	}
	else
	{
	    if ( !(*ioobj.group() == '2') )
		continue;

	    Seis2DDataSet dset( ioobj );
	    if ( dset.isPresent(geomid) )
		return true;
	}
    }

    return false;
}


void SeisIOObjInfo::getDataSetNamesForLine( const char* lnm,
					    BufferStringSet& datasets,
					    Opts2D o2d )
{ getDataSetNamesForLine( Survey::GM().getGeomID(lnm), datasets, o2d ); }

void SeisIOObjInfo::getDataSetNamesForLine( Pos::GeomID geomid,
					    BufferStringSet& datasets,
					    Opts2D o2d )
{
    if ( geomid == mUdfGeomID )
	return;

    IOObjContext ctxt( mIOObjContext(SeisTrc2D) );
    const IODir iodir( ctxt.getSelKey() );
    const IODirEntryList del( iodir, ctxt );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj_;
	if ( !ioobj )
	    continue;

	if ( o2d.zdomky_ != "*" )
	{
	    const ZDomain::Def& reqzdom = ZDomain::Def::get( o2d.zdomky_ );
	    const ZDomain::Def& datazdom = ZDomain::Def::get( ioobj->pars() );
	    if ( datazdom != reqzdom )
		continue;
	}

	if ( o2d.steerpol_ != 2 )
	{
	    const StringView dt = ioobj->pars().find( sKey::Type() );
	    const bool issteering = dt==sKey::Steering();
	    const bool wantsteering = o2d.steerpol_ == 1;
	    if ( issteering != wantsteering )
		continue;
	}

	Seis2DDataSet ds( *ioobj );
	if ( ds.isPresent(geomid) )
	    datasets.add( ioobj->name() );
    }
}


bool SeisIOObjInfo::isFullyRectAndRegular() const
{
    PtrMan<Translator> trl = ioobj_->createTranslator();
    mDynamicCastGet(CBVSSeisTrcTranslator*,cbvstrl,trl.ptr())
    if ( !cbvstrl ) return false;

    Conn* conn = ioobj_->getConn( Conn::Read );
    if ( !cbvstrl->initRead(conn) || !cbvstrl->readMgr() )
	return false;

    const CBVSInfo& info = cbvstrl->readMgr()->info();
    return info.geom_.fullyrectandreg;
}


void SeisIOObjInfo::getLinesWithData( BufferStringSet& lnms,
				      TypeSet<Pos::GeomID>& gids )
{
    Survey::GMAdmin().updateGeometries( nullptr );
    Survey::GM().getList( lnms, gids, true );
    BoolTypeSet hasdata( gids.size(), false );

    const IODir iodir( IOObjContext::getStdDirData(IOObjContext::Seis)->id_ );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	TypeSet<Pos::GeomID> dsgids;
	if ( SeisTrcTranslator::isPS(ioobj) )
	    SPSIOPF().getGeomIDs( ioobj, dsgids );
	else if ( *ioobj.group() == '2' )
	{
	    Seis2DDataSet dset( ioobj );
	    dset.getGeomIDs( dsgids );
	}

	if ( dsgids.isEmpty() )
	    continue;

	for ( int idl=0; idl<gids.size(); idl++ )
	{
	    if ( !hasdata[idl] && dsgids.isPresent(gids[idl]) )
		hasdata[idl] = true;
	}
    }

    for ( int idl=gids.size()-1; idl>=0; idl-- )
    {
	if ( !hasdata[idl] )
	{
	    lnms.removeSingle( idl );
	    gids.removeSingle( idl );
	}
    }
}


bool SeisIOObjInfo::getDisplayPars( IOPar& iop ) const
{
    if ( !ioobj_ )
	return false;

    FilePath fp( ioobj_->fullUserExpr(true) );
    fp.setExtension( "par" );
    return iop.read(fp.fullPath(),sKey::Pars()) && !iop.isEmpty();
}


void SeisIOObjInfo::getUserInfo( uiStringSet& inf ) const
{
    if ( !isOK() )
	{ inf.add( uiStrings::sNoInfoAvailable() ); return; }

    getCommonUserInfo( inf );
    if ( isPS() )
	getPreStackUserInfo( inf );
    else
	getPostStackUserInfo( inf );
}


void SeisIOObjInfo::getCommonUserInfo( uiStringSet& inf ) const
{
    const bool is2d = is2D();
    if ( is2d )
    {
	BufferStringSet nms;
	Opts2D opts2d; opts2d.zdomky_ = "*";
	getLineNames( nms, opts2d );
	inf.addKeyValue( tr("Number of lines"), nms.size() );
    }

#define mOnNewLineLine(str) \
    txt.appendPhrase(str,uiString::NoSep) \

#define mAddICRangeLine(key,memb) \
    inf.addKeyValue( key, toUiString("%1 - %2 [%3]") \
	.arg( cs.hsamp_.start_.memb() ) \
	.arg( cs.hsamp_.stop_.memb() ) \
	.arg( cs.hsamp_.step_.memb() ) )

    if ( !is2d )
    {
	const ZDomain::Def& zddef = zDomainDef();
	TrcKeyZSampling cs;
	if ( getRanges(cs) )
	{
	    if ( !mIsUdf(cs.hsamp_.stop_.inl()) )
		mAddICRangeLine( uiStrings::sInlineRange(), inl );
	    if ( !mIsUdf(cs.hsamp_.stop_.crl()) )
		mAddICRangeLine( uiStrings::sCrosslineRange(), crl );

	    SpaceInfo spcinfo;
	    double area;
	    if ( getDefSpaceInfo(spcinfo) )
	    {
		area = cs.hsamp_.lineDistance() *
		       cs.hsamp_.trcDistance() * spcinfo.expectednrtrcs;
	    }
	    else
	    {
		area = SI().getArea( cs.hsamp_.inlRange(),
				     cs.hsamp_.crlRange() );
	    }

	    inf.addKeyValue( uiStrings::sArea(),
		     getAreaString(float(area),SI().xyInFeet(),2,true) );

	    StepInterval<float> dispzrg( cs.zsamp_ );
	    dispzrg.scale( float(zddef.userFactor()) );
	    inf.addKeyValue( zddef.getRange().withUnit(zddef.unitStr()),
		    toUiString("%1 - %2 [%3]")
		    .arg(dispzrg.start).arg(dispzrg.stop).arg(dispzrg.step) );
	}
    }

    if ( !ioobj_->pars().isEmpty() )
    {
	const IOPar& pars = ioobj_->pars();
	StringView parstr = pars.find( sKey::Type() );
	if ( !parstr.isEmpty() )
	    inf.addKeyValue( uiStrings::sType(), parstr );

	parstr = pars.find( "Optimized direction" );
	if ( !parstr.isEmpty() )
	    inf.addKeyValue( tr("Optimized direction"), parstr );

	if ( pars.isTrue("Is Velocity") )
	{
	    parstr = pars.find( "Velocity Type" );
	    if ( !parstr.isEmpty() )
		inf.addKeyValue( tr("Velocity Type"), parstr );

	    Interval<float> topvavg, botvavg;
	    if ( pars.get(VelocityStretcher::sKeyTopVavg(),topvavg)
	      && pars.get(VelocityStretcher::sKeyBotVavg(),botvavg))
	    {
		const StepInterval<float> sizrg = SI().zRange();
		StepInterval<float> dispzrg;
		uiString keystr;
		if ( SI().zIsTime() )
		{
		    dispzrg.start = sizrg.start * topvavg.start / 2;
		    dispzrg.stop = sizrg.stop * botvavg.stop / 2;
		    dispzrg.step = (dispzrg.stop-dispzrg.start)
					/ sizrg.nrSteps();
		    dispzrg.scale( float(ZDomain::Depth().userFactor()) );
		    keystr = tr("Depth Range")
			    .withUnit( ZDomain::Depth().unitStr() );
		}

		else
		{
		    dispzrg.start = 2 * sizrg.start / topvavg.stop;
		    dispzrg.stop = 2 * sizrg.stop / botvavg.start;
		    dispzrg.step = (dispzrg.stop-dispzrg.start)
					/ sizrg.nrSteps();
		    dispzrg.scale( float(ZDomain::Time().userFactor()) );
		    keystr = tr("Time Range")
			    .withUnit( ZDomain::Time().unitStr() );
		}

		inf.addKeyValue( keystr, toUiString("%1 - %2 [%3]")
		    .arg(dispzrg.start).arg(dispzrg.stop).arg(dispzrg.step) );
	    }
	}
    }

    DataCharacteristics dc;
    getDataChar( dc );
    const BufferString dcstr( DataCharacteristics::toString(dc.userType()) );
    inf.addKeyValue( uiStrings::sStorage(), dcstr );
}


void SeisIOObjInfo::getPostStackUserInfo( uiStringSet& inf ) const
{
    BufferStringSet compnms;
    getComponentNames( compnms );
    if ( compnms.size() > 1 )
    {
	for ( int idx=0; idx<compnms.size(); idx++ )
	    inf.addKeyValue( toUiString("%1 %2")
		 .arg(uiStrings::sComponent()).arg(idx+1), compnms.get(idx) );
    }
}


void SeisIOObjInfo::getPreStackUserInfo( uiStringSet& inf ) const
{
    if ( is2D() )
    {
	BufferStringSet nms;
	SPSIOPF().getLineNames( *ioobj_, nms );
	inf.addKeyValue( uiStrings::sLine(mPlural), nms.getDispString(3,false));
    }
    else
    {
	PtrMan<SeisPS3DReader> rdr = SPSIOPF().get3DReader( *ioobj_ );
	if ( rdr )
	{
	    const PosInfo::CubeData& cd = rdr->posData();
	    inf.addKeyValue( tr("Total number of gathers"), cd.totalSize() );
	}
    }
}
