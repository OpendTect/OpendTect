/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : CBVS File pack reading
-*/

static const char* rcsID = "$Id: cbvsreadmgr.cc,v 1.63 2011/03/25 15:02:34 cvsbert Exp $";

#include "cbvsreadmgr.h"
#include "cbvsreader.h"
#include "file.h"
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


CBVSReadMgr::CBVSReadMgr( const char* fnm, const CubeSampling* cs,
		  bool single_file, bool glob_info_only, bool forceusecbvsinfo )
	: CBVSIOMgr(fnm)
	, info_(*new CBVSInfo)
	, vertical_(false)
	, rdr1firstsampnr_(0)
{
    bool foundone = false;

    if ( !fnm || !strcmp(fnm,StreamProvider::sStdIO()) )
    {
	addReader( &std::cin, cs, glob_info_only, forceusecbvsinfo );
	if ( readers_.isEmpty() )
	    errmsg_ = "Standard input contains no relevant data";
	else
	    createInfo();
	return;
    }

    bool alreadyfailed = false;
    for ( int fnr=0; ; fnr++ )
    {
	BufferString fname = single_file ? fnm : getFileName(fnr).buf();
	if ( !File::exists((const char*)fname) )
	    break;

	foundone = true;
	if ( !addReader(fname,cs,glob_info_only,forceusecbvsinfo) )
	{
	    if ( *(const char*)errmsg_ )
		return;
	}
	else
	    fnames_ += new BufferString( fname );

	if ( single_file ) break;
    }

    if ( readers_.isEmpty() )
    {
	if ( foundone )
	    mkErrMsg( errmsg_, basefname_, "contains no relevant data" );
	else
	    mkErrMsg( errmsg_, basefname_, "cannot be opened" );
	return;
    }

    createInfo();
}


CBVSReadMgr::~CBVSReadMgr()
{
    close();
    delete &info_;
    deepErase( readers_ );
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


bool CBVSReadMgr::addReader( const char* fname, const CubeSampling* cs,
				bool info_only, bool forceusecbvsinfo )
{
    StreamData sd = StreamProvider(fname).makeIStream();
    if ( !sd.usable() )
    {
	mkErrMsg( errmsg_, fname, "cannot be opened" );
	sd.close();
	return false;
    }

    return addReader( sd.istrm, cs, info_only, forceusecbvsinfo );
}


bool CBVSReadMgr::addReader( std::istream* strm, const CubeSampling* cs,
				bool info_only, bool usecbvsinfo )
{
    CBVSReader* newrdr = new CBVSReader( strm, info_only, usecbvsinfo );
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

    readers_ += newrdr;
    return true;
}


int CBVSReadMgr::pruneReaders( const CubeSampling& cs )
{
    if ( cs.isEmpty() )
	return readers_.size();

    for ( int idx=(vertical_?1:0); idx<readers_.size(); idx++ )
    {
	CBVSReader* rdr = readers_[idx];
	const CBVSInfo& localinfo = rdr->info();
	if ( localinfo.contributesTo(cs) ) continue;

	if ( !localinfo.geom.includesInline(-1)
	  && !localinfo.geom.includesInline(-2) )
	{
	    delete readers_.remove( idx );
	    fnames_.remove( idx );
	    idx--;
	}
    }

    rdr1firstsampnr_ = 0;
    if ( vertical_ && readers_.size() > 1 )
    {
	// Readers may have been pruned.
	SamplingData<float> sd0 = readers_[0]->info().sd;
	float start1 = readers_[1]->info().sd.start;
	rdr1firstsampnr_ = (int)((start1 - sd0.start) / sd0.step + .5);
    }

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

    const CBVSInfo& rdrinfo = rdr->info();
    if ( rdrinfo.nrtrcsperposn != info_.nrtrcsperposn )
	mErrRet("Number of traces per position")
    if ( !rdrinfo.geom.fullyrectandreg )
	const_cast<CBVSInfo&>(rdrinfo).geom.step.inl = info_.geom.step.inl;
    else if ( rdrinfo.geom.step.inl != info_.geom.step.inl )
	mErrRet("In-line number step")
    if ( rdrinfo.geom.step.crl != info_.geom.step.crl )
	mErrRet("Cross-line number step")
    if ( !mIsEqual(rdrinfo.sd.step,info_.sd.step,mDefEps) )
	mErrRet("Sample interval")
    if ( mIsEqual(rdrinfo.sd.start,info_.sd.start,mDefEps) )
    {
	// Normal, horizontal (=vertical optimised)  storage
	if ( rdrinfo.nrsamples != info_.nrsamples )
	    mErrRet("Number of samples")
    }
    else
    {
	StepInterval<float> intv = info_.sd.interval(info_.nrsamples);
	intv.stop += info_.sd.step;
	float diff = rdrinfo.sd.start - intv.stop;
	if ( diff < 0 ) diff = -diff;
	if ( diff > info_.sd.step / 10  )
	{
	    mErrMsgMk("Time range")
	    errmsg_ += "\nis unexpected.\nExpected: ";
	    errmsg_ += intv.stop; errmsg_ += " s.\nFound: ";
	    errmsg_ += rdrinfo.sd.start; errmsg_ += ".";
	    return false;
	}
	vertical_ = true;
	info_.nrsamples += rdrinfo.nrsamples;
    }

    if ( !vertical_ )
	info_.geom.merge( rdrinfo.geom );
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
	for ( int idx=0; idx<readers_.size(); idx++ )
	    if ( !readers_[idx]->goTo(bid) )
		return false;
    }
    else
    {
	int rdrnr = curnr_;
	while ( !readers_[rdrnr]->goTo( bid ) )
	{
	    rdrnr = nextRdrNr( rdrnr );
	    if ( rdrnr < 0 ) return false;
	}
	curnr_ = rdrnr;
    }

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


void CBVSReadMgr::getIsRev( bool& inl, bool& crl ) const
{
    inl = info_.geom.step.inl < 0;
    crl = info_.geom.step.crl < 0;
}


const TypeSet<Coord>& CBVSReadMgr::trailerCoords() const
{
    return readers_[curnr_]->trailerCoords();
}


void CBVSReadMgr::getPositions( TypeSet<BinID>& posns ) const
{
    posns.erase();
    BinID bid;
    if ( info_.geom.fullyrectandreg )
    {
	for ( bid.inl=info_.geom.start.inl;
	      bid.inl!=info_.geom.stop.inl+info_.geom.step.inl;
	      bid.inl += info_.geom.step.inl )
	{
	    for ( bid.crl=info_.geom.start.crl;
		  bid.crl!=info_.geom.stop.crl+info_.geom.step.crl;
		  bid.crl += info_.geom.step.crl )
		posns += bid;
	}
    }
    else
    {
	for ( int iinl=0; iinl<info_.geom.cubedata.size(); iinl++ )
	{
	    const PosInfo::LineData& inlinf = *info_.geom.cubedata[iinl];
	    bid.inl = inlinf.linenr_;
	    for ( int iseg=0; iseg<inlinf.segments_.size(); iseg++ )
	    {
		const PosInfo::LineData::Segment seg = inlinf.segments_[iseg];
		if ( seg.step > 0 )
		    for ( bid.crl=seg.start; bid.crl<=seg.stop;
			  bid.crl+=seg.step )
			posns += bid;
		else
		{
		    StepInterval<int> lseg( seg );
		    if ( lseg.start < lseg.stop ) 
			Swap( lseg.start, lseg.stop );
		    for ( bid.crl=lseg.stop; bid.crl<=seg.start;
			  bid.crl-=seg.step )
			posns += bid;
		}
	    }
	}
    }
}


void CBVSReadMgr::getPositions( TypeSet<Coord>& posns ) const
{
    posns.erase();
    CBVSIO::CoordPol cp = readers_[0]->coordPol();
    if ( cp == CBVSIO::InTrailer )
    {
	posns.append( readers_[0]->trailerCoords() );
	if ( !vertical_ && readers_.size() > 1 )
	{
	    for ( int idx=1; idx<readers_.size(); idx++ )
		posns.append( readers_[idx]->trailerCoords() );
	}
    }
    else if ( cp == CBVSIO::NotStored )
    {
	TypeSet<BinID> bids; getPositions( bids );
	for ( int idx=0; idx<bids.size(); idx++ )
	    posns += SI().transform( bids[idx] );
    }
    else
    {
	CBVSReadMgr* ncthis = const_cast<CBVSReadMgr*>( this );
	ncthis->toStart(); PosAuxInfo pai;
	ncthis->getAuxInfo( pai ); posns += pai.coord;
	while ( ncthis->toNext() )
	    { ncthis->getAuxInfo( pai ); posns += pai.coord; }
	ncthis->toStart();
    }
}


bool CBVSReadMgr::getAuxInfo( PosAuxInfo& pad )
{
    return readers_[curnr_]->getAuxInfo( pad );
}


bool CBVSReadMgr::fetch( void** d, const bool* c,
			 const Interval<int>* ss )
{
    if ( !vertical_ )
	return readers_[curnr_]->fetch( d, c, ss );

    // Need to glue the parts into the buffer.
    // The code looks simple, but that's how it got after many cycles
    // of compression.

    Interval<int> selsamps( ss ? ss->start : 0, ss ? ss->stop : mUdf(int) );
    selsamps.sort();

    const int rdr0nrsamps = readers_[0]->info().nrsamples;
    if ( selsamps.start < rdr0nrsamps )
    {
	Interval<int> rdrsamps = selsamps;
	if ( selsamps.stop >= rdr0nrsamps ) rdrsamps.stop = rdr0nrsamps-1;

	if ( !readers_[0]->fetch(d,c,&rdrsamps,0) )
	    return false;
    }

    Interval<int> cursamps( rdr1firstsampnr_, rdr1firstsampnr_-1 );

    for ( int idx=1; idx<readers_.size(); idx++ )
    {
	cursamps.stop += readers_[idx]->info().nrsamples;

	const bool islast = cursamps.stop >= selsamps.stop;
	if ( islast ) cursamps.stop = selsamps.stop;
	if ( cursamps.stop >= selsamps.start )
	{
	    const int sampoffs = selsamps.start - cursamps.start;
	    Interval<int> rdrsamps( sampoffs < 0 ? 0 : sampoffs,
		    		   cursamps.stop - cursamps.start );
	    if ( !readers_[idx]->fetch( d, c, &rdrsamps,
					sampoffs > 0 ? 0 : -sampoffs ) )
		return false;
	}

	if ( islast ) break;
	cursamps.start = cursamps.stop + 1;
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
	if ( !File::exists((const char*)fname) ) break;

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


static void putComps( std::ostream& strm,
		      const ObjectSet<BasicComponentInfo>& cinfo )
{
    for ( int idx=0; idx<cinfo.size(); idx++ )
    {
	const BasicComponentInfo& bci = *cinfo[idx];
	strm << "\nComponent " << idx+1 << ": '" << bci.name() << "'";
    }
}


static void handleInlGap( std::ostream& strm, Interval<int>& inlgap )
{
    if ( inlgap.start == inlgap.stop )
	strm << "\nInline " << inlgap.start << " not present.";
    else
	strm << "\nInlines " << inlgap.start
	    	      << '-' << inlgap.stop << " not present.";

    strm.flush();
}


void CBVSReadMgr::dumpInfo( std::ostream& strm, bool inclcompinfo ) const
{
    const int singinl = info().geom.start.inl == info().geom.stop.inl
			? info().geom.start.inl : -999;
    const char* Datastr = singinl == -999 ? "Cube" : "Data";
    const char* datastr = singinl == -999 ? "cube" : "data";
    if ( nrReaders() > 1 )
	strm << Datastr << " is stored in " << nrReaders() << " files\n";
    strm << '\n';

    if ( info().nrtrcsperposn > 1 )
	strm << info().nrtrcsperposn << " traces per position" << std::endl;

    strm << "The " << datastr
	 << (info().geom.fullyrectandreg ?
	    (singinl == -999 ? " is 100% rectangular.": " has no gaps.")
	    : " is irregular.")
	 << '\n';

    if ( info().compinfo.size() > 1 )
    {
	putComps( strm, info().compinfo );
	strm << '\n';
    }
    strm << '\n';

    if ( singinl != -999 )
    {
	strm << "Line number: " << singinl << '\n';
	strm << "Trace range: ";
    }
    else
    {
	strm << "In-line range: " << info().geom.start.inl << " - "
	     << info().geom.stop.inl
	     << " (step " << info().geom.step.inl << ").\n";
	strm << "X-line range: ";
    }
    strm << info().geom.start.crl << " - " << info().geom.stop.crl
	 << " (step " << info().geom.step.crl << ").\n";
    strm << "\nZ start: " << info().sd.start
	 << " step: " << info().sd.step << '\n';
    strm << "Number of samples: " << info().nrsamples << "\n\n";
    strm << std::endl;


    if ( !info().geom.fullyrectandreg )
    {
	strm << "Gaps: "; strm.flush();
	bool inlgaps = false; bool crlgaps = false;
	int inlstep = info().geom.step.inl;
	if ( inlstep < 0 ) inlstep = -inlstep;
	Interval<int> inlgap;
	inlgap.start = mUdf(int);
	for ( int inl=info().geom.start.inl; inl<=info().geom.stop.inl;
		inl += inlstep )
	{
	    const int inlinfidx = info().geom.cubedata.indexOf( inl );
	    if ( inlinfidx < 0 )
	    {
		inlgaps = true;
		if ( mIsUdf(inlgap.start) )
		    inlgap.start = inlgap.stop = inl;
		else
		    inlgap.stop = inl;
	    }
	    else
	    {
		const PosInfo::LineData& inlinf =
		    	*info().geom.cubedata[inlinfidx];
		if ( inlinf.segments_.size() > 1 )
		    crlgaps = true;
		if ( !mIsUdf(inlgap.start) )
		{
		    handleInlGap( strm, inlgap );
		    mSetUdf(inlgap.start);
		}
	    }
	}

	if ( crlgaps )
	{
	    if ( inlgaps )
		strm << "\nX-line/Trace number gaps also found.";
	    else
		strm << " Gaps present.";
	}
	else
	{
	    if ( inlgaps )
		strm << "\nNo X-line/Trace number gaps found.";
	    else
		strm << " not present.";
	}
	strm << std::endl << std::endl;
    }
}
