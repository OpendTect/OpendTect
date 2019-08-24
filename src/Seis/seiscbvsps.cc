/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 21-1-1998
-*/


#include "seiscbvsps.h"

#include "cbvsreadmgr.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "seisbuf.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "seistrc.h"
#include "staticstring.h"
#include "survinfo.h"
#include "survgeom.h"
#include "trckey.h"
#include "uistrings.h"
#include "od_iostream.h"
#include <string.h>

static const char* cSampNmsFnm = "samplenames.txt";
static const char* cPosDataFnm = "posdata.txt";


SeisPS2DReader::SeisPS2DReader( const char* lnm )
    : lnm_(lnm)
    , geomid_(SurvGeom::getGeomID(lnm))
{}


SeisPS2DReader::SeisPS2DReader( Pos::GeomID geomid )
    : geomid_(geomid)
    , lnm_(geomid.name())
{}


SeisTrc* SeisPS2DReader::getTrc( int trcnr, int nr ) const
{
    return getTrace( TrcKey(geomid_,trcnr), nr );
}


bool SeisPS2DReader::getGath( int trcnr, SeisTrcBuf& b ) const
{
    return getGather( TrcKey(geomid_,trcnr), b );
}


class CBVSSeisPSIOProvider : public SeisPSIOProvider
{
public:
			CBVSSeisPSIOProvider() : SeisPSIOProvider(
				CBVSSeisTrcTranslator::translKey() ) {}

    virtual bool	canHandle( bool forread, bool for2d ) const
			{ return true; }

    SeisPS3DReader*	make3DReader( const char* dirnm, int inl ) const
			{ return new SeisCBVSPS3DReader(dirnm,inl); }
    SeisPSWriter*	make3DWriter( const char* dirnm ) const
			{ return new SeisCBVSPS3DWriter(dirnm); }
    SeisPS2DReader*	make2DReader( const char* dirnm, Pos::GeomID gid ) const
			{ return new SeisCBVSPS2DReader(dirnm,gid); }
    SeisPSWriter*	make2DWriter( const char* dirnm, Pos::GeomID gid ) const
			{ return new SeisCBVSPS2DWriter(dirnm,gid); }
    SeisPS2DReader*	make2DReader( const char* dirnm, const char* lnm ) const
			{ return new SeisCBVSPS2DReader(dirnm,lnm); }
    SeisPSWriter*	make2DWriter( const char* dirnm, const char* lnm ) const
			{ return new SeisCBVSPS2DWriter(dirnm,lnm); }

    bool		getGeomIDs(const char*,GeomIDSet&) const;
    bool		getLineNames(const char*,BufferStringSet&) const;

    static int		factid;
};

// This adds the CBVS type prestack seismics data storage to the factory
int CBVSSeisPSIOProvider::factid = SPSIOPF().add( new CBVSSeisPSIOProvider );


bool CBVSSeisPSIOProvider::getGeomIDs( const char* dirnm,
				       GeomIDSet& geomids ) const
{
    geomids.erase();
    DirList dl( dirnm, File::FilesInDir, "*.cbvs" );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString filenm = File::Path( dl.get(idx) ).baseName();
	char* capptr = filenm.find( '^' );
	if ( !capptr )
	    continue;
	BufferString geomidstr( ++capptr );
	const Pos::GeomID geomid( toInt(geomidstr) );
	if ( geomid.is2D() )
	    geomids += geomid;
    }

    return geomids.size();
}



bool CBVSSeisPSIOProvider::getLineNames( const char* dirnm,
					 BufferStringSet& linenms) const
{
    linenms.setEmpty();
    GeomIDSet geomids;
    if ( !getGeomIDs(dirnm,geomids) )
	return false;

    for ( int idx=0; idx<geomids.size(); idx++ )
	linenms.add( geomids[idx].name() );

    return !linenms.isEmpty();
}


SeisCBVSPSIO::SeisCBVSPSIO( const char* dirnm )
	: dirnm_(dirnm)
	, reqdtype_(OD::AutoDataRep)
	, tr_(0)
	, nringather_(1)
{
    BufferString& sm = const_cast<BufferString&>( selmask_ );
    sm = "*."; sm += CBVSSeisTrcTranslator::sKeyDefExtension();
}


SeisCBVSPSIO::~SeisCBVSPSIO()
{
    close();
}


void SeisCBVSPSIO::close()
{
    delete tr_; tr_ = 0;
    nringather_ = 1;
}


BufferString SeisCBVSPSIO::get2DFileName( Pos::GeomID geomid ) const
{
    File::Path fp( dirnm_ );
    BufferString fnm( fp.fileName(), "^", toString(geomid) );
    fp.add( fnm );
    fp.setExtension( "cbvs", false );

    fnm = fp.fullPath();
    return fnm;
}


BufferString SeisCBVSPSIO::get2DFileName( const char* lnm ) const
{
    const auto geomid = SurvGeom::getGeomID( lnm );
    return mIsUdfGeomID(geomid) ? BufferString::empty() : get2DFileName(geomid);
}



int SeisCBVSPSIO::getInlNr( const char* filenm )
{
    File::Path fp( filenm );
    BufferString fnm( fp.fileName() );
    char* ptr = fnm.getCStr();
    while ( *ptr && !iswdigit(*ptr) ) ptr++;
    while ( *ptr && iswdigit(*ptr) ) ptr++;
    *ptr = '\0';
    return fnm.isEmpty() ? -1 : fnm.toInt();
}


bool SeisCBVSPSIO::get3DFileNames( BufferStringSet& bss,
				   const Interval<int>* inlrg ) const
{
    bss.erase();
    if ( !dirNmOK(true) )
	return false;

    DirList dl( dirnm_, File::FilesInDir, selmask_.buf() );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	if ( inlrg )
	{
	    const int inl = getInlNr( dl.get(idx) );
	    if ( inl < 1 || !inlrg->includes(inl,true) )
		continue;

	    mDynamicCastGet(const StepInterval<int>*,si,inlrg)
	    if ( si )
	    {
		int snpinl = si->snap( inl );
		if ( snpinl != inl ) continue;
	    }
	}

	bss.add( dl.fullPath(idx) );
    }

    if ( bss.isEmpty() )
    {
	errmsg_ = tr("No matching files found in data store");
	return false;
    }
    return true;
}


bool SeisCBVSPSIO::getSampleNames( BufferStringSet& nms ) const
{
    const BufferString fnm( File::Path(dirnm_,cSampNmsFnm).fullPath() );
    od_istream strm( fnm );
    if ( !strm.isOK() )
	return false;

    nms.erase();
    BufferString nm;
    while ( strm.getLine(nm) && !nm.isEmpty() )
	nms.add( nm.buf() );

    return true;
}


bool SeisCBVSPSIO::setSampleNames( const BufferStringSet& nms ) const
{
    const BufferString fnm( File::Path(dirnm_,cSampNmsFnm).fullPath() );
    if ( nms.isEmpty() )
    {
	if ( File::exists(fnm) )
	    File::remove( fnm );
	return true;
    }

    od_ostream strm( fnm );
    if ( !strm.isOK() )
	return false;

    strm << nms.get(0);
    for ( int idx=1; idx<nms.size(); idx++ )
	strm << '\n' << nms.get(idx);
    return true;
}


void SeisCBVSPSIO::usePar( const IOPar& iopar )
{
    DataCharacteristics::getUserTypeFromPar( iopar, reqdtype_ );
}


bool SeisCBVSPSIO::dirNmOK( bool forread ) const
{
    if ( File::isDirectory(dirnm_) )
	return true;

    if ( forread )
    {
	errmsg_ = tr("Directory '%1' does not exist").arg( dirnm_ );
	return false;
    }

    File::createDir( dirnm_ );
    if ( !File::isDirectory(dirnm_) )
    {
	errmsg_ = tr("Cannot create directory '%1'").arg( dirnm_ );
	return false;
    }

    return true;
}


SeisTrc* SeisCBVSPSIO::readNewTrace( int crlnr ) const
{
    SeisTrc* trc = new SeisTrc;
    if ( !tr_->read(*trc) )
	{ delete trc; return 0; }
    if ( trc->info().lineNr() != crlnr )
	{ delete trc; return 0; }

    return trc;
}


bool SeisCBVSPSIO::goTo( int crlnr, int nr ) const
{
    if ( !tr_ ) return false;

    if ( !tr_->goTo( BinID(crlnr,nr+1) ) )
    {
	if ( !tr_->goTo( BinID(crlnr,1) ) )
	{ errmsg_ = tr("Crossline not present"); return false; }
	for ( int idx=1; idx<nr; idx++ )
	{
	    if ( !tr_->goTo( BinID(crlnr,idx+1) ) )
		{ tr_->goTo( BinID(crlnr,idx) ); break; }
	}
    }
    return true;
}


bool SeisCBVSPSIO::prepGather( int crlnr, SeisTrcBuf& gath ) const
{
    if ( !tr_ ) return false;

    gath.deepErase(); gath.setIsOwner( true );
    if ( !tr_->goTo(BinID(crlnr,1)) )
    {
	errmsg_ = tr("%1 not present").arg( uiStrings::sTraceNumber() );
	return false;
    }

    return true;
}


bool SeisCBVSPSIO::startWrite( const char* fnm, const SeisTrc& trc )
{
    if ( !tr_->initWrite(new StreamConn(fnm,Conn::Write),trc) )
    {
	errmsg_ = tr("Trying to start writing to '%1':\n%2")
			.arg( fnm ).arg( tr_->errMsg() );
	return false;
    }

    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci= tr_->componentInfo();
    const DataCharacteristics dc( reqdtype_ == OD::AutoDataRep
				? trc.data().getInterpreter()->dataChar()
				: DataCharacteristics( reqdtype_ ) );
    for ( int idx=0; idx<ci.size(); idx++ )
	ci[idx]->datachar_ = dc;
    return true;
}


static const char* posdataFileName( const char* dirnm )
{
    mDeclStaticString( ret );
    ret = File::Path( dirnm, cPosDataFnm ).fullPath();
    return ret.buf();
}

#define mRemoveCache(fnm) File::remove( fnm )


SeisCBVSPS3DReader::SeisCBVSPS3DReader( const char* dirnm, int inl )
	: SeisCBVSPSIO(dirnm)
	, posdata_(*new PosInfo::SortedCubeData)
	, curinl_(mUdf(int))
{
    if ( !dirNmOK(true) ) return;

    const BufferString cachefnm( posdataFileName(dirnm_) );
    if ( mIsUdf(inl) )
    {
	od_istream istrm( cachefnm );
	if ( istrm.isOK() )
	{
	    if ( posdata_.read(istrm,true) && !posdata_.isEmpty() )
		return;
	    mRemoveCache(cachefnm);
	}
    }
    else
    {
	if ( inl >= 0 )
	    addInl( inl );
	return;
    }

    DirList dl( dirnm_, File::FilesInDir, selmask_.buf() );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const int inlnr = getInlNr( dl.get(idx) );
	if ( inlnr > 0 )
	    addInl( inlnr );
    }

    if ( posdata_.size() < 1 )
    {
	errmsg_ =
	    tr("Directory '%1' contains no usable prestack data files")
	       .arg( dirnm_ );
	return;
    }

    od_ostream ostrm( cachefnm );
    if ( ostrm.isOK() && !posdata_.write(ostrm,true) )
	mRemoveCache(cachefnm);
}


SeisCBVSPS3DReader::~SeisCBVSPS3DReader()
{
    delete &posdata_;
}


void SeisCBVSPS3DReader::addInl( int inl )
{
    if ( !mkTr(inl) ) return;

    PosInfo::LineData* newid = new PosInfo::LineData( inl );
    const CBVSInfo::SurvGeom& sg = tr_->readMgr()->info().geom_;

    if ( sg.fullyrectandreg )

	newid->segments_ += PosInfo::LineData::Segment( sg.start.inl(),
					sg.stop.inl(), sg.step.inl() );
    else
    {
	const PosInfo::CubeData& cd = sg.cubedata;
	if ( cd.size() < 1 )
	    { pErrMsg("Huh? should get error earlier"); delete newid; return; }

	PosInfo::LineData::Segment seg( cd[0]->linenr_, cd[0]->linenr_, 1 );
	if ( cd.size() > 1 )
	{
	    seg.stop = cd[1]->linenr_;
	    seg.step = seg.stop - seg.start;
	    for ( int idx=2; idx<cd.size(); idx++ )
	    {
		const PosInfo::LineData& id = *cd[idx];
		if ( seg.stop == seg.start )
		    { seg.stop = id.linenr_; seg.step = seg.stop - seg.start; }
		else if ( id.linenr_ != cd[idx-1]->linenr_ + seg.step )
		{
		    newid->segments_ += seg;
		    seg.start = seg.stop = id.linenr_;
		}
		else
		    seg.stop = id.linenr_;
	    }
	    newid->segments_ += seg;
	}
    }

    posdata_ += newid;
}


bool SeisCBVSPS3DReader::mkTr( int inl ) const
{
    if ( tr_ && curinl_ == inl )
	return true;
    else if ( mIsUdf(inl) )
	return false;

    CBVSSeisTrcTranslator*& trans = const_cast<CBVSSeisTrcTranslator*&>(tr_);
    delete trans; trans = 0;
    curinl_ = inl;

    const File::Path fp( dirnm_, BufferString("",inl,ext()) );
    const BufferString filenm = fp.fullPath();
    if( !File::exists(filenm) )
    {
	errmsg_ = tr("No Prestack data available for inline %1").arg( inl );
	return false;
    }

    errmsg_ = uiString::empty();
    trans = CBVSSeisTrcTranslator::make( filenm, false, false, &errmsg_ );
    return tr_;
}


SeisTrc* SeisCBVSPS3DReader::getNextTrace( const BinID& bid,
					   const Coord& coord ) const
{
    SeisTrc* trc = readNewTrace( bid.crl() );
    if ( !trc ) return 0;
    trc->info().setPos( bid ); trc->info().coord_ = coord;
    return trc;
}


bool SeisCBVSPS3DReader::getGather( int crl, SeisTrcBuf& gath ) const
{
    if ( !prepGather( crl, gath ) )
	return false;

    const BinID bid( curinl_, crl );
    const Coord coord = SI().transform( bid );
    while ( true )
    {
	SeisTrc* trc = getNextTrace( bid, coord );
	if ( !trc )
	    { errmsg_ = tr_->errMsg(); return errmsg_.isEmpty(); }
	gath.add( trc );
    }

    // Not reached
    return true;
}


SeisTrc* SeisCBVSPS3DReader::getTrace( const TrcKey& tk, int nr ) const
{
    return mkTr(tk.lineNr()) && goTo(tk.trcNr(),nr)
	 ? getNextTrace( tk.position(), SI().transform(tk.position()) ) : 0;
}


SeisTrc* SeisCBVSPS3DReader::getTrace( const BinID& bid, int nr ) const
{
    return mkTr(bid.inl()) && goTo(bid.crl(),nr)
	 ? getNextTrace( bid, SI().transform(bid) ) : 0;
}


bool SeisCBVSPS3DReader::getGather( const TrcKey& tk, SeisTrcBuf& gath ) const
{
    return mkTr( tk.lineNr() ) && getGather( tk.trcNr(), gath );
}


bool SeisCBVSPS3DReader::getGather( const BinID& bid, SeisTrcBuf& gath ) const
{
    return mkTr( bid.inl() ) && getGather( bid.crl(), gath );
}


StepInterval<float> SeisCBVSPS3DReader::getZRange() const
{
    StepInterval<float> ret = SI().zRange();
    if ( posdata_.isEmpty() )
	return ret;

    const PosInfo::LineData& ld = *posdata_[0];
    SeisTrc* trc = getTrace( BinID(ld.linenr_,ld.segments_[0].start), 0 );
    if ( trc )
    {
	ret = trc->zRange();
	delete trc;
    }
    return ret;
}


#define mRemCacheIfExists() \
    const BufferString cachefnm( posdataFileName(dirnm_) ); \
    if ( File::exists(cachefnm) ) \
	mRemoveCache(cachefnm)

SeisCBVSPS3DWriter::SeisCBVSPS3DWriter( const char* dirnm )
	: SeisCBVSPSIO(dirnm)
	, prevbid_(*new BinID(mUdf(int),mUdf(int)))
{
    if ( !dirNmOK(false) ) return;
    mRemCacheIfExists();
}


SeisCBVSPS3DWriter::~SeisCBVSPS3DWriter()
{
    delete &prevbid_;
}


void SeisCBVSPS3DWriter::close()
{
    prevbid_ = BinID( mUdf(int), mUdf(int) );
    SeisCBVSPSIO::close();
}


bool SeisCBVSPS3DWriter::newInl( const SeisTrc& trc )
{
    const BinID trcbid = trc.info().binID();
    BufferString fnm( "", trcbid.inl(), ext() );
    File::Path fp( dirnm_, fnm );
    fnm = fp.fullPath();

    if ( tr_ ) delete tr_;
    tr_ = CBVSSeisTrcTranslator::getInstance();
    return startWrite( fnm, trc );
}


bool SeisCBVSPS3DWriter::put( const SeisTrc& trc )
{
    SeisTrcInfo& ti = const_cast<SeisTrcInfo&>( trc.info() );
    const BinID trcbid = ti.binID();
    if ( trcbid.inl() != prevbid_.inl() )
    {
	if ( !newInl(trc) )
	    return false;
	nringather_ = 1;
	if ( mIsUdf(prevbid_.inl()) )
	    mRemoveCache( posdataFileName(dirnm_) );
    }
    else if ( trcbid.crl() != prevbid_.crl() )
	nringather_ = 1;
    prevbid_ = trcbid;

    ti.setPos( BinID(trcbid.crl(),nringather_) );
    bool res = tr_->write( trc );
    ti.setPos( trcbid );
    if ( !res )
	errmsg_ = tr_->errMsg();
    else
	nringather_++;

    return res;
}


SeisCBVSPS2DReader::SeisCBVSPS2DReader( const char* dirnm, Pos::GeomID geomid )
	: SeisCBVSPSIO(dirnm)
	, SeisPS2DReader(geomid)
	, posdata_(*new PosInfo::Line2DData)
{
    init( geomid );
}


SeisCBVSPS2DReader::SeisCBVSPS2DReader( const char* dirnm, const char* lnm )
	: SeisCBVSPSIO(dirnm)
	, SeisPS2DReader(lnm)
	, posdata_(*new PosInfo::Line2DData)
{
    Pos::GeomID geomid = SurvGeom::getGeomID( lnm );
    if ( !mIsUdfGeomID(geomid) )
	init( geomid );
}


void SeisCBVSPS2DReader::init( Pos::GeomID geomid )
{
    if ( !dirNmOK(true) ) return;

    BufferString fnm( get2DFileName(geomid) );
    if ( !File::exists(fnm) )
    {
	errmsg_ = tr("Line %1 does not exist").arg( geomid.name() );
	return;
    }

    errmsg_ = uiString::empty();
    tr_ = CBVSSeisTrcTranslator::make( fnm, false, false, &errmsg_ );
    if ( !tr_ ) return;

    TypeSet<Coord> coords; TypeSet<BinID> binids;
    tr_->readMgr()->getPositions( coords );
    tr_->readMgr()->getPositions( binids );

    int prevnr = mUdf(int);
    for ( int idx=0; idx<coords.size(); idx++ )
    {
	const int curnr = binids[idx].inl();
	if ( curnr != prevnr )
	{
	    PosInfo::Line2DPos p( curnr );
	    p.coord_ = coords[idx];
	    posdata_.add( p );
	    prevnr = curnr;
	}
    }
}


SeisCBVSPS2DReader::~SeisCBVSPS2DReader()
{
    delete &posdata_;
}


SeisTrc* SeisCBVSPS2DReader::getTrace( const TrcKey& tk, int nr ) const
{
    if ( !tr_ )
	return 0;

    if ( !goTo(tk.trcNr(),nr) )
	return 0;

    SeisTrc* trc = readNewTrace( tk.trcNr() );
    if ( trc )
	trc->info().setTrcKey( tk );
    return trc;
}


SeisTrc* SeisCBVSPS2DReader::getTrace( const BinID& bid, int nr ) const
{
    return getTrace( TrcKey(bid,true), nr );
}


bool SeisCBVSPS2DReader::getGather( const TrcKey& tk, SeisTrcBuf& tbuf ) const
{
    if ( !prepGather(tk.trcNr(),tbuf) )
	return false;

    SeisTrc* trc = readNewTrace( tk.trcNr() );
    if ( !trc )
	return false;

    while ( trc )
    {
	trc->info().setTrcKey( tk );
	tbuf.add( trc );
	trc = readNewTrace( tk.trcNr() );
    }

    return true;
}


bool SeisCBVSPS2DReader::getGather( const BinID& bid, SeisTrcBuf& tbuf ) const
{
    return getGather( TrcKey(bid,true), tbuf );
}


SeisCBVSPS2DWriter::SeisCBVSPS2DWriter( const char* dirnm, Pos::GeomID geomid )
	: SeisCBVSPSIO(dirnm)
	, prevnr_(mUdf(int))
	, lnm_(geomid.name())
	, geomid_(geomid)
{
    if ( !dirNmOK(false) ) return;
    mRemCacheIfExists();
}


SeisCBVSPS2DWriter::SeisCBVSPS2DWriter( const char* dirnm, const char* lnm )
	: SeisCBVSPSIO(dirnm)
	, prevnr_(mUdf(int))
	, lnm_(lnm)
	, geomid_(SurvGeom::getGeomID(lnm))
{
    if ( !dirNmOK(false) ) return;
    mRemCacheIfExists();
}


bool SeisCBVSPS2DWriter::ensureTr( const SeisTrc& trc )
{
    if ( tr_ ) return true;

    tr_ = CBVSSeisTrcTranslator::getInstance();
    tr_->setCoordPol( true, true );
    return startWrite( get2DFileName(lnm_), trc );
}


void SeisCBVSPS2DWriter::close()
{
    prevnr_ = mUdf(int);
    SeisCBVSPSIO::close();
}


bool SeisCBVSPS2DWriter::put( const SeisTrc& trc )
{
    if ( !ensureTr(trc) )
	return false;

    SeisTrcInfo& ti = const_cast<SeisTrcInfo&>( trc.info() );
    if ( ti.trcNr() != prevnr_ )
	nringather_ = 1;
    prevnr_ = ti.trcNr();

    const auto realtk( ti.trcKey() );
    ti.setPos( BinID(ti.trcNr(),nringather_) );
    bool res = tr_->write( trc );
    ti.setTrcKey( realtk );
    if ( !res )
	errmsg_ = tr_->errMsg();
    else
	nringather_++;

    return res;
}
