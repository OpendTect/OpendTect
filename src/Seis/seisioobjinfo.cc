/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisioobjinfo.h"

#include "bufstringset.h"
#include "cbvsreadmgr.h"
#include "conn.h"
#include "datadistributiontools.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "ptrman.h"
#include "segydirecttr.h"
#include "segytr.h"
#include "seis2ddata.h"
#include "seis2dlineio.h"
#include "seisbuf.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "seisread.h"
#include "seisstatscollector.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "statrand.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "transl.h"
#include "trckeyzsampling.h"
#include "uistrings.h"
#include "unitofmeasure.h"
#include "veldesc.h"
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


Seis::ObjectSummary::ObjectSummary( const IOObj& ioobj,
				    const Pos::GeomID& geomid )
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


void Seis::ObjectSummary::init2D( const Pos::GeomID& geomid )
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
	datachar_ = trl.componentInfo().first()->org.datachar_;

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
{
    ioobj_ = IOMan::isOK() ? IOM().get(id) : nullptr;
    setType();
}


SeisIOObjInfo::SeisIOObjInfo( const DBKey& dbkey )
{
    surveychanger_ = new SurveyChanger( dbkey.surveyDiskLocation() );
    ioobj_ = IOMan::isOK() ? IOM().get( dbkey ) : nullptr;
    setType();
}


SeisIOObjInfo::SeisIOObjInfo( const char* ioobjnm, Seis::GeomType geomtype )
    : geomtype_(geomtype)
    , ioobj_(nullptr)
{
    if ( IOMan::isOK() )
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
    }

    setType();
}


SeisIOObjInfo::SeisIOObjInfo( const char* ioobjnm )
    : ioobj_(nullptr)
{
    if ( IOMan::isOK() )
    {
	IOM().to( IOObjContext::Seis );
	ioobj_ = IOM().getLocal( ioobjnm, nullptr );
    }

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
    if ( &sii == this )
	return *this;

    delete ioobj_;
    ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : nullptr;
    geomtype_ = sii.geomtype_;
    errmsg_ = sii.errmsg_;
    if ( surveychanger_ && surveychanger_->hasChanged() )
    {
	SurveyDiskLocation sdl = surveychanger_->changedToSurvey();
	delete surveychanger_;
	surveychanger_ = new SurveyChanger( sdl );
    }

    return *this;
}

bool SeisIOObjInfo::checkAndInitTranslRead( SeisTrcTranslator* sttr,
					    Seis::ReadMode rm ) const
{
    if ( !sttr )
    {
	errmsg_ = uiStrings::phrCannotOpen( ioobj_->uiName() );
	objstatus_ = IOObj::Status::LibraryNotLoaded;
	ioobj_->setStatus( objstatus_ );
	return false;
    }

    bool ret = sttr->initRead( ioobj_->getConn(Conn::Read),rm );
    if ( !ret )
    {
	errmsg_ = sttr->errMsg();
	objstatus_ = sttr->objStatus();
	ioobj_->setStatus( objstatus_ );
    }
    else
    {
	objstatus_= IOObj::Status::OK;
	ioobj_->setStatus( objstatus_ );
    }

    return ret;
}


bool SeisIOObjInfo::isOK( bool createtr ) const
{
    if ( !errmsg_.isEmpty() )
	return false;

    if ( !createtr )
	return true;

    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		 ioobj_->createTranslator())
    return checkAndInitTranslRead( sttr.ptr() );
}


void SeisIOObjInfo::setType()
{
    if ( !ioobj_ )
    {
	errmsg_ = uiStrings::phrCannotFindObjInDB();
	objstatus_ = IOObj::Status::FileNotPresent;
	return;
    }

    const BufferString trgrpnm( ioobj_->group() );
    bool isps = false;
    if ( SeisTrcTranslator::isPS(*ioobj_) )
	isps = true;

    ioobj_->pars().getYN( SeisTrcTranslator::sKeyIsPS(), isps );

    if ( !isps && ioobj_->group()!=mTranslGroupName(SeisTrc) &&
	    ioobj_->group()!=mTranslGroupName(SeisTrc2D) )
    {
	errmsg_ = uiStrings::phrSelectObjectWrongType( tr("Seismic object.") );
	objstatus_ = IOObj::Status::WrongObject;
	return;
    }

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


#define mChk(ret) \
    if ( !ioobj_ ) \
    { \
	if ( errmsg_.isEmpty() ) \
	{ \
	    errmsg_ = uiStrings::phrCannotFindObjInDB(); \
	    objstatus_ = IOObj::Status::FileNotPresent; \
	} \
\
	return ret; \
    }

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


const ZDomain::Info& SeisIOObjInfo::zDomain() const
{
    mChk( SI().zDomainInfo() );
    return SeisStoreAccess::zDomain( ioobj_ );
}


const ZDomain::Def& SeisIOObjInfo::zDomainDef() const
{
    return zDomain().def_;
}


bool SeisIOObjInfo::isTime() const
{
    mChk( SI().zIsTime() );
    return zDomain().isTime();
}


bool SeisIOObjInfo::isDepth() const
{
    return !isTime();
}


bool SeisIOObjInfo::zInMeter() const
{
    mChk( SI().zInMeter() );
    return zDomain().isDepthMeter();
}


bool SeisIOObjInfo::zInFeet() const
{
    mChk( SI().zInFeet() );
    return zDomain().isDepthFeet();
}


const UnitOfMeasure* SeisIOObjInfo::zUnit() const
{
    mChk(nullptr);
    return UnitOfMeasure::zUnit( zDomain() );
}


const UnitOfMeasure* SeisIOObjInfo::offsetUnit() const
{
    mChk(nullptr);
    bool hasoffsetsunit;
    return isPS() ? SeisPSIOProvider::offsetUnit( ioobj_, hasoffsetsunit )
		  : nullptr;
}


Seis::OffsetType SeisIOObjInfo::offsetType() const
{
    Seis::OffsetType typ = Seis::OffsetType::OffsetMeter;
    mChk(typ);
    if ( isPS() )
	Seis::getOffsetType( ioobj_->pars(), typ );

    return typ;
}


bool SeisIOObjInfo::isCorrected() const
{
    mChk(true);
    bool iscorr = true;
    if ( isPS() )
	Seis::getGatherCorrectedYN( ioobj_->pars(), iscorr );

    return iscorr;
}


ZSampling SeisIOObjInfo::getConvertedZrg( const ZSampling& zsamp ) const
{
    ZSampling ret = zsamp;
    mChk(ret);
    if ( zDomain() == SI().zDomainInfo() || zDomain().isTime() )
	return ret;

    const UnitOfMeasure* zuom = zUnit();
    if ( !zuom )
	return ret;

    const UnitOfMeasure* depthuom = UnitOfMeasure::surveyDefDepthUnit();
    convValue( ret.start_, zuom, depthuom );
    convValue( ret.stop_, zuom, depthuom );
    convValue( ret.step_, zuom, depthuom );
    return ret;
}


od_int64 SeisIOObjInfo::SpaceInfo::expectedSize() const
{
    if ( expectednrsamps<0 || expectednrtrcs<0 )
	return -1;

    od_int64 totnrbytes = expectednrsamps;
    totnrbytes *= expectednrtrcs;
    totnrbytes *= maxbytespsamp;
    return totnrbytes;
}



int SeisIOObjInfo::SpaceInfo::expectedMBs() const
{
    return expectedSize() / mDef1MB ;
}


int SeisIOObjInfo::expectedMBs( const SpaceInfo& si ) const
{
    mChk(-1);

    const int sz = expectedSize( si );
    const double bytes2mb = 9.53674e-7;
    return sCast(int,((sz * bytes2mb) + .5));
}


od_int64 SeisIOObjInfo::expectedSize( const SpaceInfo& si ) const
{
    mChk(-1);

    od_int64 nrbytes = si.expectedSize();
    if ( nrbytes < 0 || isPS() )
	return nrbytes;

    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		 ioobj_->createTranslator())
    if ( !sttr )
	return -1;

    od_int64 overhead = sttr->bytesOverheadPerTrace();
    od_int64 sz = si.expectednrsamps;
    sz *= si.maxbytespsamp;
    sz = (sz + overhead) * si.expectednrtrcs;
    return sz;
}


od_int64 SeisIOObjInfo::getFileSize() const
{
    const FilePath filepath = ioobj_->fullUserExpr();
    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		 ioobj_->createTranslator())
    if ( !checkAndInitTranslRead(sttr.ptr()) )
	return -1;

    return sttr->getFileSize();
}


int SeisIOObjInfo::nrImpls() const
{
    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		 ioobj_->createTranslator())

    return ioobj_->nrImpls() - 1;
}


void SeisIOObjInfo::getAllFileNames( BufferStringSet& filenames,
				     bool forui ) const
{
    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		 ioobj_->createTranslator())
    if ( !checkAndInitTranslRead(sttr.ptr()) )
	return;

    sttr->getAllFileNames( filenames, forui );
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
    if ( !checkAndInitTranslRead(sttr.ptr(),Seis::PreScan) )
	return false;

    ObjectSet<SeisTrcTranslator::TargetComponentData>& comps
		= sttr->componentInfo();
    if ( comps.isEmpty() )
	return false;

    dc = comps.first()->org.datachar_;
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


RefMan<FloatDistrib> SeisIOObjInfo::getDataDistribution( int icomp ) const
{
    mChk(nullptr);
    IOPar iop;
    RefMan<FloatDistrib> ret;
    const bool allcomps = icomp < 0 || (icomp == 0 && nrComponents()==1);
    if ( haveStats() && getStats(iop) )
    {
	PtrMan<IOPar> comppars;
	if ( !allcomps )
	    comppars = iop.subselect( IOPar::compKey(sKey::Component(),icomp) );

	if ( allcomps || (!allcomps && comppars) )
	{
	    const IOPar& par = allcomps ? iop : *comppars.ptr();
	    PtrMan<IOPar> dpar = par.subselect( sKey::Distribution() );
	    if ( dpar && !dpar->isEmpty() )
	    {
		ret = new FloatDistrib;
		DataDistributionChanger<float> chgr( *ret );
		chgr.usePar( *dpar );
		return ret;
	    }
	}
    }

    // No .stats file. Extract stats right now
    SeisTrcReader rdr( *ioobj_ );
    if ( !allcomps )
	rdr.setComponent( icomp );

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
	SpaceInfo si;
	getDefSpaceInfo( si );
	const int ddsamples = 100000;
	const int nrperline = ddsamples/cd.size() + (ddsamples%cd.size()!=0);
	const int trcsperline = nrperline/si.expectednrsamps +
					    (nrperline%si.expectednrsamps!=0);

	for ( int lidx=0; lidx<cd.size(); lidx++ )
	{
	    PtrMan<SeisTrcBuf> traces;
	    cdp.lidx_ = lidx;
	    const auto& segs = cd.get( cdp.lidx_ )->segments_;
	    for ( int tidx=0; tidx<trcsperline; tidx++ )
	    {
		cdp.segnr_ = randgen.getIndex( segs.size() );
		cdp.sidx_ = randgen.getIndex(segs.get(cdp.segnr_).nrSteps()+1);
		const BinID bid = cd.binID( cdp );
		if ( trl->supportsGoTo() )
		{
		    SeisTrc trc;
		    if ( !trl->goTo(bid) || !rdr.get(trc) || trc.isNull() )
			continue;

		    ssc.useTrace( trc );
		}
		else if ( trl->is2D() )
		{
		    if ( !traces )
		    {
			traces = new SeisTrcBuf(true);
			const auto* seisdata = rdr.dataSet();
			if ( !seisdata )
			    continue;
			const Pos::GeomID geomid( bid.lineNr() );
			PtrMan<Executor> ex = seisdata->lineFetcher( geomid,
								     *traces );
			if ( !ex || !ex->execute() || traces->isEmpty() )
			    continue;
		    }
		    const int idx = traces->find( bid, true );
		    if ( idx==-1 )
			continue;

		    const auto* trc = traces->get( idx );
		    if ( !trc || trc->isNull() )
			continue;

		    ssc.useTrace( *trc );
		}
	    }
	}
    }

    ret = &ssc.distribution();
    if ( !ret->isEmpty() )
    {
	IOPar subiop;
	subiop.set( sKey::Source(), "Partial Scan" );
	ssc.fillPar( subiop );
	if ( allcomps )
	    iop.merge( subiop );
	else
	    iop.mergeComp( subiop, IOPar::compKey(sKey::Component(),icomp) );

	FilePath fp( ioobj_->mainFileName() );
	fp.setExtension( sStatsFileExtension() );
	iop.write( fp.fullPath(), sKey::Stats() );
    }

    return ret;
}


Interval<float> SeisIOObjInfo::getDataRange( int icomp ) const
{
    const bool allcomps = icomp < 0 || (icomp == 0 && nrComponents()==1);
    const int compnr = allcomps ? -1 : icomp;
    PtrMan<SeisTrcReader> rdr = new SeisTrcReader( *ioobj_ );
    if ( !allcomps )
	rdr->setComponent( icomp );

    if ( rdr->prepareWork() )
    {
	SeisTrcTranslator* trl = rdr->seisTranslator();
	if ( trl )
	{
	    Interval<float> ret = trl->getDataRange( compnr );
	    if ( !ret.isUdf() )
		return ret;
	}
    }

    rdr = nullptr;
    ConstRefMan<FloatDistrib> distrib = getDataDistribution( compnr );
    return distrib ? distrib->dataRange() : Interval<float>::udf();
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
    bool isgood = checkAndInitTranslRead( sttr.ptr() );
    if ( !isgood )
    {
	bps = 4;
	return isgood;
    }

    bps = 0;
    ObjectSet<SeisTrcTranslator::TargetComponentData>& comps
	    = sttr->componentInfo();
    for ( int idx=0; idx<comps.size(); idx++ )
    {
	int thisbps = sCast(int,comps[idx]->datachar_.nrBytes());
	if ( icomp < 0 )
	    bps += thisbps;
	else if ( icomp == idx )
	    bps = thisbps;
    }

    if ( bps == 0 )
	bps = 4;

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

    if ( !is2D() )
	return;

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

   if ( !isOK() || !is2D() || isPS() )
	return;

    PtrMan<Seis2DDataSet> dset = new Seis2DDataSet( *ioobj_ );
    if ( dset->nrLines() == 0 )
	return;

    for ( int idx=0; idx<dset->nrLines(); idx++ )
    {
	const char* nm = dset->lineName(idx);
	if ( bss.isPresent(nm) )
	    continue;

	bss.add( nm );
    }

    bss.sort();
}


bool SeisIOObjInfo::getRanges( const Pos::GeomID& geomid,
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
    if ( typs.isPresent(typ) )
	return;

    IOObjContext ctxt( SeisTrcTranslatorGroup::ioContext() );
    ctxt.requireType( typ );
    int nrpresent = 0;
    PtrMan<IOObj> ioobj = IOM().getFirst( ctxt, &nrpresent );
    if ( !ioobj || nrpresent > 1 )
	return;

    typs.add( typ );
    getIDs() += ioobj->key();
}


const MultiID& SeisIOObjInfo::getDefault( const char* typ )
{
    const int typidx = getTypes().indexOf( typ );
    return typidx < 0 ? MultiID::udf() : getIDs()[typidx];
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


int SeisIOObjInfo::nrComponents( const Pos::GeomID& geomid ) const
{
    return getComponentInfo( geomid, nullptr );
}


void SeisIOObjInfo::getComponentNames( BufferStringSet& nms,
				       const Pos::GeomID& geomid ) const
{
    getComponentInfo( geomid, &nms );
}


void SeisIOObjInfo::getCompNames( const MultiID& mid, BufferStringSet& nms )
{
    SeisIOObjInfo ioobjinf( mid );
    ioobjinf.getComponentNames( nms, Pos::GeomID::udf() );
}


int SeisIOObjInfo::getComponentInfo( const Pos::GeomID& geomid,
				     BufferStringSet* nms ) const
{
    int ret = 0;
    if ( nms )
	nms->erase();

    mChk(ret);
    if ( isPS() )
	return ret;

    if ( !is2D() )
    {
	mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		     ioobj_->createTranslator())
	if ( !sttr )
	    return ret;

	const bool supportsmulticomp = sttr->supportsMultiCompTrc();
	if ( !supportsmulticomp )
	    return 1; // Just 1 component in this case

	if ( !checkAndInitTranslRead(sttr.ptr()) )
	    return ret;

	ret = sttr->componentInfo().size();
	if ( nms )
	{
	    for ( int icomp=0; icomp<ret; icomp++ )
		nms->add( sttr->componentInfo()[icomp]->name() );
	}
    }
    else
    {
	PtrMan<Seis2DDataSet> dataset = new Seis2DDataSet( *ioobj_ );
	if ( !dataset || dataset->nrLines() == 0 )
	    return ret;

	int lidx = dataset->indexOf( geomid );
	if ( lidx < 0 )
	    lidx = 0;

	SeisTrcBuf tbuf( true );
	Executor* ex = dataset->lineFetcher( dataset->geomID(lidx), tbuf, 1 );
	if ( ex )
	    ex->doStep();

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


bool SeisIOObjInfo::hasData( const Pos::GeomID& geomid )
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
{
    getDataSetNamesForLine( Survey::GM().getGeomID(lnm), datasets, o2d );
}





void SeisIOObjInfo::getDataSetNamesForLine( const Pos::GeomID& geomid,
					    BufferStringSet& datasets,
					    const Opts2D& o2d )
{
    TypeSet<MultiID> mids;
    getDataSetInfoForLine( geomid, mids, datasets, o2d );
}


void SeisIOObjInfo::getDataSetIDsForLine( const Pos::GeomID& geomid,
					  TypeSet<MultiID>& mids,
					  const Opts2D& o2d )
{
    BufferStringSet datasets;
    getDataSetInfoForLine( geomid, mids, datasets, o2d );
}


void SeisIOObjInfo::getDataSetInfoForLine( const Pos::GeomID& geomid,
					   TypeSet<MultiID>& mids,
					   BufferStringSet& datasets,
					   const Opts2D& o2d )
{
    if ( geomid.isUdf() )
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
	    const BufferString dt = ioobj->pars().find( sKey::Type() );
	    const bool issteering = dt==sKey::Steering();
	    const bool wantsteering = o2d.steerpol_ == 1;
	    if ( issteering != wantsteering )
		continue;
	}

	Seis2DDataSet ds( *ioobj );
	if ( ds.isPresent(geomid) )
	{
	    mids.add( ioobj->key() );
	    datasets.add( ioobj->name() );
	}
    }
}


bool SeisIOObjInfo::isFullyRectAndRegular() const
{
    mChk(false)
    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		 ioobj_->createTranslator())
    if ( !checkAndInitTranslRead(sttr.ptr()) )
	return false;

    PosInfo::SortedCubeData scdata;
    if ( !sttr->getGeometryInfo(scdata) )
	return false;

    if ( scdata.totalSize()<1 )
	return false;

    return scdata.isFullyRectAndReg();
}


void SeisIOObjInfo::getLinesWithData( BufferStringSet& lnms,
				      TypeSet<Pos::GeomID>& gids )
{
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

    const ZDomain::Info& zinfo = zDomain();
    if ( !is2d )
    {
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
		     getAreaString(float(area),SI().xyInFeet(),
				   SI().nrXYDecimals(),true) );

	    ZSampling zrg = cs.zsamp_;
	    const int nrzdec = zinfo.nrDecimals( zrg.step_, false );
	    zrg.scale( zinfo.userFactor() );
	    const uiString zrgstartuistr = toUiStringDec( zrg.start_, nrzdec );
	    const uiString zrgstopuistr = toUiStringDec( zrg.stop_, nrzdec );
	    const uiString zrgstepuistr = toUiStringDec( zrg.step_, nrzdec );
	    inf.addKeyValue( zinfo.getRange(),
		    toUiString("%1 - %2 [%3]")
                             .arg( zrgstartuistr )
                             .arg( zrgstopuistr )
                             .arg( zrgstepuistr ) );
	}
    }

    if ( !ioobj_->pars().isEmpty() )
    {
	const IOPar& pars = ioobj_->pars();
	BufferString parstr = pars.find( sKey::Type() );
	if ( !parstr.isEmpty() )
	    inf.addKeyValue( uiStrings::sType(), parstr );

	parstr = pars.find( "Optimized direction" );
	if ( !parstr.isEmpty() )
	    inf.addKeyValue( tr("Optimized direction"), parstr );

	VelocityDesc desc;
	if ( desc.usePar(pars) )
	{
	    inf.addKeyValue( tr("Velocity Type"), OD::toString(desc.type_) );
	    TrcKeyZSampling cs;
	    if ( !is2d && getRanges(cs) )
	    {
		const ZDomain::Info& todomain = zinfo.isTime()
				? (SI().depthsInFeet() ? ZDomain::DepthFeet()
						       : ZDomain::DepthMeter())
				: ZDomain::TWT();
		ZSampling zrg = getConvertedZrg( cs.zsamp_ );
		zrg = VelocityStretcher::getWorkZSampling( zrg, zinfo,
							   todomain, pars );
		if ( !zrg.isUdf() )
		{
		    zrg.scale( todomain.def_.userFactor() );
		    const uiString keystr = tr("%1 Range ")
						.arg( todomain.userName() )
						.withUnit( todomain.unitStr() );
		    inf.addKeyValue( keystr, toUiString("%1 - %2 [%3]")
						.arg(zrg.start_)
						.arg(zrg.stop_)
						.arg(zrg.step_) );
		}
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
