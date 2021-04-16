/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : June 2005
-*/


#include "seisioobjinfo.h"

#include "bufstringset.h"
#include "cbvsio.h"
#include "conn.h"
#include "cubesubsel.h"
#include "dbdir.h"
#include "dbman.h"
#include "datadistributiontools.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "fullsubsel.h"
#include "genc.h"
#include "globexpr.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "linesubsel.h"
#include "od_istream.h"
#include "ptrman.h"
#include "seistrctr.h"
#include "seis2dlineio.h"
#include "seis2ddata.h"
#include "seisbuf.h"
#include "seisprovider.h"
#include "seispsioprov.h"
#include "seispacketinfo.h"
#include "seisstatscollector.h"
#include "seistrc.h"
#include "statrand.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "trckeyzsampling.h"
#include "zdomain.h"


static bool isSteering( const IOObj& ioobj )
{
    const FixedString res = ioobj.pars().find( sKey::Type() );
    return res == sKey::Steering();
}


Seis::ObjectSummary::ObjectSummary( const DBKey& dbkey, GeomID geomid )
    : ioobjinfo_(*new SeisIOObjInfo(dbkey))		{ init( geomid ); }
Seis::ObjectSummary::ObjectSummary( const IOObj& ioobj, GeomID geomid )
    : ioobjinfo_(*new SeisIOObjInfo(ioobj))		{ init( geomid ); }
Seis::ObjectSummary::ObjectSummary( const Seis::ObjectSummary& oth )
    : ioobjinfo_(*new SeisIOObjInfo(oth.ioobjinfo_))	{ init( oth.geomid_ ); }

Seis::ObjectSummary::~ObjectSummary()
{
    delete &ioobjinfo_;
}


Seis::ObjectSummary& Seis::ObjectSummary::operator =(
						const Seis::ObjectSummary& oth )
{
    if ( &oth != this )
    {
	const_cast<SeisIOObjInfo&>( ioobjinfo_ ) = oth.ioobjinfo_;
	init( oth.geomid_ );
    }
    return *this;
}


void Seis::ObjectSummary::init( GeomID geomid )
{
    bad_ = !ioobjinfo_.isOK();
    if ( bad_ )
	return;

    modiftm_ = ioobjinfo_.getFileModifTime();
    geomtype_ = ioobjinfo_.geomType();
    if ( !is2D() )
	geomid = GeomID::get3D();
    else if ( !geomid.isValid() )
    {
	Seis2DDataSet dset( *ioobjinfo_.ioObj() );
	if ( dset.nrLines() > 0 )
	    geomid = dset.geomID( 0 );
	if ( !geomid.isValid() )
	    { bad_ = true; return; }
    }
    if ( is2D() != geomid.is2D() )
	{ pErrMsg("Geometry wrongly set"); bad_ = true; return; }

    geomid_ = geomid;
    TrcKeyZSampling tkzs( false );
    if ( is2D() )
	tkzs.hsamp_ = TrcKeySampling( geomid_ );

    ioobjinfo_.getRanges( tkzs );
    zsamp_ = tkzs.zsamp_;

    uiRetVal uirv;
    PtrMan<Seis::Provider> prov =
	       Seis::Provider::create( *ioobjinfo_.ioObj(), &uirv );
    if ( !prov || !uirv.isOK() )
	{ bad_ = true; return; }

    refreshCache( *prov );
}


void Seis::ObjectSummary::refreshCache( const Seis::Provider& prov )
{
    const auto* trl = prov.curTransl();
    if ( trl && !trl->inputComponentData().isEmpty() )
	datachar_ = trl->inputComponentData().first()->datachar_;

    prov.getComponentInfo( compnms_ );
    nrsamppertrc_ = zsamp_.nrSteps()+1;
    nrbytespersamp_ = datachar_.nrBytes();
    nrdatabytespespercomptrc_ = nrbytespersamp_ * nrsamppertrc_;
    nrdatabytespertrc_ = nrdatabytespespercomptrc_ * compnms_.size();
    nrbytestrcheader_ = trl ? trl->bytesOverheadPerTrace() : 48;
    nrbytespertrc_ = nrbytestrcheader_ + nrdatabytespertrc_;
}


bool Seis::ObjectSummary::hasSameFormatAs( const BinDataDesc& desc ) const
{
    return datachar_ == desc;
}


const IOObj* Seis::ObjectSummary::ioObj() const
{
    return ioObjInfo().ioObj();
}


DBKey Seis::ObjectSummary::key() const
{
    return ioObj() ? ioObj()->key() : DBKey();
}


const char* Seis::ObjectSummary::name() const
{
    return ioObj() ? ioObj()->name() : "";
}



SeisIOObjInfo::SeisIOObjInfo( const IOObj* ioobj )
	: ioobj_(ioobj ? ioobj->clone() : 0)		{ setType(); }
SeisIOObjInfo::SeisIOObjInfo( const IOObj& ioobj )
	: ioobj_(ioobj.clone())				{ setType(); }
SeisIOObjInfo::SeisIOObjInfo( const DBKey& id )
	: ioobj_(getIOObj(id))				{ setType(); }


SeisIOObjInfo::SeisIOObjInfo( const char* ioobjnm, Seis::GeomType geomtype )
	: ioobj_(0)
	, geomtype_(geomtype)
{
    ConstRefMan<DBDir> dbdir = DBM().fetchDir( IOObjContext::Seis );
    if ( dbdir )
    {
#	define mGetIOObj(trgrpnm) \
	    dbdir->getEntryByName( ioobjnm, mTranslGroupName(trgrpnm) );
	switch ( geomtype_ )
	{
	    case Seis::Vol:
		ioobj_ = mGetIOObj( SeisTrc );
	    break;

	    case Seis::VolPS:
		ioobj_ = mGetIOObj( SeisPS3D );
	    break;

	    case Seis::Line:
		ioobj_ = mGetIOObj( SeisTrc2D );
	    break;

	    case Seis::LinePS:
		ioobj_ = mGetIOObj( SeisPS2D );
	    break;
	}
    }

    setType();
}


SeisIOObjInfo::SeisIOObjInfo( const SeisIOObjInfo& sii )
	: geomtype_(sii.geomtype_)
	, bad_(sii.bad_)
{
    ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : 0;
}


SeisIOObjInfo::~SeisIOObjInfo()
{
    delete ioobj_;
}


SeisIOObjInfo& SeisIOObjInfo::operator =( const SeisIOObjInfo& sii )
{
    if ( &sii != this )
    {
	delete ioobj_;
	ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : 0;
	geomtype_ = sii.geomtype_;
	bad_ = sii.bad_;
    }
    return *this;
}


void SeisIOObjInfo::setType()
{
    bad_ = !ioobj_;
    geomtype_ = Seis::Vol;
    if ( bad_ )
	return;

    const BufferString trgrpnm( ioobj_->group() );
    bool isps = false;
    if ( SeisTrcTranslator::isPS(*ioobj_) )
	isps = true;
    ioobj_->pars().getYN( sKey::IsPS(), isps );

    if ( !isps && ioobj_->group()!=mTranslGroupName(SeisTrc) &&
	    ioobj_->group()!=mTranslGroupName(SeisTrc2D) )
	{ bad_ = true; return; }

    const bool is2d = SeisTrcTranslator::is2D( *ioobj_, false );
    geomtype_ = isps ? (is2d ? Seis::LinePS : Seis::VolPS)
		     : (is2d ? Seis::Line : Seis::Vol);
}


SeisIOObjInfo::SpaceInfo::SpaceInfo( int ns, int ntr, int bps )
	: expectednrsamps(ns)
	, expectednrtrcs(ntr)
	, maxbytespsamp(bps)
{
    if ( expectednrsamps < 0 )
	expectednrsamps = SI().zRange().nrSteps() + 1;
    if ( expectednrtrcs < 0 )
    {
	const TrcKeySampling tks( true );
	expectednrtrcs = (int)tks.totalNr();
    }
}


#define mChk(ret) if ( bad_ ) return ret

bool SeisIOObjInfo::getDefSpaceInfo( SpaceInfo& spinf ) const
{
    mChk(false);

    PtrMan<Seis::Provider> prov = Seis::Provider::create( *ioobj_ );
    if ( !prov )
	return false;

    const auto nroffs = prov->nrOffsets();
    spinf.expectednrtrcs = prov->totalNr() * nroffs;
    spinf.expectednrsamps = prov->zRange().nrSteps() + 1;
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


BufferString SeisIOObjInfo::iconID() const
{
    mChk( BufferString() );
    PtrMan<Translator> transl = ioobj_->createTranslator();
    return BufferString( transl ? transl->iconName() : "" );
}


int SeisIOObjInfo::SpaceInfo::expectedMBs() const
{
    if ( expectednrsamps<0 || expectednrtrcs<0 )
	return -1;
    od_int64 totnrbytes = expectednrsamps;
    totnrbytes *= expectednrtrcs;
    totnrbytes *= maxbytespsamp;
    return (int)( totnrbytes / 1048576 );
}


int SeisIOObjInfo::expectedMBs( const SpaceInfo& si ) const
{
    mChk(-1);

    int nrbytes = si.expectedMBs();
    if ( nrbytes < 0 || isPS() )
	return nrbytes;

    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		    ioobj_->createTranslator() );
    if ( !sttr )
	return -1;

    int overhead = sttr->bytesOverheadPerTrace();
    double sz = si.expectednrsamps;
    sz *= si.maxbytespsamp;
    sz = (sz + overhead) * si.expectednrtrcs;

    const double bytes2mb = 9.53674e-7;
    return (int)((sz * bytes2mb) + .5);
}



static bool isCBVSFile( const char* fnm )
{
    const File::Path filepath( fnm );
    return FixedString( filepath.extension() ) == "cbvs";
}


od_int64 SeisIOObjInfo::getFileSize( const char* filenm, int& nrfiles )
{
    if ( !File::isDirectory(filenm) && File::isEmpty(filenm) )
	return -1;

    od_int64 totalsz = 0;
    nrfiles = 0;
    if ( File::isDirectory(filenm) )
    {
	DirList dl( filenm, File::FilesInDir );
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    const BufferString fnm( dl.fullPath(idx) );
	    if ( isCBVSFile(fnm) )
	    {
		totalsz += File::getKbSize( fnm );
		nrfiles++;
	    }
	}
    }
    else if ( File::exists(filenm) )
    {
	totalsz += File::getKbSize( filenm );
	nrfiles++;
	if ( isCBVSFile(filenm) )
	{
	    while ( true )
	    {
		BufferString fullnm( CBVSIOMgr::getFileName(filenm,nrfiles) );
		if ( !File::exists(fullnm) )
		    break;

		totalsz += File::getKbSize( fullnm );
		nrfiles++;
	    }
	}
    }

    return totalsz;
}


od_int64 SeisIOObjInfo::getFileSize() const
{
    int nrfiles;
    return ioobj_ ? getFileSize( ioobj_->mainFileName(), nrfiles ) : -1;
}


od_int64 SeisIOObjInfo::getFileModifTime() const
{
    const od_int64 modiftm = File::getTimeInSeconds( ioobj_->mainFileName() );
    return modiftm < 1 ? mUdf(od_int64) : modiftm;
}


Survey::FullSubSel* SeisIOObjInfo::getSurvSubSel() const
{
    if ( is2D() )
    {
	GeomIDSet geomids;
	getGeomIDs( geomids );
	LineSubSelSet lsss;
	for ( const auto geomid : geomids )
	{
	    TrcKeyZSampling tkzs( geomid );
	    if ( !getRanges(tkzs) )
		continue;
	    lsss.add( new LineSubSel(tkzs) );
	}
	return new Survey::FullSubSel( lsss );
    }

    TrcKeyZSampling cs( false );
    if ( !getRanges(cs) )
	return nullptr;;
    return new Survey::FullSubSel( CubeSubSel(cs) );
}


Survey::GeomSubSel* SeisIOObjInfo::getGeomSubSel( GeomID gid ) const
{
    PtrMan<Survey::FullSubSel> fss = getSurvSubSel();
    if ( !fss || !fss->isPresent(gid) )
	return nullptr;

    const int ssidx = fss->indexOf(gid);
    return fss->getGeomSubSel( ssidx );
}


bool SeisIOObjInfo::getRanges( TrcKeyZSampling& cs ) const
{
    mChk(false);
    mDynamicCastGet(IOStream*,iostrm,ioobj_)
    if ( is2D() )
    {
	cs.hsamp_.setIs2D();
	StepInterval<int> trcrg;
	if ( (iostrm && iostrm->isMulti()) ||
	     !getRanges(cs.hsamp_.getGeomID(),trcrg,cs.zsamp_) )
	    return false;

	cs.hsamp_.setTrcRange( trcrg );
	return true;
    }

    cs.init( true );
    cs.hsamp_.setIs3D();
    if ( !isPS() )
	return SeisTrcTranslator::getRanges( *ioobj_, cs );

    SeisPS3DReader* rdr = SPSIOPF().get3DReader( *ioobj_ );
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
    if ( isPS() )
    {
	DataCharacteristics::UserType ut = OD::F32;
	DataCharacteristics::getUserTypeFromPar( ioobj_->pars(), ut );
	dc = DataCharacteristics( ut );
	return true;
    }

    Translator* trl = ioobj_->createTranslator();
    if ( !trl )
	{ pErrMsg("No Translator!"); return false; }
    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		 trl );
    if ( !sttr )
	{ pErrMsg("Translator not SeisTrcTranslator!"); return false; }

    Conn* conn = ioobj_->getConn( Conn::Read );
    if ( !sttr->initRead(conn,Seis::PreScan) )
	return false;

    const ObjectSet<SeisTrcTranslator::ComponentData>& comps
					    = sttr->inputComponentData();
    if ( comps.isEmpty() )
	return false;

    dc = comps.first()->datachar_;
    return true;
}


bool SeisIOObjInfo::haveAux( const char* ext ) const
{
    mChk(false);
    File::Path fp( ioobj_->mainFileName() );
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
    File::Path fp( ioobj_->mainFileName() );
    fp.setExtension( ext );
    return iop.read( fp.fullPath(), filetyp );
}


bool SeisIOObjInfo::getPars( IOPar& iop ) const
{ return getAux( sParFileExtension(), sKey::Pars(), iop ); }
bool SeisIOObjInfo::getStats( IOPar& iop ) const
{ return getAux( sStatsFileExtension(), sKey::Stats(), iop ); }


#define mMaxRandTrcs PosInfo::LineCollData::glob_size_type(100000)

static bool addDataFromPos( const Seis::Provider::LineCollData& lcd,
			const PosInfo::LineCollDataPos& lcdp,
			Seis::Provider2D* prov2d, Seis::Provider3D* prov3d,
			Seis::StatsCollector& ssc, uiRetVal& uirv )
{
    SeisTrc trc;
    if ( prov2d )
	uirv = prov2d->getAt( lcd.bin2D(lcdp), trc );
    else
	uirv = prov3d->getAt( lcd.binID(lcdp), trc );

    if ( !uirv.isOK() )
	return false;
    else if ( trc.isNull() )
	return true;

    ssc.useTrace( trc );
    return true;
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
    PtrMan<Seis::Provider> prov = Seis::Provider::create( *ioobj_ );
    if ( !prov || prov->isEmpty() )
	return ret;

    Seis::StatsCollector ssc;
    const Seis::Provider::LineCollData& lcd = prov->possiblePositions();
    PosInfo::LineCollData::glob_size_type maxdsnrtrcs = 0;
    for ( const auto* cd : lcd )
    {
	const PosInfo::LineData::SegmentSet& segs = cd->segments_;
	for ( const auto& seg : segs )
	{
	    maxdsnrtrcs +=
		PosInfo::LineCollData::glob_size_type( seg.nrSteps()+1 );
	}
    }

    const PosInfo::LineCollData::glob_size_type maxnrtrcs =
				mMIN(maxdsnrtrcs,mMaxRandTrcs);
    Seis::Provider2D* prov2d = prov->as2D();
    Seis::Provider3D* prov3d = prov->as3D();
    const bool fullscan = maxnrtrcs == maxdsnrtrcs;
    PosInfo::LineCollDataPos lcdp;
    uiRetVal uirv;
    if ( fullscan )
    {
	for ( lcdp.lidx_=0; lcdp.lidx_<lcd.size(); lcdp.lidx_++ )
	{
	    const PosInfo::LineData::SegmentSet& segs =
				lcd.get( lcdp.lidx_ )->segments_;
	    for ( lcdp.segnr_=0; lcdp.segnr_<segs.size(); lcdp.segnr_++ )
	    {
		const PosInfo::LineData::Segment& seg = segs[lcdp.segnr_];
		const int segsz = seg.nrSteps()+1;
		for ( lcdp.sidx_=0; lcdp.sidx_<segsz; lcdp.sidx_++ )
		{
		    addDataFromPos( lcd, lcdp, prov2d, prov3d,
				    ssc, uirv );
		}
	    }
	}
    }
    else
    {
	Stats::RandGen randgen;
	PosInfo::LineCollData::glob_size_type nrdrawn = 0;
	const PosInfo::LineCollData::glob_size_type maxnrtries =
				    mMaxRandTrcs * 5;
	while ( nrdrawn < maxnrtries )
	{
	    lcdp.lidx_ = randgen.getIndex( lcd.size() );
	    const auto& segs = lcd.get( lcdp.lidx_ )->segments_;
	    lcdp.segnr_ = randgen.getIndex( segs.size() );
	    lcdp.sidx_ = randgen.getIndex( segs.get(lcdp.segnr_).nrSteps()+1 );
	    nrdrawn++;
	    if ( !addDataFromPos(lcd,lcdp,prov2d,prov3d,ssc,uirv) ||
		  ssc.nrSamplesUsed() > maxnrtrcs )
		break;
	}
    }

    ret = &ssc.distribution();
    if ( ret->isEmpty() )
	ssc.setToUdf();

    ret = &ssc.distribution();
    if ( !ret->isEmpty() )
    {
	iop.set( sKey::Source(), fullscan ? sKeyFullScan()
					  : sKeyPartialScan() );
	ssc.fillPar( iop );
	File::Path fp( ioobj_->mainFileName() );
	fp.setExtension( sStatsFileExtension() );
	iop.write( fp.fullPath(), sKey::Stats() );
    }

    return ret;
}


bool SeisIOObjInfo::getBPS( int& bps, int icomp ) const
{
    mChk(false);
    if ( is2D() || isPS() )
    {
	DataCharacteristics dc;
	getDataChar( dc );
	bps = dc.nrBytes();
	if ( icomp < 0 && isSteering(*ioobj_) )
	    bps *= 2;
	return true;
    }

    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		 ioobj_->createTranslator() );
    if ( !sttr )
	{ pErrMsg("No Translator!"); bps = 4; return false; }

    Conn* conn = ioobj_->getConn( Conn::Read );
    bool isgood = sttr->initRead(conn,Seis::Scan);
    bps = 0;
    if ( isgood )
    {
	ObjectSet<SeisTrcTranslator::TargetComponentData>& comps
		= sttr->componentInfo();
	for ( int idx=0; idx<comps.size(); idx++ )
	{
	    int thisbps = (int)comps[idx]->datachar_.nrBytes();
	    if ( icomp < 0 )
		bps += thisbps;
	    else if ( icomp == idx )
		bps = thisbps;
	}
    }

    if ( bps == 0 )
	bps = 4;
    return isgood;
}


#define mGetZDomainGE \
    const GlobExpr zdomge( o2d.zdomky_.isEmpty() ? ZDomain::SI().key() \
						 : o2d.zdomky_.buf() )

void SeisIOObjInfo::getGeomIDs( GeomIDSet& geomids ) const
{
    if ( !isOK() )
	return;
    if ( isPS() )
	{ SPSIOPF().getGeomIDs( *ioobj_, geomids ); return; }
    if ( !is2D() )
	return;

    Seis2DDataSet ds2d( *ioobj_ );
    ds2d.getGeomIDs( geomids );
}


bool SeisIOObjInfo::isCompatibleType( const char* typestr1,
				      const char* typestr2 )
{
    const FixedString typ1( typestr1 ); const FixedString typ2( typestr2 );
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
	{ SPSIOPF().getLineNames( *ioobj_, bss ); return; }
   if ( !isOK() || !is2D() || isPS() )
       return;

    Seis2DDataSet ds2d( *ioobj_ );
    if ( ds2d.nrLines() == 0 )
	return;
    mGetZDomainGE;

    BufferStringSet rejected;
    for ( int idx=0; idx<ds2d.nrLines(); idx++ )
    {
	const char* nm = ds2d.lineName(idx);
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


bool SeisIOObjInfo::getRanges( const GeomID geomid,
			       StepInterval<int>& trcrg,
			       StepInterval<float>& zrg ) const
{
    mChk(false);

    PtrMan<Seis2DDataSet> dataset = new Seis2DDataSet( *ioobj_ );
    return dataset->getRanges( geomid, trcrg, zrg );
}


static BufferStringSet defaulttypes_;
static DBKeySet defaultids_;


void SeisIOObjInfo::initDefault( const char* typ )
{
    if ( defaulttypes_.isPresent(typ) )
	return;

    IOObjContext ctxt( SeisTrcTranslatorGroup::ioContext() );
    ctxt.toselect_.require_.set( sKey::Type(), typ );
    int nrpresent = 0;
    if ( DBM().isBad() )
	return;

    PtrMan<IOObj> ioobj = DBM().getFirst( ctxt, &nrpresent );
    if ( !ioobj || nrpresent > 1 )
	return;

    defaulttypes_.add( typ );
    defaultids_ += ioobj->key();
}


DBKey SeisIOObjInfo::getDefault( const char* typ )
{
    const int typidx = defaulttypes_.indexOf( typ );
    return typidx < 0 ? DBKey::getInvalid() : defaultids_[typidx];
}


void SeisIOObjInfo::setDefault( const DBKey& id, const char* typ )
{
    const int typidx = defaulttypes_.indexOf( typ );
    if ( typidx > -1 )
	defaultids_[typidx] = id;
    else
    {
	defaulttypes_.add( typ );
	defaultids_ += id;
    }
}


int SeisIOObjInfo::nrComponents( GeomID geomid ) const
{
    return getComponentInfo( geomid, 0 );
}


void SeisIOObjInfo::getComponentNames( BufferStringSet& nms,
				       GeomID geomid ) const
{
    getComponentInfo( geomid, &nms );
}


int SeisIOObjInfo::getComponentInfo( GeomID geomid, BufferStringSet* nms ) const
{
    int ret = 0;
    if ( nms )
	nms->erase();
    mChk(ret);

    if ( isPS() )
    {
	if ( nms )
	    nms->add( "Component 1" );
	return 1;
    }

    if ( !is2D() )
    {
	mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		     ioobj_->createTranslator() );
	if ( !sttr )
	    { pErrMsg("No Translator!"); return 1; }
	Conn* conn = ioobj_->getConn( Conn::Read );
	if ( sttr->initRead(conn,Seis::Scan) )
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
	    return 1;

	int lidx = dataset->indexOf( geomid );
	if ( lidx < 0 )
	    lidx = 0;

	uiRetVal uirv;
	PtrMan<Seis2DTraceGetter> getter = dataset->traceGetter(
					dataset->geomID(lidx), 0, uirv );
	if ( !uirv.isOK() )
	    return 1;

	BufferStringSet names;
	if ( !nms )
	    nms = &names;

	getter->getComponentInfo( *nms );
	return nms->size();
    }

    return ret;
}


bool SeisIOObjInfo::hasData( GeomID geomid )
{
    const BufferString linenm = geomid.name();
    ConstRefMan<DBDir> dbdir = DBM().fetchDir( IOObjContext::Seis );
    if ( !dbdir )
	return false;

    DBDirIter iter( *dbdir );
    while ( iter.next() )
    {
	const IOObj& ioobj = iter.ioObj();
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
{ getDataSetNamesForLine( SurvGeom::getGeomID(lnm), datasets, o2d ); }

void SeisIOObjInfo::getDataSetNamesForLine( GeomID geomid,
					    BufferStringSet& datasets,
					    Opts2D o2d )
{
    if ( mIsUdfGeomID(geomid) )
	return;

    IOObjContext ctxt( mIOObjContext(SeisTrc2D) );
    const DBDirEntryList del( ctxt );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj& ioobj = del.ioobj( idx );

	if ( !o2d.zdomky_.isEmpty() )
	{
	    const FixedString zdomkey = ioobj.pars().find( ZDomain::sKey() );
	    if ( o2d.zdomky_ != zdomkey )
		continue;
	}

	if ( o2d.steerpol_ != 2 )
	{
	    const bool issteering = isSteering( ioobj );
	    const bool wantsteering = o2d.steerpol_ == 1;
	    if ( issteering != wantsteering ) continue;
	}

	Seis2DDataSet ds( ioobj );
	if ( ds.isPresent(geomid) )
	    datasets.add( ioobj.name() );
    }
}


bool SeisIOObjInfo::isFullyRegular() const
{
    PtrMan<Translator> trl = ioobj_->createTranslator();
    mDynamicCastGet(SeisTrcTranslator*,strl,trl.ptr())
    if ( !strl ) return false;

    Conn* conn = ioobj_->getConn( Conn::Read );
    if ( !strl->initRead(conn,Seis::Scan) )
	return false;

    return strl->packetInfo().fullyrectandreg;
}


void SeisIOObjInfo::getLinesWithData( BufferStringSet& lnms, GeomIDSet& gids )
{
    SurvGeom::list2D( gids, &lnms );
    BoolTypeSet hasdata( gids.size(), false );

    ConstRefMan<DBDir> dbdir = DBM().fetchDir( IOObjContext::Seis );
    if ( !dbdir )
	return;

    DBDirIter iter( *dbdir );
    while ( iter.next() )
    {
	const IOObj& ioobj = iter.ioObj();
	GeomIDSet dsgids;
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


void SeisIOObjInfo::saveDisplayPars( const IOPar& par )
{
    if ( bad_ )
	return;

    File::Path fp( ioobj_->mainFileName() );
    fp.setExtension( sParFileExtension() );
    par.write( fp.fullPath(), sKey::Pars() );
}


void SeisIOObjInfo::getUserInfo( uiPhraseSet& inf ) const
{
    if ( !isOK() )
	{ inf.add( uiStrings::sNoInfoAvailable() ); return; }

    getCommonUserInfo( inf );
    if ( isPS() )
	getPreStackUserInfo( inf );
    else
	getPostStackUserInfo( inf );
}


void SeisIOObjInfo::getCommonUserInfo( uiPhraseSet& inf ) const
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
    inf.addKeyValue( key, uiStrings::sRangeTemplate(true) \
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
		if ( SI().xyInFeet() )
		    area *= (mFromFeetFactorD * mFromFeetFactorD);
	    }
	    else
	    {
		area = SI().getArea( cs.hsamp_.inlRange(),
				     cs.hsamp_.crlRange() );
	    }

	    inf.addKeyValue( uiStrings::sArea(),
		     getAreaString(mCast(float,area),SI().xyInFeet(),2,true) );

	    StepInterval<float> dispzrg( cs.zsamp_ );
	    dispzrg.scale( (float)zddef.userFactor() );
	    inf.addKeyValue( zddef.getRange().withUnit(zddef.unitStr()),
		    uiStrings::sRangeTemplate(true)
		    .arg(dispzrg.start).arg(dispzrg.stop).arg(dispzrg.step) );
	}
    }

    if ( !ioobj_->pars().isEmpty() )
    {
	const IOPar& pars = ioobj_->pars();
	FixedString parstr = pars.find( sKey::Type() );
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
		    dispzrg.scale( (float)ZDomain::Depth().userFactor() );
		    keystr = tr("Depth Range")
			    .withUnit( ZDomain::Depth().unitStr() );
		}

		else
		{
		    dispzrg.start = 2 * sizrg.start / topvavg.stop;
		    dispzrg.stop = 2 * sizrg.stop / botvavg.start;
		    dispzrg.step = (dispzrg.stop-dispzrg.start)
					/ sizrg.nrSteps();
		    dispzrg.scale( (float)ZDomain::Time().userFactor() );
		    keystr = tr("Time Range")
			    .withUnit( ZDomain::Time().unitStr() );
		}

		inf.addKeyValue( keystr, uiStrings::sRangeTemplate(true)
		    .arg(dispzrg.start).arg(dispzrg.stop).arg(dispzrg.step) );
	    }
	}
    }

    DataCharacteristics dc;
    getDataChar( dc );
    const BufferString dcstr( DataCharacteristics::toString(dc.userType()) );
    inf.addKeyValue( uiStrings::sStorage(), dcstr );
}


void SeisIOObjInfo::getPostStackUserInfo( uiPhraseSet& inf ) const
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


void SeisIOObjInfo::getPreStackUserInfo( uiPhraseSet& inf ) const
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
