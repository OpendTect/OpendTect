/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : CBVS File pack reading
-*/

static const char* rcsID = "$Id: cbvsreadmgr.cc,v 1.29 2003-11-12 12:57:04 bert Exp $";

#include "cbvsreadmgr.h"
#include "cbvsreader.h"
#include "filegen.h"
#include "strmprov.h"
#include "survinfo.h"
#include "datachar.h"
#include "cubesampling.h"
#include "errh.h"
#include "strmprov.h"
#include <iostream>

static inline void mkErrMsg( BufferString& errmsg, const char* fname,
			     const char* msg )
{
    errmsg = "'"; errmsg += fname; errmsg += "' ";
    errmsg += msg;
}


CBVSReadMgr::CBVSReadMgr( const char* fnm, const CubeSampling* cs )
	: CBVSIOMgr(fnm)
	, info_(*new CBVSInfo)
	, vertical_(false)
	, auxstrm_(0)
	, haveaux_(false)
	, auxinlidx_(0)
	, auxcrlidx_(0)
    	, iinterp(DataCharacteristics())
    	, finterp(DataCharacteristics())
    	, dinterp(DataCharacteristics())
	, auxnrbytes_(0)
{
    bool foundone = false;

    if ( !fnm || !strcmp(fnm,StreamProvider::sStdIO) )
    {
	addReader( &cin, cs );
	if ( !readers_.size() )
	    errmsg_ = "Standard input contains no relevant data";
	else
	    createInfo();
	return;
    }

    bool alreadyfailed = false;
    for ( int fnr=0; ; fnr++ )
    {
	BufferString fname = getFileName( fnr );
	if ( !File_exists((const char*)fname) )
	    break;

	foundone = true;
	if ( !addReader(fname,cs) )
	{
	    if ( *(const char*)errmsg_ )
		return;
	}
	else
	    fnames_ += new BufferString( fname );
    }

    if ( !readers_.size() )
    {
	if ( foundone )
	    mkErrMsg( errmsg_, basefname_, "contains no relevant data" );
	else
	    mkErrMsg( errmsg_, basefname_, "cannot be opened" );
	return;
    }

    handleAuxFile();
    createInfo();
}


// Aux file can contain 'trace header' data instead of in the main files.
// Not (yet) used in OpendTect.

#define mErrRet(s) \
	{ BufferString msg( "Auxiliary file: " ); msg += s; ErrMsg(s); \
	    delete auxstrm_; auxstrm_ = 0; return; }

void CBVSReadMgr::handleAuxFile()
{
    BufferString fname = getFileName( -1 );
    if ( !File_exists((const char*)fname) )
	return;

    StreamData sd = StreamProvider(fname).makeIStream();
    auxstrm_ = sd.istrm;
    if ( !auxstrm_ ) return;

    const char* res = CBVSReader::check( *auxstrm_ );
    if ( res ) mErrRet(res)

    auxstrm_->seekg( 3, ios::beg );
    char plf; auxstrm_->read( &plf, 1 );
    DataCharacteristics dc;
    dc.littleendian = plf != 0;
    finterp.set( dc );
    dc.setNrBytes( BinDataDesc::N8 );
    dinterp.set( dc );
    dc.BinDataDesc::set( true, true, BinDataDesc::N4 );
    iinterp.set( dc );

    auxstrm_->seekg( 12, ios::beg );
    unsigned char* ptr = &auxflgs_;
    auxstrm_->read( (char*)ptr, 1 );
#define mBytes(n,t) (mAuxSetting(ptr,n) ? sizeof(t) : 0)
    auxnrbytes_ = mBytes(1,float)
		+ mBytes(2,double) + mBytes(2,double) // x and y
		+ mBytes(4,float)
		+ mBytes(8,float)
		+ mBytes(16,float)
		+ mBytes(32,float);

    auxstrm_->seekg( -3, ios::end );
    char buf[4];
    auxstrm_->read( buf, 3 ); buf[3] = '\0';
    if ( strcmp(buf,"BGd") ) mErrRet("Missing required file trailer")
    
    handleAuxTrailer();
}


void CBVSReadMgr::handleAuxTrailer()
{
    char buf[20];
    auxstrm_->seekg( -4-CBVSIO::integersize, ios::end );
    auxstrm_->read( buf, CBVSIO::integersize );
    const int nrbytes = iinterp.get( buf, 0 );

    auxstrm_->seekg( -4-CBVSIO::integersize-nrbytes, ios::end );
    auxstrm_->read( buf, CBVSIO::integersize );
    const int nrinl = iinterp.get( buf, 0 );
    if ( nrinl == 0 ) mErrRet("No traces in file")

    for ( int iinl=0; iinl<nrinl; iinl++ )
    {
	if ( !auxstrm_->good() )
	{
	    deepErase( auxinlinfs_ );
	    mErrRet("File trailer not complete")
	}
	auxstrm_->read( buf, 2 * CBVSIO::integersize );
	const int inl = iinterp.get( buf, 0 );
	const int nrxl = iinterp.get( buf, 1 );
	if ( !nrxl ) continue;

	AuxInlInf* aii = new AuxInlInf( inl );
	for ( int icrl=0; icrl<nrxl; icrl++ )
	{
	    auxstrm_->read( buf, CBVSIO::integersize );
	    aii->xlines += iinterp.get( buf, 0 );
	}
	aii->cumnrxlines = aii->xlines.size();
	if ( iinl )
	    aii->cumnrxlines += auxinlinfs_[iinl-1]->cumnrxlines;
	auxinlinfs_ += aii;
    }

    haveaux_ = auxinlinfs_.size();
    if ( haveaux_ )
    {
	for ( int idx=0; idx<readers_.size(); idx++ )
	    readers_[idx]->fetchAuxInfo( false );
    }
}


CBVSReadMgr::~CBVSReadMgr()
{
    close();
    delete &info_;
    deepErase( readers_ );
    deepErase( auxinlinfs_ );
}


void CBVSReadMgr::close()
{
    for ( int idx=0; idx<readers_.size(); idx++ )
	readers_[idx]->close();
}


const char* CBVSReadMgr::errMsg_() const
{
    return readers_.size() ? readers_[curnr_]->errMsg() : 0;
}


bool CBVSReadMgr::addReader( const char* fname, const CubeSampling* cs )
{
    StreamData sd = StreamProvider(fname).makeIStream();
    if ( !sd.usable() )
    {
	mkErrMsg( errmsg_, fname, "cannot be opened" );
	sd.close();
	return false;
    }

    return addReader( sd.istrm, cs );
}


bool CBVSReadMgr::addReader( istream* strm, const CubeSampling* cs )
{
    CBVSReader* newrdr = new CBVSReader( strm );
    if ( newrdr->errMsg() )
    {
	errmsg_ = newrdr->errMsg();
	delete newrdr;
	return false;
    }

    if ( cs && !newrdr->info().contributesTo(*cs) )
    {
	delete newrdr;
	return false;
    }

    if ( !readers_.size() )
	haveaux_ = newrdr->hasAuxInfo();
    else if ( vertical_ )
	newrdr->fetchAuxInfo( false );

    readers_ += newrdr;
    return true;
}


int CBVSReadMgr::pruneReaders( const CubeSampling& cs )
{
    if ( cs.isEmpty() )
	return readers_.size();

    for ( int idx=0; idx<readers_.size(); idx++ )
    {
	CBVSReader* rdr = readers_[idx];
	const CBVSInfo& localinfo = rdr->info();
	if ( localinfo.contributesTo(cs) ) continue;

	if ( !localinfo.geom.includesInline(-1)
	  && !localinfo.geom.includesInline(-2) )
	{
	    BufferString* fname = fnames_[idx];
	    readers_ -= rdr; fnames_ -= fname;
	    delete rdr; delete fname;
	    idx--;
	}
    }

    createInfo();
    return readers_.size();
}


void CBVSReadMgr::createInfo()
{
    const int sz = readers_.size();
    if ( sz == 0 ) return;
    info_ = readers_[0]->info();
    if ( !info_.geom.step.inl ) // unknown, get from other source
    {
	int rdrnr = 1;
	while ( rdrnr < sz )
	{
	    if ( readers_[rdrnr]->info().geom.step.inl )
	    {
		info_.geom.step.inl = readers_[rdrnr]->info().geom.step.inl;
	        break;
	    }
	}
	if ( !info_.geom.step.inl )
	    info_.geom.step.inl = SI().inlStep();
    }

    for ( int idx=1; idx<sz; idx++ )
	if ( !handleInfo(readers_[idx],idx) ) return;
}


#define mErrMsgMk(s) \
    errmsg_ = s; \
    errmsg_ += " found in:\n"; errmsg_ += *fnames_[ireader];

#undef mErrRet
#define mErrRet(s) { \
    mErrMsgMk(s) \
    errmsg_ += "\ndiffers from first file"; \
    return false; \
}

bool CBVSReadMgr::handleInfo( CBVSReader* rdr, int ireader )
{
    if ( !ireader ) return true;

    const CBVSInfo& ci = rdr->info();
    if ( ci.nrtrcsperposn != info_.nrtrcsperposn )
	mErrRet("Number of traces per position")
    if ( !ci.geom.fullyrectandreg )
	const_cast<CBVSInfo&>(ci).geom.step.inl = info_.geom.step.inl;
    else if ( ci.geom.step.inl != info_.geom.step.inl )
	mErrRet("In-line number step")
    if ( ci.geom.step.crl != info_.geom.step.crl )
	mErrRet("Cross-line number step")

    for ( int icomp=0; icomp<ci.compinfo.size(); icomp++ )
    {
	const BasicComponentInfo& cicompinf = *ci.compinfo[icomp];
	BasicComponentInfo& compinf = *info_.compinfo[icomp];
	if ( !mIS_ZERO(cicompinf.sd.step-compinf.sd.step) )
	    mErrRet("Sample interval")
	if ( mIS_ZERO(cicompinf.sd.start-compinf.sd.start) )
	{
	    if ( cicompinf.nrsamples != compinf.nrsamples )
		mErrRet("Number of samples")
	}
	else
	{
	    StepInterval<float> intv = compinf.sd.interval(compinf.nrsamples);
	    intv.stop += compinf.sd.step;
	    float diff = cicompinf.sd.start - intv.stop;
	    if ( diff < 0 ) diff = -diff;
	    if ( diff > compinf.sd.step / 10  )
	    {
		mErrMsgMk("Time range")
		errmsg_ += "\nis unexpected.\nExpected: ";
		errmsg_ += intv.stop; errmsg_ += " s.\nFound: ";
		errmsg_ += cicompinf.sd.start; errmsg_ += " s.";
		return false;
	    }
	    vertical_ = true;
	    compinf.nrsamples += cicompinf.nrsamples;
	}
    }

    if ( !vertical_ )
	info_.geom.merge( ci.geom );
    // We'll just assume that in vertical situation the files have exactly
    // the same geometry ...

    return true;
}


int CBVSReadMgr::nextRdrNr( int rdrnr ) const
{
    rdrnr++;
    if ( rdrnr >= readers_.size() ) rdrnr = 0;
    if ( rdrnr == curnr_ ) rdrnr = -1;
    return rdrnr;
}


BinID CBVSReadMgr::nextBinID() const
{
    int rdrnr = curnr_;
    BinID ret = readers_[rdrnr]->nextBinID();
    if ( vertical_ ) return ret;

    while ( 1 )
    {
	if ( ret != BinID(0,0) )
	    return ret;
	else
	{
	    rdrnr++;
	    if ( rdrnr >= readers_.size() )
		return ret;
	}
	ret = readers_[rdrnr]->nextBinID();
    }

    return ret;
}


bool CBVSReadMgr::goTo( const BinID& bid )
{
    const int sz = readers_.size();
    if ( sz < 2 )
	return readers_[0]->goTo(bid);

    if ( vertical_ )
    {
	CBVSReader& rdr0 = *readers_[0];
	int posnr = rdr0.getPosNr(bid,rdr0.curinlinfnr_,rdr0.cursegnr_);
	if ( posnr < 0 ) return false;

	for ( int idx=0; idx<readers_.size(); idx++ )
	{
	    if ( !readers_[idx]->goTo(posnr,bid,
				      rdr0.curinlinfnr_,rdr0.cursegnr_) )
		return false;
	}
	return true;
    }

    int rdrnr = curnr_;

    while ( !readers_[rdrnr]->goTo( bid ) )
    {
	rdrnr = nextRdrNr( rdrnr );
	if ( rdrnr < 0 ) return false;
    }

    curnr_ = rdrnr;
    return true;
}


bool CBVSReadMgr::toNext()
{
    if ( vertical_ )
    {
	for ( int idx=0; idx<readers_.size(); idx++ )
	{
	    if ( !readers_[idx]->toNext() )
		return false;
	}
	return true;
    }

    if ( !readers_[curnr_]->toNext() )
    {
	if ( curnr_ == readers_.size()-1 ) return false;
	curnr_++;
	return readers_[curnr_]->toStart();
    }

    return true;
}


bool CBVSReadMgr::toStart()
{
    if ( vertical_ )
    {
	for ( int idx=0; idx<readers_.size(); idx++ )
	{
	    if ( !readers_[idx]->toStart() )
		return false;
	}
	return true;
    }

    curnr_ = 0;
    return readers_[curnr_]->toStart();
}


bool CBVSReadMgr::skip( bool fnp )
{
    if ( vertical_ )
    {
	for ( int idx=0; idx<readers_.size(); idx++ )
	{
	    if ( !readers_[idx]->skip(fnp) )
		return false;
	}
	return true;
    }

    if ( !readers_[curnr_]->skip(fnp) )
    {
	if ( curnr_ == readers_.size()-1 ) return false;
	curnr_++;
	return readers_[curnr_]->toStart();
    }

    return true;
}


void CBVSReadMgr::fetchAuxInfo( bool yn )
{
    bool enablerdrs = yn;
    if ( auxstrm_ )
    {
	enablerdrs = false;
	if ( !yn )
	{
	    delete auxstrm_; auxstrm_ = 0;
	    deepErase( auxinlinfs_ );
	}
    }

    for ( int idx=0; idx<readers_.size(); idx++ )
	readers_[idx]->fetchAuxInfo( enablerdrs );
}


bool CBVSReadMgr::getAuxInfo( PosAuxInfo& pad )
{
    if ( !haveaux_ )
	return true;
    else if ( !auxstrm_ )
	return readers_[curnr_]->getAuxInfo( pad );

    BinID binid = binID();
    const AuxInlInf* aii = auxinlinfs_[auxinlidx_];
    if ( aii->inl == binid.inl && aii->xlines[auxcrlidx_] == binid.crl )
	{ getAuxFromFile(pad); return true; }
    auxcrlidx_++;
    if ( auxcrlidx_ >= aii->xlines.size() )
    {
	auxcrlidx_ = 0; auxinlidx_++;
	if ( auxinlidx_ >= auxinlinfs_.size() )
	    auxinlidx_ = 0;
	aii = auxinlinfs_[auxinlidx_];
    }
    if ( aii->inl == binid.inl && aii->xlines[auxcrlidx_] == binid.crl )
	{ getAuxFromFile(pad); return true; }

    // Extensive search necessary.
    for ( auxinlidx_=0; auxinlidx_<auxinlinfs_.size(); auxinlidx_++ )
    {
	aii = auxinlinfs_[auxinlidx_];
	if ( aii->inl != binid.inl ) continue;

	for ( auxcrlidx_=0; auxcrlidx_<aii->xlines.size(); auxcrlidx_++ )
	{
	    if ( aii->xlines[auxcrlidx_] == binid.crl )
		{ getAuxFromFile(pad); return true; }
	}

	break;
    }

    auxinlidx_ = auxcrlidx_ = 0;
    return false;
}


void CBVSReadMgr::getAuxFromFile( PosAuxInfo& pad )
{
    const AuxInlInf* aii = auxinlinfs_[auxinlidx_];
    int posnr = aii->cumnrxlines - aii->xlines.size() + auxcrlidx_;

    streampos pos = CBVSIO::headstartbytes; // start of data
    pos += auxnrbytes_ * info_.nrtrcsperposn * posnr; // start of position
    pos += auxnrbytes_ * readers_[0]->trcNrAtPosition(); // n-th trc on position

    if ( !auxstrm_->seekg(pos,ios::beg) )
	return;

    char buf[2*sizeof(double)];
    unsigned char* ptr = &auxflgs_;
#define mCondGetAuxFromStrm(memb,n) \
    if ( mAuxSetting(ptr,n) ) \
	{ mGetAuxFromStrm(pad,buf,memb,(*auxstrm_)); }
#define mCondGetCoordAuxFromStrm() \
    if ( mAuxSetting(ptr,2) ) \
	{ mGetCoordAuxFromStrm(pad,buf,(*auxstrm_)); }

    mCondGetAuxFromStrm(startpos,1)
    mCondGetCoordAuxFromStrm()
    mCondGetAuxFromStrm(offset,4)
    mCondGetAuxFromStrm(pick,8)
    mCondGetAuxFromStrm(refpos,16)
    mCondGetAuxFromStrm(azimuth,32)
}


bool CBVSReadMgr::fetch( void** d, const bool* c,
			 const Interval<int>* ss )
{
    if ( !vertical_ )
	return readers_[curnr_]->fetch( d, c, ss );

    // Need to glue the parts into the buffer.
    // The code looks simple, but that's how it got after many cycles
    // of compression.

    Interval<int> selsamps( ss ? ss->start : 0, ss ? ss->stop : 2000000000 );
    selsamps.sort();

    Interval<int> avsamps( 0, -1 );

    for ( int idx=0; idx<readers_.size(); idx++ )
    {
	avsamps.stop += readers_[idx]->info().compinfo[0]->nrsamples;

	const bool islast = avsamps.stop >= selsamps.stop;
	if ( islast ) avsamps.stop = selsamps.stop;
	if ( avsamps.stop >= selsamps.start )
	{
	    const int sampoffs = selsamps.start - avsamps.start;
	    Interval<int> rdrsamps( sampoffs < 0 ? 0 : sampoffs,
		    		   avsamps.stop - avsamps.start );
	    if ( !readers_[idx]->fetch( d, c, &rdrsamps,
					sampoffs > 0 ? 0 : -sampoffs ) )
		return false;
	}

	if ( islast ) break;
	avsamps.start = avsamps.stop + 1;
    }

    return true;
}


int CBVSReadMgr::nrComponents() const
{
    return readers_[curnr_]->nrComponents();
}


const BinID& CBVSReadMgr::binID() const
{
    return readers_[curnr_]->binID();
}


const char* CBVSReadMgr::check( const char* basefname )
{
    static BufferString ret;

    int curnr=0;
    for ( ; ; curnr++ )
    {
	BufferString fname = getFileName( basefname, curnr );
	if ( !File_exists((const char*)fname) ) break;

	StreamData sd = StreamProvider(fname).makeIStream();
	const char* res = CBVSReader::check( *sd.istrm );

	if ( res && *res )
	{
	    ret = "'"; ret += fname; ret += "': ";
	    ret += res;
	    return (const char*)ret;
	}
    }

    if ( curnr == 0 )
    {
	ret = "'"; ret += basefname; ret += "' does not exist";
	return (const char*)ret;
    }

    return 0;
}


static void putComps( ostream& strm,
		      const ObjectSet<BasicComponentInfo>& cinfo )
{
    strm << "Data is written on a "
	 << (cinfo[0]->datachar.littleendian ? "little" : "big")
	 << " endian machine.\n";

    for ( int idx=0; idx<cinfo.size(); idx++ )
    {
	const BasicComponentInfo& bci = *cinfo[idx];
	strm << "\nComponent '" << (const char*)bci.name() << "':\n";
	strm << "Data Characteristics: "
	     << (bci.datachar.isInteger() ? "Integer" : "Floating point") <<' ';
	if ( bci.datachar.isInteger() )
	     strm << (bci.datachar.isSigned() ? "(Signed) " : "(Unsigned) ");
	strm << (int)bci.datachar.nrBytes() << " bytes\n";
	strm << "Z/T start: " << bci.sd.start
	     << " step: " << bci.sd.step << '\n';
	strm << "Number of samples: " << bci.nrsamples << "\n\n";
    }
}


static void handleInlGap( ostream& strm, Interval<int>& inlgap )
{
    if ( inlgap.start == inlgap.stop )
	strm << "\nInline " << inlgap.start << " not present.";
    else
	strm << "\nInlines " << inlgap.start
	    	      << '-' << inlgap.stop << " not present.";

    strm.flush();
    inlgap.start = inlgap.stop = -999;
}


void CBVSReadMgr::dumpInfo( ostream& strm, bool inclcompinfo ) const
{
    if ( nrReaders() > 1 )
	strm << "Cube is stored in " << nrReaders() << " files\n";
    strm << '\n';

    if ( info().nrtrcsperposn > 1 )
	strm << info().nrtrcsperposn << " traces per position" << endl;

    if ( inclcompinfo )
	putComps( strm, info().compinfo );

    strm << "The cube is "
	 << (info().geom.fullyrectandreg ? "100% rectangular." : "irregular.")
	 << '\n';
    strm << "In-line range: " << info().geom.start.inl << " - "
	 << info().geom.stop.inl << " (step " << info().geom.step.inl << ").\n";
    strm << "X-line range: " << info().geom.start.crl << " - "
	 << info().geom.stop.crl << " (step " << info().geom.step.crl << ").\n";
    strm << endl;

    Interval<int> inlgap( -999, -999 );

    if ( !info().geom.fullyrectandreg )
    {
	strm << "Gaps: "; strm.flush();
	bool inlgaps = false; bool crlgaps = false;
	for ( int inl=info().geom.start.inl; inl<=info().geom.stop.inl;
		inl += info().geom.step.inl )
	{
	    const CBVSInfo::SurvGeom::InlineInfo* inlinf
		    = info().geom.getInfoFor( inl );
	    if ( !inlinf )
	    {
		inlgaps = true;
		if ( inlgap.start == -999 )
		    inlgap.start = inlgap.stop = inl;
		else if ( inl == inlgap.stop + info().geom.step.inl )
		    inlgap.stop = inl;
		else
		    handleInlGap( strm, inlgap );
		continue;
	    }
	    if ( inlinf->segments.size() > 1 )
		crlgaps = true;
	}
	if ( inlgap.start != -999 )
	    handleInlGap( strm, inlgap );

	if ( crlgaps )
	{
	    if ( inlgaps )
		strm << "\nData holes (X-line gaps) also found.";
	    else
		strm << " Gaps present.";
	}
	else
	{
	    if ( inlgaps )
		strm << "\nNo data holes (X-line gaps) found.";
	    else
		strm << " not present.";
	}
	strm << endl << endl;
    }
}
