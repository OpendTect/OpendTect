/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : June 2005
-*/


#include "seisioobjinfo.h"

#include "bufstringset.h"
#include "cbvsio.h"
#include "conn.h"
#include "genc.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "globexpr.h"
#include "dbdir.h"
#include "dbman.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "ptrman.h"
#include "seistrctr.h"
#include "seis2dlineio.h"
#include "seis2ddata.h"
#include "seisbuf.h"
#include "seispsioprov.h"
#include "seisselection.h"
#include "seispacketinfo.h"
#include "seistrc.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "zdomain.h"


Seis::ObjectSummary::ObjectSummary( const DBKey& dbkey )
    : ioobjinfo_(*new SeisIOObjInfo(dbkey))		{ init(); }
Seis::ObjectSummary::ObjectSummary( const IOObj& ioobj )
    : ioobjinfo_(*new SeisIOObjInfo(ioobj))		{ init(); }
Seis::ObjectSummary::ObjectSummary( const Seis::ObjectSummary& oth )
    : ioobjinfo_(*new SeisIOObjInfo(oth.ioobjinfo_))	{ init(); }

Seis::ObjectSummary::~ObjectSummary()
{
    delete &ioobjinfo_;
}


Seis::ObjectSummary& Seis::ObjectSummary::operator =(
						const Seis::ObjectSummary& oth )
{
    const_cast<SeisIOObjInfo&>( ioobjinfo_ ) = oth.ioobjinfo_;
    init();

    return *this;
}


void Seis::ObjectSummary::init()
{
    bad_ = !ioobjinfo_.isOK();
    if ( bad_ ) return;

    ioobjinfo_.getDataChar( datachar_ );
    TrcKeyZSampling tkzs( false );
    ioobjinfo_.getRanges( tkzs );
    zsamp_ = tkzs.zsamp_;
    geomtype_ = ioobjinfo_.geomType();
    ioobjinfo_.getComponentNames( compnms_, tkzs.hsamp_.getGeomID() );

    nrcomp_ = compnms_.size();
    nrsamppertrc_ = tkzs.nrZ();
    nrbytespersamp_ = datachar_.nrBytes();
    nrdatabytespespercomptrc_ = nrbytespersamp_ * nrsamppertrc_;
    nrdatabytespertrc_ = nrdatabytespespercomptrc_ * nrcomp_;

    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		    ioobjinfo_.ioObj()->createTranslator() );
    if ( !sttr )
	{ pErrMsg("Translator not SeisTrcTranslator!"); bad_ = true; return; }

    datatype_ = sttr->dataType();
    nrbytestrcheader_ = sttr->bytesOverheadPerTrace();
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
	: ioobj_(ioobj ? ioobj->clone() : 0)		{ setType(); }
SeisIOObjInfo::SeisIOObjInfo( const IOObj& ioobj )
	: ioobj_(ioobj.clone())				{ setType(); }
SeisIOObjInfo::SeisIOObjInfo( const DBKey& id )
	: ioobj_(DBM().get(id))				{ setType(); }


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
    if ( bad_ ) return;

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
	expectednrsamps = SI().zRange(false).nrSteps() + 1;
    if ( expectednrtrcs < 0 )
	expectednrtrcs = mCast( int, SI().sampling(false).hsamp_.totalNr() );
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

    spinf.expectednrsamps = cs.zsamp_.nrSteps() + 1;
    spinf.expectednrtrcs = mCast( int, cs.hsamp_.totalNr() );
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
		if ( !File::exists(fullnm) ) break;

		totalsz += File::getKbSize( fullnm );
		nrfiles++;
	    }
	}
    }

    return totalsz;
}


od_int64 SeisIOObjInfo::getFileSize() const
{
    const char* fnm = ioobj_->mainFileName();
    int nrfiles;
    return getFileSize( fnm, nrfiles );
}


bool SeisIOObjInfo::getRanges( TrcKeyZSampling& cs ) const
{
    mChk(false);
    mDynamicCastGet(IOStream*,iostrm,ioobj_)
    cs.init( true );
    if ( is2D() || (iostrm && iostrm->isMulti()) )
	return false;

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
    Translator* trl = ioobj_->createTranslator();
    if ( !trl )
	{ pErrMsg("No Translator!"); return false; }
    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		 trl );
    if ( !sttr )
	{ pErrMsg("Translator not SeisTrcTranslator!"); return false; }

    Conn* conn = ioobj_->getConn( Conn::Read );
    if ( !sttr->initRead(conn,Seis::Scan) )
	return false;

    ObjectSet<SeisTrcTranslator::TargetComponentData>& comps
		= sttr->componentInfo();
    if ( comps.isEmpty() )
	return false;

    dc = comps.first()->datachar_;
    return true;
}


bool SeisIOObjInfo::getPars( IOPar& iop ) const
{
    mChk(false);
    File::Path fp( ioobj_->mainFileName() );
    fp.setExtension( sParFileExtension() );
    return iop.read( fp.fullPath(), sKey::Pars() );
}


bool SeisIOObjInfo::getStats( IOPar& iop ) const
{
    mChk(false);
    File::Path fp( ioobj_->mainFileName() );
    fp.setExtension( sStatsFileExtension() );
    return iop.read( fp.fullPath(), sKey::Stats() );
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
		 ioobj_->createTranslator() );
    if ( !sttr )
	{ pErrMsg("No Translator!"); return false; }

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

    if ( bps == 0 ) bps = 4;
    return isgood;
}


#define mGetZDomainGE \
    const GlobExpr zdomge( o2d.zdomky_.isEmpty() ? ZDomain::SI().key() \
						 : o2d.zdomky_.buf() )

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
    {
	SPSIOPF().getLineNames( *ioobj_, bss );
	return;
    }

   if ( !isOK() || !is2D() || isPS() ) return;

    PtrMan<Seis2DDataSet> dset
	= new Seis2DDataSet( *ioobj_ );
    if ( dset->nrLines() == 0 )
	return;
    mGetZDomainGE;

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

    PtrMan<Seis2DDataSet> dataset = new Seis2DDataSet( *ioobj_ );
    return dataset->getRanges( geomid, trcrg, zrg );
}



static BufferStringSet& getTypes()
{
    mDefineStaticLocalObject( BufferStringSet, types, );
    return types;
}

static DBKeySet& getIDs()
{
    mDefineStaticLocalObject( DBKeySet, ids, );
    return ids;
}


void SeisIOObjInfo::initDefault( const char* typ )
{
    BufferStringSet& typs = getTypes();
    if ( typs.isPresent(typ) ) return;

    IOObjContext ctxt( SeisTrcTranslatorGroup::ioContext() );
    ctxt.toselect_.require_.set( sKey::Type(), typ );
    int nrpresent = 0;
    if ( DBM().isBad() )
	return;

    PtrMan<IOObj> ioobj = DBM().getFirst( ctxt, &nrpresent );
    if ( !ioobj || nrpresent > 1 )
	return;

    typs.add( typ );
    getIDs() += ioobj->key();
}


DBKey SeisIOObjInfo::getDefault( const char* typ )
{
    const int typidx = getTypes().indexOf( typ );
    return typidx < 0 ? DBKey::getInvalid() : getIDs()[typidx];
}


void SeisIOObjInfo::setDefault( const DBKey& id, const char* typ )
{
    BufferStringSet& typs = getTypes();
    DBKeySet& ids = getIDs();

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
    return getComponentInfo( geomid, 0 );
}


void SeisIOObjInfo::getComponentNames( BufferStringSet& nms,
				       Pos::GeomID geomid ) const
{
    getComponentInfo( geomid, &nms );
}


void SeisIOObjInfo::getCompNames( const DBKey& mid, BufferStringSet& nms )
{
    SeisIOObjInfo ioobjinf( mid );
    ioobjinf.getComponentNames( nms, mUdfGeomID );
}


int SeisIOObjInfo::getComponentInfo( Pos::GeomID geomid,
				     BufferStringSet* nms ) const
{
    int ret = 0; if ( nms ) nms->erase();
    mChk(ret);
    if ( isPS() )
	return 1;

    if ( !is2D() )
    {
	mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		     ioobj_->createTranslator() );
	if ( !sttr )
	    { pErrMsg("No Translator!"); return 0; }
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
	    return 0;

	int lidx = dataset->indexOf( geomid );
	if ( lidx < 0 ) lidx = 0;

	uiRetVal uirv;
	Seis2DTraceGetter* getter = dataset->traceGetter(
					dataset->geomID(lidx), 0, uirv );
	if ( !uirv.isOK() )
	    return 0;

	BufferStringSet names;
	if ( !nms )
	    nms = &names;

	getter->getComponentInfo( *nms );
	return nms->size();
    }

    return ret;
}


bool SeisIOObjInfo::hasData( Pos::GeomID geomid )
{
    const char* linenm = Survey::GM().getName( geomid );
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
{ getDataSetNamesForLine( Survey::GM().getGeomID(lnm), datasets, o2d ); }

void SeisIOObjInfo::getDataSetNamesForLine( Pos::GeomID geomid,
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
	    const FixedString dt = ioobj.pars().find( sKey::Type() );
	    const bool issteering = dt==sKey::Steering();
	    const bool wantsteering = o2d.steerpol_ == 1;
	    if ( issteering != wantsteering ) continue;
	}

	Seis2DDataSet ds( ioobj );
	if ( ds.isPresent(geomid) )
	    datasets.add( ioobj.name() );
    }
}


bool SeisIOObjInfo::isFullyRectAndRegular() const
{
    PtrMan<Translator> trl = ioobj_->createTranslator();
    mDynamicCastGet(SeisTrcTranslator*,strl,trl.ptr())
    if ( !strl ) return false;

    Conn* conn = ioobj_->getConn( Conn::Read );
    if ( !strl->initRead(conn,Seis::Scan) )
	return false;

    return strl->packetInfo().fullyrectandreg;
}


void SeisIOObjInfo::getLinesWithData( BufferStringSet& lnms,
				      TypeSet<Pos::GeomID>& gids )
{
    Survey::GMAdmin().updateGeometries( 0 );
    Survey::GM().getList( lnms, gids, true );
    BoolTypeSet hasdata( gids.size(), false );

    ConstRefMan<DBDir> dbdir = DBM().fetchDir( IOObjContext::Seis );
    if ( !dbdir )
	return;

    DBDirIter iter( *dbdir );
    while ( iter.next() )
    {
	const IOObj& ioobj = iter.ioObj();
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


void SeisIOObjInfo::saveDisplayPars( const IOPar& par )
{
    if ( bad_ )
	return;

    File::Path fp( ioobj_->mainFileName() );
    fp.setExtension( sParFileExtension() );
    par.write( fp.fullPath(), sKey::Pars() );
}
