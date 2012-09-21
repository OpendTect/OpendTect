/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "seiscbvsps.h"

#include "cbvsreadmgr.h"
#include "dirlist.h"
#include "errh.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "seisbuf.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "seistrc.h"
#include "strmoper.h"
#include "strmprov.h"
#include "survinfo.h"

static const char* cSampNmsFnm = "samplenames.txt";
static const char* cPosDataFnm = "posdata.txt";


class CBVSSeisPSIOProvider : public SeisPSIOProvider
{
public:
	    CBVSSeisPSIOProvider() : SeisPSIOProvider(
		    CBVSSeisTrcTranslator::translKey() ) {}
    SeisPS3DReader*	make3DReader( const char* dirnm, int inl ) const
			{ return new SeisCBVSPS3DReader(dirnm,inl); }
    SeisPSWriter*	make3DWriter( const char* dirnm ) const
			{ return new SeisCBVSPS3DWriter(dirnm); }
    SeisPS2DReader*	make2DReader( const char* dirnm, const char* lnm ) const
			{ return new SeisCBVSPS2DReader(dirnm,lnm); }
    SeisPSWriter*	make2DWriter( const char* dirnm, const char* lnm ) const
			{ return new SeisCBVSPS2DWriter(dirnm,lnm); }
    bool		getLineNames(const char*,BufferStringSet&) const;
    static int		factid;
};

// This adds the CBVS type pre-stack seismics data storage to the factory
int CBVSSeisPSIOProvider::factid = SPSIOPF().add( new CBVSSeisPSIOProvider );


bool CBVSSeisPSIOProvider::getLineNames( const char* dirnm,
					 BufferStringSet& linenms) const
{
    deepErase( linenms );
    DirList dl( dirnm, DirList::FilesOnly );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString filenm = dl.get( idx );
	char* str = filenm.buf();
	int cidx = 0;
	while ( cidx < filenm.size() && str[cidx] != '.')
	    cidx++;

	if ( !cidx || cidx >= filenm.size() || str[cidx] != '.' )
	    continue;

	const char* ext = str + cidx + 1;
	if ( strncmp(ext,"cbvs",4) )
	    continue;

	str[cidx] = '\0';
	linenms.add( filenm );
    }

    return linenms.size();
}


SeisCBVSPSIO::SeisCBVSPSIO( const char* dirnm )
    	: dirnm_(dirnm)
    	, reqdtype_(DataCharacteristics::Auto)
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


BufferString SeisCBVSPSIO::get2DFileName( const char* lnm ) const
{
    BufferString fnm( lnm );
    cleanupString( fnm.buf(), false, false, false );

    FilePath fp( dirnm_, fnm );
    fp.setExtension( "cbvs" );

    fnm = fp.fullPath();
    return fnm;
}


int SeisCBVSPSIO::getInlNr( const char* filenm )
{
    FilePath fp( filenm );
    BufferString fnm( fp.fileName() );
    char* ptr = fnm.buf();
    while ( *ptr && !isdigit(*ptr) ) ptr++;
    while ( *ptr && isdigit(*ptr) ) ptr++;
    *ptr = '\0';
    return fnm.isEmpty() ? -1 : toInt( fnm.buf() );
}


bool SeisCBVSPSIO::get3DFileNames( BufferStringSet& bss,
				   const Interval<int>* inlrg ) const
{
    bss.erase();
    if ( !dirNmOK(true) )
	return false;

    DirList dl( dirnm_, DirList::FilesOnly, selmask_.buf() );
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
	errmsg_ = "No matching files found in data store";
	return false;
    }
    return true;
}


bool SeisCBVSPSIO::getSampleNames( BufferStringSet& nms ) const
{
    const BufferString fnm( FilePath(dirnm_,cSampNmsFnm).fullPath() );
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() ) return false;

    nms.erase();
    BufferString nm;
    while ( StrmOper::readLine(*sd.istrm,&nm) )
	nms.add( nm.buf() );
    sd.close();

    return true;
}


bool SeisCBVSPSIO::setSampleNames( const BufferStringSet& nms ) const
{
    const BufferString fnm( FilePath(dirnm_,cSampNmsFnm).fullPath() );
    if ( nms.isEmpty() )
    {
	if ( File::exists(fnm) )
	    File::remove( fnm );
	return true;
    }

    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() ) return false;

    *sd.ostrm << nms.get(0);
    for ( int idx=1; idx<nms.size(); idx++ )
	*sd.ostrm << '\n' << nms.get(idx);
    sd.close();

    return true;
}


void SeisCBVSPSIO::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( sKey::DataStorage() );
    if ( res && *res )
	reqdtype_ = (DataCharacteristics::UserType)(*res-'0');
}


bool SeisCBVSPSIO::dirNmOK( bool forread ) const
{
    if ( File::isDirectory(dirnm_) )
	return true;

    if ( forread )
    {
	errmsg_ = "Directory '"; errmsg_ += dirnm_;
	errmsg_ += "' does not exist";
	return false;
    }

    File::createDir( dirnm_ );
    if ( !File::isDirectory(dirnm_) )
    {
	errmsg_ = "Cannot create directory '";
	errmsg_ += dirnm_; errmsg_ += "'";
	return false;
    }

    return true;
}


SeisTrc* SeisCBVSPSIO::readNewTrace( int crlnr ) const
{
    SeisTrc* trc = new SeisTrc;
    if ( !tr_->read(*trc) )
	{ delete trc; return 0; }
    if ( trc->info().binid.inl != crlnr )
	{ delete trc; return 0; }

    return trc;
}


bool SeisCBVSPSIO::goTo( int crlnr, int nr ) const
{
    if ( !tr_ ) return false;

    if ( !tr_->goTo( BinID(crlnr,nr+1) ) )
    {
	if ( !tr_->goTo( BinID(crlnr,1) ) )
	    { errmsg_ = "Crossline not present"; return false; }
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
    if ( !tr_->goTo( BinID(crlnr,1) ) )
	{ errmsg_ = "Trace number not present"; return false; }
    return true;
}


bool SeisCBVSPSIO::startWrite( const char* fnm, const SeisTrc& trc )
{
    if ( !tr_->initWrite(new StreamConn(fnm,Conn::Write),trc) )
    {
	errmsg_ = "Trying to start writing to '";
	errmsg_ += fnm; errmsg_ += "':\n";
	errmsg_ += tr_->errMsg();
	return false;
    }

    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci= tr_->componentInfo();
    const DataCharacteristics dc( reqdtype_ == DataCharacteristics::Auto
				? trc.data().getInterpreter()->dataChar()
				: DataCharacteristics( reqdtype_ ) );
    for ( int idx=0; idx<ci.size(); idx++ )
	ci[idx]->datachar = dc;
    return true;
}


static const char* posdataFileName( const char* dirnm )
{
    static BufferString ret;
    ret = FilePath( dirnm, cPosDataFnm ).fullPath();
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
	StreamData sd = StreamProvider(cachefnm).makeIStream();
	if ( sd.usable() )
	{
	    bool posdataok = posdata_.read( *sd.istrm, true );
	    sd.close();
	    if ( !posdataok || posdata_.isEmpty() )
		mRemoveCache(cachefnm);
	    else
		return;
	}
    }
    else
    {
	if ( inl >= 0 )
	    addInl( inl );
	return;
    }

    DirList dl( dirnm_, DirList::FilesOnly, selmask_.buf() );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const int inlnr = getInlNr( dl.get(idx) );
	if ( inlnr > 0 )
	    addInl( inlnr );
    }

    if ( posdata_.size() < 1 )
    {
	errmsg_ = "Directory '"; errmsg_ += dirnm_;
	errmsg_ += "' contains no usable pre-stack data files";
	return;
    }

    StreamData sd = StreamProvider(cachefnm).makeOStream();
    if ( sd.usable() )
    {
	if ( !posdata_.write(*sd.ostrm,true) )
	    mRemoveCache(cachefnm);
    }
    sd.close();
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

	newid->segments_ += PosInfo::LineData::Segment( sg.start.inl,
					sg.stop.inl, sg.step.inl );
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

    CBVSSeisTrcTranslator*& tr = const_cast<CBVSSeisTrcTranslator*&>(tr_);
    delete tr; tr = 0;
    curinl_ = inl;

    const FilePath fp( dirnm_, BufferString("",inl,ext()) );
    const BufferString filenm = fp.fullPath();
    if( !File::exists(filenm) )
    {
	errmsg_ = "No Pre Stack data available for inline "; errmsg_ += inl;
	return false;
    }

    errmsg_ = "";
    tr = CBVSSeisTrcTranslator::make( filenm, false, false, &errmsg_ );
    return tr_;
}


SeisTrc* SeisCBVSPS3DReader::getNextTrace( const BinID& bid,
					   const Coord& coord ) const
{
    SeisTrc* trc = readNewTrace( bid.crl );
    if ( !trc ) return 0;
    trc->info().nr = trc->info().binid.crl;
    trc->info().binid = bid; trc->info().coord = coord;
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


SeisTrc* SeisCBVSPS3DReader::getTrace( const BinID& bid, int nr ) const
{
    return mkTr(bid.inl) && goTo(bid.crl,nr)
	 ? getNextTrace( bid, SI().transform(bid) ) : 0;
}


bool SeisCBVSPS3DReader::getGather( const BinID& bid, SeisTrcBuf& gath ) const
{
    return mkTr( bid.inl ) && getGather( bid.crl, gath );
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
    const BinID& trcbid = trc.info().binid;
    BufferString fnm( "", trcbid.inl, ext() );
    FilePath fp( dirnm_, fnm );
    fnm = fp.fullPath();

    if ( tr_ ) delete tr_;
    tr_ = CBVSSeisTrcTranslator::getInstance();
    return startWrite( fnm, trc );
}


bool SeisCBVSPS3DWriter::put( const SeisTrc& trc )
{
    SeisTrcInfo& ti = const_cast<SeisTrcInfo&>( trc.info() );
    const BinID trcbid = ti.binid;
    if ( trcbid.inl != prevbid_.inl )
    {
	if ( !newInl(trc) )
	    return false;
	nringather_ = 1;
	if ( mIsUdf(prevbid_.inl) )
	    mRemoveCache( posdataFileName(dirnm_) );
    }
    else if ( trcbid.crl != prevbid_.crl )
	nringather_ = 1;
    prevbid_ = trcbid;

    ti.binid = BinID( trcbid.crl, nringather_ );
    bool res = tr_->write( trc );
    ti.binid = trcbid;
    if ( !res )
	errmsg_ = tr_->errMsg();
    else
	nringather_++;

    return res;
}


SeisCBVSPS2DReader::SeisCBVSPS2DReader( const char* dirnm, const char* lnm )
    	: SeisCBVSPSIO(dirnm)
    	, SeisPS2DReader(lnm)
    	, posdata_(*new PosInfo::Line2DData)
{
    if ( !dirNmOK(true) ) return;

    const BufferString fnm( get2DFileName(lnm) );
    if ( !File::exists(fnm) ) return;

    errmsg_ = "";
    tr_ = CBVSSeisTrcTranslator::make( fnm, false, false, &errmsg_ );
    if ( !tr_ ) return;

    TypeSet<Coord> coords; TypeSet<BinID> binids;
    tr_->readMgr()->getPositions( coords );
    tr_->readMgr()->getPositions( binids );

    int prevnr = mUdf(int);
    for ( int idx=0; idx<coords.size(); idx++ )
    {
	const int curnr = binids[idx].inl;
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


SeisTrc* SeisCBVSPS2DReader::getTrace( const BinID& bid, int nr ) const
{
    if ( !tr_ ) return 0;

    if ( !goTo(bid.crl,nr) )
	return 0;

    SeisTrc* trc = readNewTrace( bid.crl );
    if ( !trc ) return 0;
    trc->info().nr = trc->info().binid.inl;
    trc->info().binid = SI().transform( trc->info().coord );
    return trc;
}


bool SeisCBVSPS2DReader::getGather( const BinID& bid, SeisTrcBuf& tbuf ) const
{
    if ( !prepGather(bid.crl,tbuf) )
	return false;

    SeisTrc* trc = readNewTrace( bid.crl );
    if ( !trc ) return false;

    while ( trc )
    {
	trc->info().nr = bid.crl;
	trc->info().binid = SI().transform( trc->info().coord );
	tbuf.add( trc );

	trc = readNewTrace( bid.crl );
    }

    return true;
}


SeisCBVSPS2DWriter::SeisCBVSPS2DWriter( const char* dirnm, const char* lnm )
    	: SeisCBVSPSIO(dirnm)
    	, prevnr_(mUdf(int))
    	, lnm_(lnm)
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
    if ( !ensureTr(trc) ) return false;

    SeisTrcInfo& ti = const_cast<SeisTrcInfo&>( trc.info() );
    if ( ti.nr != prevnr_ )
	nringather_ = 1;
    prevnr_ = ti.nr;

    const BinID trcbid( ti.binid );
    ti.binid = BinID( ti.nr, nringather_ );
    bool res = tr_->write( trc );
    ti.binid = trcbid;
    if ( !res )
	errmsg_ = tr_->errMsg();
    else
	nringather_++;

    return res;
}
